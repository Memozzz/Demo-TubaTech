#pragma once
#include "function_library.h"
namespace demo {
struct BenchProfile {
 Component slots[70]={};size_t count=0;int function=-1;uint32_t revision=0;bool confirmed=false;uint8_t requiredMask=0;
 const Component *findName(const char *name)const{return demo::find(name,slots,count);}
 bool build(const FunctionDefinition &f,uint8_t availableMask=DEFAULT_BENCH_MASK){
  count=0;confirmed=false;requiredMask=0;
  if(f.status!=DefinitionStatus::RelayDemo||!f.phases||!f.phaseCount)return false;
  for(size_t p=0;p<f.phaseCount;++p)for(size_t a=0;a<f.phases[p].count;++a){
   const auto &action=f.phases[p].actions[a];if(action.kind!=ActionKind::SetOutput)return false;
   const auto *c=demo::find(action.component);if(!c||!(availableMask&(1U<<c->node)))return false;
   if(findName(c->name))continue;
   slots[count++]=*c;requiredMask|=uint8_t(1U<<c->node);
  }
  return count&&validRegistry(slots,count,5);
 }
};
}
