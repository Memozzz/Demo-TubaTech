#pragma once
using esp_err_t=int;
constexpr int ESP_OK=0, WIFI_SECOND_CHAN_NONE=0;
inline int esp_wifi_set_channel(int,int) { return ESP_OK; }
