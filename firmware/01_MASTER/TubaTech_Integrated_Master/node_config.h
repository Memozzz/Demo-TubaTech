#pragma once
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
namespace nodecfg {
constexpr uint8_t MASTER_MAC[6]={0x38,0x18,0x2B,0x8A,0x9D,0x30};
struct Position {const char *name;uint8_t mac[6];bool installed;};
inline Position positions[]={
 {"SECONDARY / SLAVE 1",{0x68,0x25,0xDD,0xF1,0xA3,0x78},true},
 {"TETRIARY / SLAVE 2",{0x58,0x2A,0xBD,0xD3,0x08,0xF4},true},
 {"SLAVE 3",{0x78,0x1C,0x3C,0xA5,0x50,0x80},true},
 // Zero storage is an UNPROVISIONED sentinel, never a radio peer/address.
 {"SLAVE 4",{0x38,0x3E,0x51,0xD4,0x6A,0x30},true}
};
inline std::string macText(const uint8_t *mac){char s[18];std::snprintf(s,sizeof(s),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);return s;}
inline bool parseMac(const std::string &s,uint8_t (&mac)[6]){
 if(s.size()!=17)return false;
 auto digit=[](char c){if(c>='0'&&c<='9')return int(c-'0');if(c>='a'&&c<='f')return int(c-'a'+10);if(c>='A'&&c<='F')return int(c-'A'+10);return -1;};
 for(unsigned i=0;i<6;++i){size_t p=i*3;if(digit(s[p])<0||digit(s[p+1])<0||(i<5&&s[p+2]!=':'))return false;mac[i]=uint8_t(digit(s[p])*16+digit(s[p+1]));}
 bool nonzero=false;for(auto b:mac)nonzero|=b!=0;return nonzero&&!(mac[0]&1);
}
inline bool validSlave4(const uint8_t (&mac)[6]){
 if(std::memcmp(mac,MASTER_MAC,6)==0)return false;
 for(unsigned i=0;i<3;++i)if(std::memcmp(mac,positions[i].mac,6)==0)return false;
 return true;
}
} // namespace nodecfg
