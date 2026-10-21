#pragma once
#include "component_registry.h"
// Generated from definitions/control.json; edit the authoritative JSON.
namespace demo {
enum class DefinitionStatus { RelayDemo, SensorsPending, Ambiguous, Reserved };
enum class ActionKind { SetOutput, WaitForSensor };
struct Action {const char *component; bool on; ActionKind kind=ActionKind::SetOutput;};
struct Phase {const char *name; const Action *actions; size_t count; uint32_t defaultMs;};
struct FunctionDefinition {const char *id; const char *title; const Phase *phases; size_t phaseCount; DefinitionStatus status; const char *note;};
constexpr Action a0_0[]={{"V1",true}};
constexpr Phase p0[]={{"Wine pour initial",a0_0,sizeof(a0_0)/sizeof(Action),30000}};
constexpr Action a1_0[]={{"V2",true},{"V3",true},{"PUMP_1",true}};
constexpr Phase p1[]={{"Wine initial -> PC1",a1_0,sizeof(a1_0)/sizeof(Action),30000}};
constexpr Action a2_0[]={{"CV1",true},{"DV1",true},{"HP_PUMP",true},{"PUMP_5",true}};
constexpr Phase p2[]={{"Wine clean initial",a2_0,sizeof(a2_0)/sizeof(Action),30000}};
constexpr Action a3_0[]={{"CV1",false},{"DV1",false},{"V2",false},{"CV2",true},{"FV2",true},{"HP_PUMP",true},{"PUMP_1",true},{"PUMP_5",true}};
constexpr Action a3_1[]={{"PUMP_1",false},{"CV3",true},{"FV1",true}};
constexpr Phase p3[]={{"First flush",a3_0,sizeof(a3_0)/sizeof(Action),15000},{"Reverse backflush",a3_1,sizeof(a3_1)/sizeof(Action),15000}};
constexpr Action a5_0[]={{"V4",true},{"V5",true},{"PUMP_2",true}};
constexpr Phase p5[]={{"Wine PC1 -> PC2",a5_0,sizeof(a5_0)/sizeof(Action),30000}};
constexpr Action a6_0[]={{"CV4",true},{"DV2",true},{"HP_PUMP",true},{"PUMP_5",true}};
constexpr Phase p6[]={{"Wine clean PC1",a6_0,sizeof(a6_0)/sizeof(Action),30000}};
constexpr Action a7_0[]={{"CV4",false},{"DV2",false},{"V4",false},{"CV5",true},{"FV4",true},{"HP_PUMP",true},{"PUMP_2",true},{"PUMP_5",true}};
constexpr Action a7_1[]={{"PUMP_2",false},{"CV6",true},{"FV3",true}};
constexpr Phase p7[]={{"First flush",a7_0,sizeof(a7_0)/sizeof(Action),15000},{"Reverse backflush",a7_1,sizeof(a7_1)/sizeof(Action),15000}};
constexpr Action a9_0[]={{"V6",true},{"V7",true},{"PUMP_3",true}};
constexpr Phase p9[]={{"Wine PC2 -> PC1",a9_0,sizeof(a9_0)/sizeof(Action),30000}};
constexpr Action a10_0[]={{"CV7",true},{"DV3",true},{"HP_PUMP",true},{"PUMP_5",true}};
constexpr Phase p10[]={{"Wine clean PC2",a10_0,sizeof(a10_0)/sizeof(Action),30000}};
constexpr Action a11_0[]={{"CV7",false},{"DV3",false},{"V6",false},{"CV8",true},{"FV5",true},{"HP_PUMP",true},{"PUMP_3",true},{"PUMP_5",true}};
constexpr Action a11_1[]={{"PUMP_3",false},{"CV9",true},{"FV6",true}};
constexpr Phase p11[]={{"First flush",a11_0,sizeof(a11_0)/sizeof(Action),15000},{"Reverse backflush",a11_1,sizeof(a11_1)/sizeof(Action),15000}};
constexpr Action a12_0[]={{"V8",true},{"PUMP_4",true}};
constexpr Phase p12[]={{"Wine PC1 -> final",a12_0,sizeof(a12_0)/sizeof(Action),30000}};
constexpr Action a13_0[]={{"V9",true},{"PUMP_4",true}};
constexpr Phase p13[]={{"Wine PC2 -> final",a13_0,sizeof(a13_0)/sizeof(Action),30000}};
constexpr Action a14_0[]={{"CV8",true},{"DV4",true},{"HP_PUMP",true},{"PUMP_4",true},{"PUMP_5",true}};
constexpr Phase p14[]={{"Wine clean final transfer hose",a14_0,sizeof(a14_0)/sizeof(Action),30000}};
constexpr Action a16_0[]={{"CV10",true},{"DV5",true},{"HP_PUMP",true},{"PUMP_5",true}};
constexpr Phase p16[]={{"Wine clean output",a16_0,sizeof(a16_0)/sizeof(Action),30000}};
constexpr Action a18_0[]={{"V10",true}};
constexpr Phase p18[]={{"Wine product dispensing",a18_0,sizeof(a18_0)/sizeof(Action),30000}};
constexpr Action a19_0[]={{"VV1",true}};
constexpr Phase p19[]={{"Vinegar pour initial",a19_0,sizeof(a19_0)/sizeof(Action),30000}};
constexpr Action a20_0[]={{"DCP1",true}};
constexpr Phase p20[]={{"Mother vinegar dosing",a20_0,sizeof(a20_0)/sizeof(Action),30000}};
constexpr Action a21_0[]={{"VV2",true},{"VV3",true},{"VPUMP_1",true}};
constexpr Phase p21[]={{"Vinegar initial -> PC1",a21_0,sizeof(a21_0)/sizeof(Action),30000}};
constexpr Action a22_0[]={{"VV2",true},{"VV4",true},{"VPUMP_1",true}};
constexpr Phase p22[]={{"Vinegar initial -> PC2",a22_0,sizeof(a22_0)/sizeof(Action),30000}};
constexpr Action a23_0[]={{"CVV1",true},{"DVV1",true},{"HP_PUMP",true},{"VPUMP_4",true}};
constexpr Phase p23[]={{"Vinegar clean initial",a23_0,sizeof(a23_0)/sizeof(Action),30000}};
constexpr Action a24_0[]={{"CVV2",true},{"DVV2",true},{"HP_PUMP",true}};
constexpr Phase p24[]={{"Vinegar clean initial hoses",a24_0,sizeof(a24_0)/sizeof(Action),30000}};
constexpr Action a25_0[]={{"AV1",true},{"AP",true}};
constexpr Phase p25[]={{"Vinegar aerate PC1",a25_0,sizeof(a25_0)/sizeof(Action),30000}};
constexpr Action a26_0[]={{"AV2",true},{"AP",true}};
constexpr Phase p26[]={{"Vinegar aerate PC2",a26_0,sizeof(a26_0)/sizeof(Action),30000}};
constexpr Action a27_0[]={{"VV5",true},{"VV7",true},{"VPUMP_2",true}};
constexpr Phase p27[]={{"Vinegar PC1 -> output",a27_0,sizeof(a27_0)/sizeof(Action),30000}};
constexpr Action a28_0[]={{"VV6",true},{"VV8",true},{"VPUMP_3",true}};
constexpr Phase p28[]={{"Vinegar PC2 -> output",a28_0,sizeof(a28_0)/sizeof(Action),30000}};
constexpr Action a29_0[]={{"CVV3",true},{"DVV3",true},{"HP_PUMP",true},{"VPUMP_4",true}};
constexpr Phase p29[]={{"Vinegar clean PC1",a29_0,sizeof(a29_0)/sizeof(Action),30000}};
constexpr Action a30_0[]={{"CVV5",true},{"DVV4",true},{"HP_PUMP",true},{"VPUMP_4",true}};
constexpr Phase p30[]={{"Vinegar clean PC2",a30_0,sizeof(a30_0)/sizeof(Action),30000}};
constexpr Action a31_0[]={{"CVV4",true},{"DVV5",true},{"HP_PUMP",true},{"VPUMP_2",true}};
constexpr Phase p31[]={{"Vinegar clean PC1 hoses",a31_0,sizeof(a31_0)/sizeof(Action),30000}};
constexpr Action a32_0[]={{"CVV6",true},{"DVV5",true},{"HP_PUMP",true},{"VPUMP_3",true}};
constexpr Phase p32[]={{"Vinegar clean PC2 hoses",a32_0,sizeof(a32_0)/sizeof(Action),30000}};
constexpr Action a33_0[]={{"CVV7",true},{"DVV6",true},{"HP_PUMP",true},{"VPUMP_4",true}};
constexpr Phase p33[]={{"Vinegar clean output",a33_0,sizeof(a33_0)/sizeof(Action),30000}};
constexpr FunctionDefinition functions[]={
  {"W_A","Wine pour initial",p0,1,DefinitionStatus::RelayDemo,"Manual bench intake only; pH/ethanol/ultrasonic inspection not implemented."},
  {"W_B","Wine initial -> PC1",p1,1,DefinitionStatus::RelayDemo,""},
  {"W_C","Wine clean initial",p2,1,DefinitionStatus::RelayDemo,""},
  {"W_D","Wine F1/FF1 backflush",p3,2,DefinitionStatus::RelayDemo,"Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation."},
  {"W_E","Wine PC1 monitor",nullptr,0,DefinitionStatus::SensorsPending,"pH and ethanol sensors not installed/calibrated."},
  {"W_F","Wine PC1 -> PC2",p5,1,DefinitionStatus::RelayDemo,""},
  {"W_G","Wine clean PC1",p6,1,DefinitionStatus::RelayDemo,""},
  {"W_H","Wine F2/FF2 backflush",p7,2,DefinitionStatus::RelayDemo,"Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation."},
  {"W_I","Wine PC2 monitor",nullptr,0,DefinitionStatus::SensorsPending,"pH and ethanol sensors not installed/calibrated."},
  {"W_J","Wine PC2 -> PC1",p9,1,DefinitionStatus::RelayDemo,"V2 explicitly normalizes old PUMP 3 to PUMP_3."},
  {"W_K","Wine clean PC2",p10,1,DefinitionStatus::RelayDemo,""},
  {"W_L","Wine F3/FF3 backflush",p11,2,DefinitionStatus::RelayDemo,"Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation."},
  {"W_M1","Wine PC1 -> final",p12,1,DefinitionStatus::RelayDemo,""},
  {"W_M2","Wine PC2 -> final",p13,1,DefinitionStatus::RelayDemo,""},
  {"W_N","Wine clean final transfer hose",p14,1,DefinitionStatus::RelayDemo,""},
  {"W_O","Wine post-transfer J -> K",nullptr,0,DefinitionStatus::Ambiguous,"J is product transfer, not cleaning; clarify composite route."},
  {"W_P","Wine clean output",p16,1,DefinitionStatus::RelayDemo,""},
  {"W_DAILY","Wine daily alternation",nullptr,0,DefinitionStatus::Reserved,"Availability, sensors and one-week orchestration are not validated."},
  {"W_DISPENSE","Wine product dispensing",p18,1,DefinitionStatus::RelayDemo,"Final source specifies manual V10. Timed unloaded demonstration; no dispensing volume claimed."},
  {"V_A","Vinegar pour initial",p19,1,DefinitionStatus::RelayDemo,"User clarification 2026-10-18: IV2 was a typo for VV1. Sensor gates pending."},
  {"V_B","Mother vinegar dosing",p20,1,DefinitionStatus::RelayDemo,"Bench pulse only; no ultrasonic dosing decision or validated dose."},
  {"V_C1","Vinegar initial -> PC1",p21,1,DefinitionStatus::RelayDemo,""},
  {"V_C2","Vinegar initial -> PC2",p22,1,DefinitionStatus::RelayDemo,""},
  {"V_D1","Vinegar clean initial",p23,1,DefinitionStatus::RelayDemo,""},
  {"V_D2","Vinegar clean initial hoses",p24,1,DefinitionStatus::RelayDemo,""},
  {"V_E1","Vinegar aerate PC1",p25,1,DefinitionStatus::RelayDemo,"Bench only; temperature/pH/ethanol control unimplemented."},
  {"V_E2","Vinegar aerate PC2",p26,1,DefinitionStatus::RelayDemo,"Bench only; temperature/pH/ethanol control unimplemented."},
  {"V_F1","Vinegar PC1 -> output",p27,1,DefinitionStatus::RelayDemo,""},
  {"V_F2","Vinegar PC2 -> output",p28,1,DefinitionStatus::RelayDemo,"User clarification 2026-10-18: vinegar VPUMP_3 is distinct from wine PUMP_3."},
  {"V_G1","Vinegar clean PC1",p29,1,DefinitionStatus::RelayDemo,""},
  {"V_G2","Vinegar clean PC2",p30,1,DefinitionStatus::RelayDemo,""},
  {"V_H1","Vinegar clean PC1 hoses",p31,1,DefinitionStatus::RelayDemo,""},
  {"V_H2","Vinegar clean PC2 hoses",p32,1,DefinitionStatus::RelayDemo,""},
  {"V_I","Vinegar clean output",p33,1,DefinitionStatus::RelayDemo,""},
};
constexpr size_t FUNCTION_COUNT=sizeof(functions)/sizeof(functions[0]);
inline int functionIndex(const char *name){for(size_t i=0;i<FUNCTION_COUNT;++i)if(sameName(name,functions[i].id))return static_cast<int>(i);return -1;}
} // namespace demo
