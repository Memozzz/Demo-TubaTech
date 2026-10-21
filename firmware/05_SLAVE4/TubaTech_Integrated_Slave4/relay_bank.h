#pragma once
#include <Arduino.h>
#include "board_map.h"
#include "relay_protocol.h"
#include "session_guard.h"
#include "bench_arm_policy.h"
namespace integrated {
constexpr unsigned channelCount(uint8_t owner){unsigned count=0;for(uint8_t ch=0;ch<14;++ch)if(bankMask(owner)&(1U<<ch))++count;return count;}
// Only this adapter touches GPIO. ACK means commanded GPIO state, not contact
// travel/flow. Bench authorization stays on MASTER; optional per-board arming
// is selected by bench_arm_policy.h. No handshake ever switches a GPIO ON.
struct RelayBank {
  uint8_t owner;
  bool enabled=false,linked=false;
  uint16_t commanded=0;
  uint32_t lastHeartbeat=0;
  SessionGuard guard;
  explicit RelayBank(uint8_t id):owner(id){}
  void off(){for(uint8_t ch=0;ch<14;++ch)if(bankMask(owner)&(1U<<ch))digitalWrite(demo::outputPins[ch],HIGH);commanded=0;}
  void begin(){enabled=linked=false;guard=SessionGuard{};commanded=0;
    for(uint8_t ch=0;ch<14;++ch)if(bankMask(owner)&(1U<<ch)){
      digitalWrite(demo::outputPins[ch],HIGH);pinMode(demo::outputPins[ch],OUTPUT);
    }
    off();
  }
  void disable(){off();enabled=linked=false;guard.disarm();}
  bool arm(){if(!linked||commanded)return false;enabled=true;return true;}
  void tick(uint32_t now){if(linked&&uint32_t(now-lastHeartbeat)>2500)disable();}
  Packet receive(const Packet &p,uint32_t now){
    tick(now);Packet reply(MAGIC,p.session,p.sequence,REJECT,owner,1,0);reply.owner=owner;
    const bool command=p.type==HEARTBEAT||p.type==SET_RELAY||p.type==ALL_OFF||p.type==SYNC||p.type==CONFIRM;
    if(!validEnvelope(p)||p.owner!=owner||p.flags||!command||
       (p.type==SET_RELAY&&(p.channel>=14||!(bankMask(owner)&(1U<<p.channel))||p.value>1||p.stateMask||(!enabled&&p.value)))){
      disable();return reply;
    }
    const auto decision=guard.accept(p.type,p.session,p.sequence,p.channel,p.value,p.stateMask,now,esp_random());
    if(guard.forceOff)off();
    if(decision==SessionGuard::Rejected){disable();return reply;}
    if(decision==SessionGuard::Challenge){
      linked=false;reply.type=CHALLENGE;reply.channel=uint8_t(guard.nonce);reply.value=uint8_t(guard.nonce>>8);reply.stateMask=uint8_t(guard.nonce>>16);reply.flags=0;return reply;
    }
    if(decision!=SessionGuard::Duplicate){
      linked=true;lastHeartbeat=now;
#if TUBATECH_MASTER_ONLY_UNLOADED_ARM
      if(p.type==CONFIRM && commanded==0){
        enabled=true; // Readiness only; no GPIO is switched ON here.
      }
#endif
      if(decision==SessionGuard::Fresh&&p.type==SET_RELAY){
        digitalWrite(demo::outputPins[p.channel],p.value?LOW:HIGH);
        if(p.value)commanded|=uint16_t(1U<<p.channel);else commanded&=uint16_t(~(1U<<p.channel));
      }
    }
    reply.type=ACK;reply.channel=owner;reply.value=0;reply.stateMask=commanded;reply.flags=enabled?1:0;return reply;
  }
};
}
