#pragma once
// Generated from definitions/control.json; edit the authoritative JSON.
#include <stddef.h>
#include <stdint.h>
namespace demo {
enum class Kind : uint8_t { Valve, Pump };
struct Component { const char *name; uint8_t node; uint8_t channel; Kind kind; const char *category; int gpio; uint8_t relayInput; bool installed; bool mappingVerified; uint8_t driver; uint8_t address; uint8_t driverChannel; uint8_t module; bool driverAvailable; };
constexpr uint8_t UNMAPPED=255, DEFAULT_BENCH_MASK=31;
// Indices 0..3 = Slaves 1..4; index 4 = MASTER-local, not a fifth slave.
constexpr size_t ACTIVE_NODES=5, PLANNED_NODES=5, RELAYS_PER_NODE=14, MAX_OUTPUTS=14;
constexpr bool REAL_HARDWARE_ENABLED=false;
constexpr uint8_t outputPins[14]={16,17,18,19,21,22,23,25,26,27,32,33,13,14};
constexpr uint8_t moduleCapacity[10]={16,16,16,4,4,4,4,2,2,2};
constexpr Component components[]={
 {"V1",3,3,Kind::Valve,"transfer/intake valve",19,1,false,false,1,0,3,1,true},
 {"V2",0,0,Kind::Valve,"transfer/intake valve",16,1,false,false,1,0,0,4,true},
 {"V3",0,1,Kind::Valve,"transfer/intake valve",17,2,false,false,1,0,1,4,true},
 {"V4",4,0,Kind::Valve,"transfer/intake valve",16,2,false,false,1,0,0,1,true},
 {"V5",4,1,Kind::Valve,"transfer/intake valve",17,3,false,false,1,0,1,1,true},
 {"V6",3,4,Kind::Valve,"transfer/intake valve",21,4,false,false,1,0,4,1,true},
 {"V7",0,12,Kind::Valve,"transfer/intake valve",13,5,false,false,1,0,12,1,true},
 {"V8",3,5,Kind::Valve,"transfer/intake valve",22,6,false,false,1,0,5,1,true},
 {"V9",1,12,Kind::Valve,"transfer/intake valve",13,7,false,false,1,0,12,1,true},
 {"V10",1,13,Kind::Valve,"transfer/intake valve",14,8,false,false,1,0,13,1,true},
 {"CV1",4,2,Kind::Valve,"cleaning valve",18,9,false,false,1,0,2,1,true},
 {"CV2",4,3,Kind::Valve,"cleaning valve",19,10,false,false,1,0,3,1,true},
 {"CV3",4,4,Kind::Valve,"cleaning valve",21,11,false,false,1,0,4,1,true},
 {"CV4",0,13,Kind::Valve,"cleaning valve",14,12,false,false,1,0,13,1,true},
 {"CV5",4,12,Kind::Valve,"cleaning valve",13,13,false,false,1,0,12,1,true},
 {"CV6",4,13,Kind::Valve,"cleaning valve",14,14,false,false,1,0,13,1,true},
 {"CV7",0,2,Kind::Valve,"cleaning valve",18,15,false,false,1,0,2,1,true},
 {"CV8",0,3,Kind::Valve,"cleaning valve",19,16,false,false,1,0,3,1,true},
 {"CV9",0,4,Kind::Valve,"cleaning valve",21,1,false,false,1,0,4,2,true},
 {"CV10",3,8,Kind::Valve,"cleaning valve",26,2,false,false,1,0,8,2,true},
 {"DV1",3,9,Kind::Valve,"drain valve",27,3,false,false,1,0,9,2,true},
 {"DV2",0,9,Kind::Valve,"drain valve",27,4,false,false,1,0,9,2,true},
 {"DV3",0,10,Kind::Valve,"drain valve",32,5,false,false,1,0,10,2,true},
 {"DV4",1,1,Kind::Valve,"drain valve",17,6,false,false,1,0,1,2,true},
 {"DV5",1,2,Kind::Valve,"drain valve",18,7,false,false,1,0,2,2,true},
 {"FV1",4,5,Kind::Valve,"filter valve",22,8,false,false,1,0,5,2,true},
 {"FV2",4,6,Kind::Valve,"filter valve",23,9,false,false,1,0,6,2,true},
 {"FV3",0,5,Kind::Valve,"filter valve",22,10,false,false,1,0,5,2,true},
 {"FV4",0,6,Kind::Valve,"filter valve",23,11,false,false,1,0,6,2,true},
 {"FV5",0,7,Kind::Valve,"filter valve",25,12,false,false,1,0,7,2,true},
 {"FV6",0,8,Kind::Valve,"filter valve",26,13,false,false,1,0,8,2,true},
 {"PUMP_1",1,0,Kind::Pump,"wine pump",16,1,false,false,1,0,0,5,true},
 {"PUMP_2",3,10,Kind::Pump,"wine pump",32,14,false,false,1,0,10,2,true},
 {"PUMP_3",0,11,Kind::Pump,"wine pump",33,15,false,false,1,0,11,2,true},
 {"PUMP_4",1,3,Kind::Pump,"wine pump",19,16,false,false,1,0,3,2,true},
 {"PUMP_5",4,11,Kind::Pump,"wine pump",33,1,false,false,1,0,11,3,true},
 {"VV1",1,4,Kind::Valve,"vinegar valve",21,2,false,false,1,0,4,3,true},
 {"VV2",1,5,Kind::Valve,"vinegar valve",22,3,false,false,1,0,5,3,true},
 {"VV3",1,6,Kind::Valve,"vinegar valve",23,4,false,false,1,0,6,3,true},
 {"VV4",1,7,Kind::Valve,"vinegar valve",25,5,false,false,1,0,7,3,true},
 {"VV5",3,6,Kind::Valve,"vinegar valve",23,6,false,false,1,0,6,3,true},
 {"VV6",2,12,Kind::Valve,"vinegar valve",13,7,false,false,1,0,12,3,true},
 {"VV7",3,7,Kind::Valve,"vinegar valve",25,8,false,false,1,0,7,3,true},
 {"VV8",3,12,Kind::Valve,"vinegar valve",13,9,false,false,1,0,12,3,true},
 {"CVV1",1,8,Kind::Valve,"vinegar cleaning valve",26,10,false,false,1,0,8,3,true},
 {"CVV2",2,1,Kind::Valve,"vinegar cleaning valve",17,11,false,false,1,0,1,3,true},
 {"CVV3",2,3,Kind::Valve,"vinegar cleaning valve",19,12,false,false,1,0,3,3,true},
 {"CVV4",2,5,Kind::Valve,"vinegar cleaning valve",22,13,false,false,1,0,5,3,true},
 {"CVV5",2,6,Kind::Valve,"vinegar cleaning valve",23,14,false,false,1,0,6,3,true},
 {"CVV6",3,13,Kind::Valve,"vinegar cleaning valve",14,15,false,false,1,0,13,3,true},
 {"CVV7",3,1,Kind::Valve,"vinegar cleaning valve",17,16,false,false,1,0,1,3,true},
 {"DVV1",1,9,Kind::Valve,"vinegar drain valve",27,3,false,false,1,0,9,4,true},
 {"DVV2",2,2,Kind::Valve,"vinegar drain valve",18,4,false,false,1,0,2,4,true},
 {"DVV3",2,4,Kind::Valve,"vinegar drain valve",21,2,false,false,1,0,4,5,true},
 {"DVV4",2,7,Kind::Valve,"vinegar drain valve",25,3,false,false,1,0,7,5,true},
 {"DVV5",3,0,Kind::Valve,"vinegar drain valve",16,4,false,false,1,0,0,5,true},
 {"DVV6",3,2,Kind::Valve,"vinegar drain valve",18,1,false,false,1,0,2,6,true},
 {"VPUMP_1",1,11,Kind::Pump,"vinegar pump",33,2,false,false,1,0,11,6,true},
 {"VPUMP_2",2,10,Kind::Pump,"vinegar pump",32,3,false,false,1,0,10,6,true},
 {"VPUMP_3",2,11,Kind::Pump,"vinegar pump",33,4,false,false,1,0,11,6,true},
 {"VPUMP_4",2,8,Kind::Pump,"vinegar pump",26,1,false,false,1,0,8,7,true},
 {"AV1",2,13,Kind::Valve,"aeration valve",14,2,false,false,1,0,13,7,true},
 {"AV2",2,0,Kind::Valve,"aeration valve",16,3,false,false,1,0,0,7,true},
 {"HP_PUMP",4,10,Kind::Pump,"shared cleaning pump",32,4,false,false,1,0,10,7,true},
 {"AP",2,9,Kind::Pump,"aeration pump",27,1,false,false,1,0,9,8,true},
 {"DCP1",1,10,Kind::Pump,"dosing actuator, hardware type unvalidated",32,2,false,false,1,0,10,8,true},
};
constexpr size_t COMPONENT_COUNT=sizeof(components)/sizeof(components[0]);
constexpr bool sameName(const char *a,const char *b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
constexpr bool mapped(const Component &c){return c.node!=UNMAPPED&&c.channel!=UNMAPPED;}
constexpr bool validAssignment(const Component &c){return c.node<5&&c.channel<14&&c.driver==1&&c.gpio==outputPins[c.channel]&&c.module>=1&&c.module<=10&&c.relayInput>=1&&c.relayInput<=moduleCapacity[c.module-1]&&!(c.node==4&&(c.gpio==25||c.gpio==26||c.gpio==27));}
constexpr bool realOutputAvailable(const Component &){return false;}
constexpr const Component *find(const char *name,const Component *entries=components,size_t count=COMPONENT_COUNT){for(size_t i=0;i<count;++i)if(sameName(name,entries[i].name))return &entries[i];return nullptr;}
constexpr bool validRegistry(const Component *entries,size_t count,size_t activeNodes){
 if(!entries||!count||!activeNodes||activeNodes>5)return false;
 for(size_t i=0;i<count;++i){const auto &c=entries[i];if(!validAssignment(c)||c.node>=activeNodes)return false;
  for(size_t j=0;j<i;++j)if(sameName(c.name,entries[j].name)||(c.node==entries[j].node&&c.channel==entries[j].channel)||(c.module==entries[j].module&&c.relayInput==entries[j].relayInput))return false;}
 return true;
}
static_assert(COMPONENT_COUNT==66&&validRegistry(components,COMPONENT_COUNT,5),"Invalid proposed fixed map");
}
