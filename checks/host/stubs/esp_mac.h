#pragma once
#include <cstdio>
#include <cstring>
#include "esp_wifi.h"
#include "WiFi.h"
constexpr int ESP_MAC_WIFI_STA=0;
inline bool mockFactoryMacEnabled=false, mockFactoryMacFailure=false;
inline unsigned char mockFactoryMac[6]={};
inline esp_err_t esp_read_mac(unsigned char *mac,int type){
  if(type!=ESP_MAC_WIFI_STA||mockFactoryMacFailure)return -1;
  if(mockFactoryMacEnabled){std::memcpy(mac,mockFactoryMac,6);return ESP_OK;}
  unsigned bytes[6]={};
  if(std::sscanf(mockStationMac.c_str(),"%2x:%2x:%2x:%2x:%2x:%2x",&bytes[0],&bytes[1],&bytes[2],&bytes[3],&bytes[4],&bytes[5])!=6)return -1;
  for(unsigned i=0;i<6;++i)mac[i]=static_cast<unsigned char>(bytes[i]);
  return ESP_OK;
}
