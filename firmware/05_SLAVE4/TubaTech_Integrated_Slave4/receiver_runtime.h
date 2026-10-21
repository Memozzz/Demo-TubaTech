#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "relay_bank.h"
#include "station_identity.h"
integrated::RelayBank bank(NODE_ID);
QueueHandle_t rxQueue=nullptr;
volatile bool rxOverflow=false;
bool identityOK=false;
String serialLine;
bool serialOverflow=false;
inline void printReceiverRadio(){
#ifdef ARDUINO_ARCH_ESP32
  uint8_t primary=0,mac[6]={};wifi_second_chan_t second=WIFI_SECOND_CHAN_NONE;
  wifi_ps_type_t ps=WIFI_PS_NONE;
  int channelResult=esp_wifi_get_channel(&primary,&second);
  int macResult=esp_wifi_get_mac(WIFI_IF_STA,mac);
  int psResult=esp_wifi_get_ps(&ps);
  Serial.printf("RADIO node=%u actual_channel=%u channel_rc=%d ps=%d ps_rc=%d sta_mac=%02X:%02X:%02X:%02X:%02X:%02X mac_rc=%d packet_bytes=%u\n",
    NODE_ID,primary,channelResult,int(ps),psResult,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],macResult,unsigned(sizeof(Packet)));
#endif
}
inline void onReceive(const esp_now_recv_info_t *info,const uint8_t *data,int len){
  if(!identityOK||!info||!data||len!=sizeof(Packet)||!rxQueue||memcmp(info->src_addr,integrated::macs[0],6))return;
  Packet p;memcpy(&p,data,sizeof(p));
  if(!validEnvelope(p)||p.owner!=NODE_ID)return;
  if(xQueueSend(rxQueue,&p,0)!=pdTRUE)rxOverflow=true;
}
inline void receiverCommand(String line){
  line.trim();line.toUpperCase();
  if(line=="RADIO"){printReceiverRadio();return;}
  if(line=="OUTPUTS ARM UNLOADED"||line=="OUTPUTS ARM GPIO_ONLY"){
    Serial.println(identityOK&&bank.arm()?"UNLOADED GPIO outputs armed; no contact loads permitted":"ARM blocked: identity/session must be verified and outputs OFF");return;
  }
  if(line=="OUTPUTS DISARM"||line=="STOP"){bank.disable();Serial.println("OFF / disarmed; fresh handshake required");return;}
  if(line=="STATUS"||line=="IDENTITY"){
    Serial.printf("SLAVE %u | station MAC %s | identity=%s session=%s outputs=%s mask=0x%04X REAL=DISABLED\n",NODE_ID,integrated::stationMacText().c_str(),identityOK?"MATCH":"MISMATCH",bank.linked?"VERIFIED":"UNVERIFIED",bank.enabled?"UNLOADED ARMED":"DISARMED",bank.commanded);return;
  }
  if(line=="PINS"){
    for(const auto &c:demo::components)if(c.node==NODE_ID-1)Serial.printf("%s CH%u GPIO%d R%02u IN%u\n",c.name,c.channel,c.gpio,c.module,c.relayInput);return;
  }
  Serial.println("Commands: IDENTITY, STATUS, PINS, OUTPUTS ARM GPIO_ONLY (no relay inputs connected), OUTPUTS ARM UNLOADED, OUTPUTS DISARM, STOP. BOOT never starts a sequence.");
}
void setup(){
  Serial.begin(115200);
  identityOK=integrated::stationIdentity(NODE_ID);
  receiverCommand("IDENTITY");
  if(!identityOK){Serial.println("Wrong physical board: radio/output activation locked out");return;}
  bank.begin();
  // Outputs are OFF before radio startup. STA.begin(false) waits for the
  // driver's START event without associating with a router. This wait is
  // setup-only: no active sequence/heartbeat processing is blocked.
#ifdef ARDUINO_ARCH_ESP32
  WiFi.setSleep(false);WiFi.setAutoReconnect(false);
  if(!WiFi.mode(WIFI_STA)||!WiFi.STA.begin(false)){
    bank.disable();Serial.println("FAULT: STA startup failed; outputs OFF");return;
  }
#else
  WiFi.mode(WIFI_STA);
#endif
  WiFi.disconnect();
  rxQueue=xQueueCreate(20,sizeof(Packet));
  if(!rxQueue||esp_wifi_set_channel(6,WIFI_SECOND_CHAN_NONE)!=ESP_OK||esp_now_init()!=ESP_OK){bank.disable();return;}
#ifdef ARDUINO_ARCH_ESP32
  uint8_t actualMac[6]={},actualChannel=0;wifi_second_chan_t second=WIFI_SECOND_CHAN_NONE;
  if(esp_wifi_set_ps(WIFI_PS_NONE)!=ESP_OK||esp_wifi_get_mac(WIFI_IF_STA,actualMac)!=ESP_OK||
     memcmp(actualMac,integrated::macs[NODE_ID],6)||esp_wifi_get_channel(&actualChannel,&second)!=ESP_OK||actualChannel!=6){
    bank.disable();printReceiverRadio();Serial.println("FAULT: radio identity/channel verification failed; outputs OFF");return;
  }
#endif
  if(esp_now_register_recv_cb(onReceive)!=ESP_OK){bank.disable();return;}
  esp_now_peer_info_t peer={};memcpy(peer.peer_addr,integrated::macs[0],6);peer.channel=6;peer.encrypt=false;
#ifdef ARDUINO_ARCH_ESP32
  peer.ifidx=WIFI_IF_STA;
#endif
  if(esp_now_add_peer(&peer)!=ESP_OK){bank.disable();return;}
  printReceiverRadio();
  Serial.println(TUBATECH_MASTER_ONLY_UNLOADED_ARM?
    "Communication ready on channel 6; unloaded commands require MASTER bench arm + confirmed profile. Boot outputs OFF.":
    "Communication ready on channel 6; ON disabled until local OUTPUTS ARM UNLOADED.");
}
void loop(){
  if(!identityOK)return;
  bool wasLinked=bank.linked;bank.tick(millis());
  if(wasLinked&&!bank.linked)Serial.println("FAULT: MASTER LOST - GPIO outputs OFF, disarmed; fresh handshake required");
  if(rxOverflow){rxOverflow=false;bank.disable();Serial.println("FAULT: RX overflow; OFF and disarmed");}
  Packet p;
  for(unsigned count=0;count<20&&rxQueue&&xQueueReceive(rxQueue,&p,0)==pdTRUE;++count){
    bool verifiedBefore=bank.linked;Packet reply=bank.receive(p,millis());
    if(!verifiedBefore&&bank.linked)Serial.println(TUBATECH_MASTER_ONLY_UNLOADED_ARM?
      "SESSION VERIFIED: fresh OFF handshake; outputs OFF; bench authorization remains on MASTER":
      "SESSION VERIFIED: fresh OFF handshake; GPIO ON remains locally gated");
    esp_now_send(integrated::macs[0],reinterpret_cast<const uint8_t*>(&reply),sizeof(reply));
  }
  for(unsigned count=0;count<64&&Serial.available();++count){
    char c=char(Serial.read());
    if(c=='\n'||c=='\r'){
      if(serialOverflow)Serial.println("Line too long; rejected");else if(serialLine.length())receiverCommand(serialLine);
      serialLine="";serialOverflow=false;
    }else if(serialLine.length()<63)serialLine+=c;else serialOverflow=true;
  }
}
