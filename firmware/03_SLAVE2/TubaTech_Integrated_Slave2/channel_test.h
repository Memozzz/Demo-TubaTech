#pragma once
// Included after MasterIO/engine declarations. One component, unloaded only,
// one second after its ON ACK, then a matching OFF ACK. Not a process function.
struct ChannelTest {
  const demo::Component *component=nullptr;
  enum State{Idle,SendOn,OnAck,Timed,SendOff,OffAck} state=Idle;
  uint32_t ticket=0,at=0;
  bool stopped=false;
  bool busy()const{return state!=Idle;}
  bool start(const char *name,uint32_t now){
    const auto *c=demo::find(name);
    if(!c||busy()||engine.busy()||engine.faulted()||recovering||!engine.benchArmed()||
       masterIO.unsafeNodes()||!masterIO.ready(c->node,now)||!masterIO.allOffKnown(c->node))return false;
    component=c;state=SendOn;at=now;stopped=false;engine.disarmBench();
    Serial.printf("UNLOADED CHANNEL TEST %s; automatic OFF after one second; ON/OFF ACKs required\n",c->name);return true;
  }
  void stop(uint32_t now){
    if(!busy())return;
    stopped=true;
    if(state==SendOff||state==OffAck)return;
    masterIO.cancelPending();state=SendOff;at=now;
  }
  void fault(uint32_t now){
    const uint8_t node=component->node;state=Idle;
    masterIO.quarantine(node,now);engine.fail("FAULT: unloaded channel test ACK/readiness failure",now);
  }
  void tick(uint32_t now){
    if(!busy())return;
    if(engine.faulted()||!masterIO.ready(component->node,now)){fault(now);return;}
    if(state==Timed){if(uint32_t(now-at)>=1000){state=SendOff;at=now;}else return;}
    if(state==OnAck||state==OffAck){
      const auto ack=masterIO.acknowledgement(component->node,ticket);
      if(ack==demo::AckResult::Failed||uint32_t(now-at)>=2000){fault(now);return;}
      if(ack==demo::AckResult::Pending)return;
      if(state==OffAck){state=Idle;Serial.println(stopped?"CHANNEL TEST STOPPED: OFF acknowledged":"CHANNEL TEST COMPLETE: OFF acknowledged");return;}
      state=Timed;at=now;return;
    }
    const auto sent=masterIO.relay(component->node,component->channel,state==SendOn,ticket);
    if(sent==demo::SendResult::Sent){state=state==SendOn?OnAck:OffAck;at=now;}
    else if(sent==demo::SendResult::Failed||uint32_t(now-at)>=2000)fault(now);
  }
};
ChannelTest channelTest;
inline void requestStop(){recovering=false;if(channelTest.busy())channelTest.stop(millis());else engine.stop(millis());}
