#define TUBATECH_TEST_CURRENT_CONFIG
#define main baseline_main
#include "integrated_master_tests.cpp"
#undef main

static void untilRunning(){
  unsigned guard=0;
  while(engine.stage()!=demo::Stage::Running){drive(1);assert(++guard<1000&&!engine.faulted());}
}
static void serialWB(){
  boot();off();
  assert(demo::PARTIAL_DEMO_ENABLED&&demo::excludedNodeMask()==0);
  const uint8_t replacement[]={0x20,0x50,0x0D,0x8C,0x80,0xC4};
  assert(!memcmp(integrated::macs[2],replacement,6)&&!memcmp(nodes[1].mac,replacement,6));
  for(unsigned n=0;n<5;++n)assert(masterIO.ready(uint8_t(n),mockNow));
  const auto before=mockSends.size();Serial.output.clear();handleCommand("NODES");
  assert(Serial.output.find("TETRIARY | 20:50:0D:8C:80:C4 | READY")!=std::string::npos);
  assert(mockSends.size()==before&&!engine.busy()&&!engine.benchArmed());off();
  handleCommand("START W_B");assert(engine.busy());untilRunning();
  assert(engine.skippedActions()==0&&engine.requiredMask()==3);
  assert(remoteBanks[0].commanded==3&&remoteBanks[1].commanded==1);
  assert(engine.remainingMs(mockNow)==30000);
  handleCommand("STATUS");assert(Serial.output.find("\"remaining_ms\":30000")!=std::string::npos);
  drive(2999);assert(engine.stage()==demo::Stage::Running&&engine.remainingMs(mockNow)==10);
  drive(1);finish();off();assert(!engine.partialCompletion()&&!engine.faulted());
  puts("PASS: default config, all five verified links, replacement S2 MAC, read-only NODES, app-free Serial START W_B, full 30s and ACKed shutdown (host stubs)");
}
static void serialSafety(){
  for(unsigned at=0;at<3;++at){
    boot();handleCommand("START W_B");
    if(at==1)drive(1);if(at==2){untilRunning();drive(50);}
    handleCommand("STOP");finish();off();assert(engine.stage()==demo::Stage::Stopped);
    handleCommand("RESET");drive(500);assert(!recovering&&!engine.faulted()&&!engine.busy());off();
  }
  boot();handleCommand("START W_B");untilRunning();online[1]=false;
  drive(1500);assert(engine.faulted()&&engine.stage()==demo::Stage::Fault);off();
  online[1]=true;handleCommand("RESET");drive(500);assert(!recovering&&!engine.faulted()&&!engine.busy());off();
  boot();online[1]=false;drive(400);handleCommand("START W_B");untilRunning();
  assert(engine.requiredMask()==1&&engine.skippedActions()==1&&remoteBanks[0].commanded==3);
  assert(engine.remainingMs(mockNow)==30000);handleCommand("STOP");finish();off();
  boot();for(auto &b:online)b=false;drive(400);handleCommand("START W_F");untilRunning();
  assert(localBank.commanded==3&&engine.requiredMask()==16&&engine.skippedActions()==1);
  assert(engine.remainingMs(mockNow)==30000);drive(3000);finish();off();assert(engine.partialCompletion());
  for(const char *id:{"W_E","W_I","W_O","W_DAILY"}){
    boot();std::string command=std::string("START ")+id;handleCommand(command.c_str());assert(!engine.busy());off();
  }
  puts("PASS: app-free STOP/RESET, participating S2 loss still faults, missing-at-start S2 skipped, MASTER-only outputs keep 30s, unsupported functions stay blocked");
}
static void serialAllFunctions(){
  unsigned count=0;
  for(size_t i=0;i<demo::FUNCTION_COUNT;++i){
    if(demo::functions[i].status!=demo::DefinitionStatus::RelayDemo)continue;
    boot();for(size_t p=0;p<demo::functions[i].phaseCount;++p)assert(engine.setTime(int(i),1,p));
    const std::string command=std::string("START ")+demo::functions[i].id;
    handleCommand(command.c_str());assert(engine.busy());finish();off();
    assert(engine.stage()==demo::Stage::Complete&&!engine.faulted()&&!engine.partialCompletion());++count;
  }
  assert(count==30);
  puts("PASS: all 30 supported functions run through Serial command dispatch using current production configuration (1s test overrides only)");
}
int main(){serialWB();serialSafety();serialAllFunctions();backflushMappingAndTimers();return 0;}
