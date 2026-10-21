#pragma once
constexpr int WIFI_STA=1;
inline String mockStationMac("02:00:00:00:00:01");
struct WiFiStub { void mode(int) {} void disconnect() {} String macAddress() {return mockStationMac;} };
inline WiFiStub WiFi;
