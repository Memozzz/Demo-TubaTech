#pragma once
#include <esp_mac.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "board_map.h"
namespace integrated {
inline bool stationIdentity(uint8_t owner){
  if(owner>4)return false;
  // WiFi.macAddress() can still be all zeros immediately after WIFI_STA starts
  // in core 3.3.12. Read the physical station identity independently of startup.
  uint8_t actual[6]={};
  return esp_read_mac(actual,ESP_MAC_WIFI_STA)==ESP_OK &&
         std::memcmp(actual,macs[owner],sizeof(actual))==0;
}
inline std::string stationMacText(){
  uint8_t mac[6]={};
  if(esp_read_mac(mac,ESP_MAC_WIFI_STA)!=ESP_OK)return "UNAVAILABLE";
  char text[18];
  std::snprintf(text,sizeof(text),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return text;
}
}
