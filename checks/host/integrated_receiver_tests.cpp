#include <cassert>
#if TEST_NODE==1
#include "../../firmware/02_SLAVE1/TubaTech_Integrated_Slave1/TubaTech_Integrated_Slave1.ino"
#elif TEST_NODE==2
#include "../../firmware/03_SLAVE2/TubaTech_Integrated_Slave2/TubaTech_Integrated_Slave2.ino"
#elif TEST_NODE==3
#include "../../firmware/04_SLAVE3/TubaTech_Integrated_Slave3/TubaTech_Integrated_Slave3.ino"
#else
#include "../../firmware/05_SLAVE4/TubaTech_Integrated_Slave4/TubaTech_Integrated_Slave4.ino"
#endif
static Packet packet(uint8_t type,uint32_t session,uint32_t ticket,uint8_t channel=0,uint8_t value=0){Packet p(MAGIC,session,ticket,type,channel,value,0);p.owner=NODE_ID;return p;}
static void inject(const Packet &p){esp_now_recv_info_t info={integrated::macs[0]};onReceive(&info,reinterpret_cast<const uint8_t*>(&p),sizeof(p));loop();}
static Packet last(){assert(!mockSends.empty());Packet p;memcpy(&p,mockSends.back().data.data(),sizeof(p));return p;}
static void off(){assert(bank.commanded==0);for(uint8_t ch=0;ch<14;++ch)if(integrated::bankMask(NODE_ID)&(1U<<ch))assert(mockPinLevels[demo::outputPins[ch]]==HIGH);}
static void handshake(uint32_t session,uint32_t ticket){
  inject(packet(SYNC,session,ticket));auto c=last();assert(c.type==CHALLENGE);off();
  auto p=packet(CONFIRM,session,ticket,c.channel,c.value);p.stateMask=c.stateMask;inject(p);assert(last().type==ACK&&bank.linked);
}
int main(){
  mockNow=100;setup();assert(!identityOK&&mockPeers.empty());
  char mac[18];const auto *m=integrated::macs[NODE_ID];snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);
  // Reproduce physical startup: the Wi-Fi interface reports zero while the
  // factory station MAC is already available. Identity must remain guarded.
  mockStationMac="00:00:00:00:00:00";mockFactoryMacEnabled=true;memcpy(mockFactoryMac,m,6);
  mockFactoryMacFailure=true;assert(!integrated::stationIdentity(NODE_ID));mockFactoryMacFailure=false;
  assert(!integrated::stationIdentity(NODE_ID==1?2:1));
  setup();assert(identityOK&&mockPeers.size()==1);off();assert(!bank.enabled);
  mockFactoryMacEnabled=false;mockStationMac=mac;
  inject(packet(SET_RELAY,1,1,0,1));assert(last().type==REJECT);off();handshake(2,1);
  assert(bank.enabled==bool(TUBATECH_MASTER_ONLY_UNLOADED_ARM));off();
  // An old/unverified session cannot acquire the optional readiness grant.
  assert(bank.arm());
  uint32_t ticket=2;
  for(uint8_t ch=0;ch<14;++ch)if(integrated::bankMask(NODE_ID)&(1U<<ch)){
    inject(packet(SET_RELAY,2,ticket++,ch,1));assert(last().type==ACK&&last().stateMask==uint16_t(1U<<ch));assert(mockPinLevels[demo::outputPins[ch]]==LOW);
    inject(packet(SET_RELAY,2,ticket++,ch,0));off();
  }
  auto on=packet(SET_RELAY,2,ticket++,0,1);inject(on);auto at=bank.lastHeartbeat;mockNow+=2000;inject(on);assert(bank.lastHeartbeat==at);
  mockNow=at+2501;loop();off();assert(!bank.enabled&&!bank.linked);inject(packet(SET_RELAY,2,ticket++,0,1));off();
  handshake(3,1);assert(bank.arm());inject(packet(SET_RELAY,3,2,0,1));auto altered=packet(SET_RELAY,3,2,0,0);inject(altered);off();assert(last().type==REJECT);
  handshake(4,1);assert(bank.arm());inject(packet(SET_RELAY,4,2,0,1));inject(packet(SET_RELAY,999,3,0,1));off();assert(!bank.guard.armed);
  handshake(5,1);assert(bank.arm());inject(packet(SET_RELAY,5,2,14,1));off();assert(last().type==REJECT);
  handshake(6,1);assert(bank.arm());auto good=packet(SET_RELAY,6,2,0,1);size_t count=mockSends.size();esp_now_recv_info_t bad={integrated::macs[NODE_ID]};onReceive(&bad,reinterpret_cast<const uint8_t*>(&good),sizeof(good));loop();off();assert(mockSends.size()==count);
  auto old=good;old.version=2;esp_now_recv_info_t master={integrated::macs[0]};onReceive(&master,reinterpret_cast<const uint8_t*>(&old),sizeof(old));onReceive(&master,reinterpret_cast<const uint8_t*>(&good),16);loop();off();assert(mockSends.size()==count);
  inject(good);rxOverflow=true;loop();off();assert(!bank.enabled&&!bank.guard.armed);
  printf("PASS: actual integrated Slave %u identity, every owned GPIO, extended ACK masks, source/version/length/ownership gates, local arming, replay, watchdog and overflow OFF\n",NODE_ID);
}
