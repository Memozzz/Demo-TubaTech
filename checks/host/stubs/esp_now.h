#pragma once
#include "Arduino.h"
#include "esp_wifi.h"
struct esp_now_recv_info_t { const uint8_t *src_addr; };
struct esp_now_peer_info_t { uint8_t peer_addr[6]; uint8_t channel; bool encrypt; };
struct MockSend { uint8_t mac[6]; std::vector<uint8_t> data; };
inline std::vector<MockSend> mockSends;
inline std::vector<esp_now_peer_info_t> mockPeers;
inline bool mockSendFailure=false;
inline int esp_now_del_peer(const uint8_t*) {return ESP_OK;}
inline int esp_now_init() { return ESP_OK; }
inline int esp_now_add_peer(const esp_now_peer_info_t *peer) { mockPeers.push_back(*peer);return ESP_OK; }
inline int esp_now_register_recv_cb(void (*)(const esp_now_recv_info_t*,const uint8_t*,int)) { return ESP_OK; }
inline int esp_now_send(const uint8_t *mac,const uint8_t *data,size_t size) {
  if(mockSendFailure) return -1;
  MockSend sent; memcpy(sent.mac,mac,6); sent.data.assign(data,data+size); mockSends.push_back(sent); return ESP_OK;
}
