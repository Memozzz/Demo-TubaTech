#pragma once

// Read-only transport diagnostics. Radio delivery NEVER satisfies a sequence
// ACK, changes readiness, arms a bank or activates an output.
namespace radio_diag {
#ifdef TUBATECH_HOST_TEST
inline void observeRx(const esp_now_recv_info_t *, const uint8_t *, int) {}
inline void validRx() {}
inline void sendResult(int, int) {}
inline bool begin() { return true; }
inline void report() {}
#else
inline volatile uint32_t rxCount=0, rxValid=0, rxUnknown=0, rxWrongLength=0;
inline volatile int lastLength=0;
inline volatile uint32_t txDelivered[4]={}, txFailed[4]={};
inline volatile int lastSendError[4]={};
inline uint8_t lastSource[6]={}, lastTxSource[6]={};
inline volatile int lastTxInterface=-1;
inline volatile uint32_t lastMagic=0;

inline int peerIndex(const uint8_t *mac) {
  if(mac) for(int i=0;i<4;++i)
    if(!memcmp(mac,integrated::macs[i+1],6)) return i;
  return -1;
}
inline void observeRx(const esp_now_recv_info_t *info,const uint8_t *data,int len) {
  ++rxCount; lastLength=len;
  if(info&&info->src_addr) memcpy(lastSource,info->src_addr,6);
  if(!info||peerIndex(info->src_addr)<0) ++rxUnknown;
  if(len!=sizeof(Packet)) ++rxWrongLength;
  uint32_t magic=0;
  if(data&&len>=4) memcpy(&magic,data,4);
  lastMagic=magic;
}
inline void validRx() { ++rxValid; }
inline void sendResult(int peer,int result) {
  if(peer>=0&&peer<4) lastSendError[peer]=result;
}
inline void onSend(const esp_now_send_info_t *info,esp_now_send_status_t status) {
  if(!info) return;
  int peer=peerIndex(info->des_addr);
  if(peer>=0) {
    if(status==ESP_NOW_SEND_SUCCESS) ++txDelivered[peer];
    else ++txFailed[peer];
  }
  if(info->src_addr) memcpy(lastTxSource,info->src_addr,6);
  lastTxInterface=int(info->ifidx);
}
inline bool begin() { return esp_now_register_send_cb(onSend)==ESP_OK; }
inline void printMac(const uint8_t *mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}
inline void report() {
  uint8_t primary=0,sta[6]={};
  wifi_second_chan_t secondary=WIFI_SECOND_CHAN_NONE;
  wifi_mode_t mode=WIFI_MODE_NULL;
  wifi_ps_type_t sleep=WIFI_PS_NONE;
  int channelResult=esp_wifi_get_channel(&primary,&secondary);
  int macResult=esp_wifi_get_mac(WIFI_IF_STA,sta);
  int modeResult=esp_wifi_get_mode(&mode);
  int sleepResult=esp_wifi_get_ps(&sleep);
  Serial.printf("RADIO actual_channel=%u channel_rc=%d mode=%d mode_rc=%d ps=%d ps_rc=%d sta_mac=",
    primary,channelResult,int(mode),modeResult,int(sleep),sleepResult);
  if(macResult==ESP_OK) printMac(sta); else Serial.print("UNAVAILABLE");
  Serial.printf(" mac_rc=%d packet_bytes=%u magic=0x%08lX\n",macResult,unsigned(sizeof(Packet)),(unsigned long)MAGIC);
  Serial.printf("RADIO RX total=%lu valid=%lu unknown_source=%lu wrong_length=%lu last_len=%d last_magic=0x%08lX last_src=",
    (unsigned long)rxCount,(unsigned long)rxValid,(unsigned long)rxUnknown,(unsigned long)rxWrongLength,int(lastLength),(unsigned long)lastMagic);
  printMac(lastSource); Serial.println();
  Serial.print("RADIO TX last_src="); printMac(lastTxSource);
  Serial.printf(" ifidx=%d (delivery counts are NOT output ACKs)\n",int(lastTxInterface));
  for(int i=0;i<4;++i) Serial.printf("RADIO peer=%d delivered=%lu failed=%lu send_rc=%d\n",i+1,
    (unsigned long)txDelivered[i],(unsigned long)txFailed[i],int(lastSendError[i]));
}
#endif
} // namespace radio_diag
