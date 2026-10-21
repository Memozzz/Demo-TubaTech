#pragma once
#include "component_registry.h"
namespace integrated {
constexpr uint8_t macs[5][6]={
 {0x38,0x18,0x2B,0x8A,0x9D,0x30},
 {0x68,0x25,0xDD,0xF1,0xA3,0x78},
 {0x58,0x2A,0xBD,0xD3,0x08,0xF4},
 {0x78,0x1C,0x3C,0xA5,0x50,0x80},
 {0x38,0x3E,0x51,0xD4,0x6A,0x30},
};
constexpr uint8_t indexForOwner(uint8_t owner){return owner?uint8_t(owner-1):4;}
constexpr uint8_t ownerForIndex(uint8_t index){return index==4?0:uint8_t(index+1);}
constexpr uint16_t bankMask(uint8_t owner){uint16_t m=0;for(const auto &c:demo::components)if(c.node==indexForOwner(owner))m|=uint16_t(1U<<c.channel);return m;}
}
