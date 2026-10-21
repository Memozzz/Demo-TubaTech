#pragma once
#include <stdint.h>
// V3 fixed-map unloaded bench protocol. Never compatible with legacy 16 bytes.
constexpr uint32_t MAGIC=0x54544233;
enum PacketType:uint8_t {HEARTBEAT=1,SET_RELAY=2,ALL_OFF=3,ACK=4,SYNC=5,CHALLENGE=6,CONFIRM=7,REJECT=8};
struct Packet {
  uint32_t magic=0,session=0,sequence=0;
  uint8_t type=0,channel=0,value=0,version=3;
  uint16_t stateMask=0;
  uint8_t owner=255,flags=0;
  Packet()=default;
  Packet(uint32_t m,uint32_t s,uint32_t q,uint8_t t,uint8_t c,uint8_t v,uint16_t mask):
    magic(m),session(s),sequence(q),type(t),channel(c),value(v),stateMask(mask){}
};
static_assert(sizeof(Packet)==20,"V3 packet must be exactly 20 bytes");
inline bool validEnvelope(const Packet &p){return p.magic==MAGIC&&p.version==3&&p.owner<=4&&p.flags<=1;}
