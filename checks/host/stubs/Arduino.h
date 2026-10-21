#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <cctype>
inline uint32_t mockNow = 100;
inline uint32_t mockRandom = 123456;
inline uint32_t millis() { return mockNow; }
inline uint32_t esp_random() { return ++mockRandom; }
constexpr int HIGH=1, LOW=0, OUTPUT=1;
inline int mockPinLevels[40] = {}, mockPinModes[40] = {};
inline void digitalWrite(uint8_t pin,int value) { mockPinLevels[pin]=value; }
inline void pinMode(uint8_t pin,int mode) { mockPinModes[pin]=mode; }
struct String {
  std::string text;
  String(const char *s="") : text(s) {}
  const char *c_str() const { return text.c_str(); }
  size_t length() const { return text.size(); }
  void trim() {
    const auto first=text.find_first_not_of(" \r\n\t");
    if(first==std::string::npos) { text.clear(); return; }
    text=text.substr(first,text.find_last_not_of(" \r\n\t")-first+1);
  }
  void toUpperCase() { for(auto &c:text) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
  bool startsWith(const char *s) const { return text.rfind(s,0)==0; }
  String substring(size_t n) const { return String(text.substr(n).c_str()); }
  bool operator==(const char *s) const { return text==s; }
  String &operator+=(char c) { text+=c; return *this; }
};
struct SerialStub {
  std::string output;
  void begin(int) {}
  void println(const char *s="") { output+=s; output+='\n'; }
  void printf(const char *format,...) {
    char buffer[1024]; va_list args; va_start(args,format);
    vsnprintf(buffer,sizeof(buffer),format,args); va_end(args); output+=buffer;
  }
  int available() { return 0; }
  int read() { return 0; }
};
inline SerialStub Serial;
struct MockQueue { size_t size; std::deque<std::vector<uint8_t>> data; };
using QueueHandle_t=MockQueue*;
constexpr int pdTRUE=1;
inline QueueHandle_t xQueueCreate(int,size_t size) { return new MockQueue{size,{}}; }
inline int xQueueSend(QueueHandle_t q,const void *data,int) {
  const auto p=static_cast<const uint8_t*>(data); q->data.emplace_back(p,p+q->size); return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q,void *data,int) {
  if(q->data.empty()) return 0; memcpy(data,q->data.front().data(),q->size); q->data.pop_front(); return pdTRUE;
}
