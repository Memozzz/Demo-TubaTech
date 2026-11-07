#pragma once
#include "function_library.h"
#include "demo_config.h"
namespace demo {
struct BenchProfile {
 Component slots[70]={};size_t count=0,skippedActionCount=0;int function=-1;uint32_t revision=0;bool confirmed=false;uint8_t requiredMask=0,skippedNodes=0;
 const Component *findName(const char *name)const{return demo::find(name,slots,count);}
 bool skips(uint8_t node)const{return node<5&&(skippedNodes&(1U<<node));}
 bool build(const FunctionDefinition &f,uint8_t availableMask=DEFAULT_BENCH_MASK,uint8_t excluded=excludedNodeMask()){
  count=skippedActionCount=0;confirmed=false;requiredMask=0;
  skippedNodes=excluded;
  if(f.status!=DefinitionStatus::RelayDemo||!f.phases||!f.phaseCount)return false;
  for(size_t p=0;p<f.phaseCount;++p)for(size_t a=0;a<f.phases[p].count;++a){
   const auto &action=f.phases[p].actions[a];if(action.kind!=ActionKind::SetOutput)return false;
   const auto *c=demo::find(action.component);if(!c)return false;
   const bool skipped=skips(c->node);
   if(skipped)++skippedActionCount;
   else if(!(availableMask&(1U<<c->node)))return false;
   if(findName(c->name))continue;
   slots[count++]=*c;if(!skipped)requiredMask|=uint8_t(1U<<c->node);
  }
  return count&&validRegistry(slots,count,5);
 }
};
}
