#define TUBATECH_TEST_CURRENT_CONFIG
#define main baseline_main
#include "integrated_master_tests.cpp"
#undef main

static void command(const std::string &s){
  const std::string line=s+"\n";Serial.input.insert(Serial.input.end(),line.begin(),line.end());drive(1);
  assert(Serial.input.empty());
}
static void held(){unsigned guard=0;while(!engine.manualHeld()){drive(1);assert(++guard<1000&&!engine.faulted());}}
static void wb(){
  boot();off();command("START W_B ON");held();
  assert(remoteBanks[0].commanded==3&&remoteBanks[1].commanded==1&&!localBank.commanded);
  assert(Serial.output.find("GPIO ACK: V2 ON")!=std::string::npos);
  assert(Serial.output.find("GPIO ACK: V3 ON")!=std::string::npos);
  assert(Serial.output.find("GPIO ACK: PUMP_1 ON")!=std::string::npos);
  assert(std::string(engine.stateName())=="MANUAL_ON"&&engine.remainingMs(mockNow)==0);
  const auto heartbeats=heartbeatCount;drive(6000);
  assert(engine.manualHeld()&&remoteBanks[0].commanded==3&&remoteBanks[1].commanded==1&&heartbeatCount>heartbeats);
  assert(engine.duration(demo::functionIndex("W_B"))==30000);
  assert(local::statusJson().find("\"manual_held\":true")!=std::string::npos);
  command("START W_B OFF");finish();off();assert(engine.stage()==demo::Stage::Stopped&&!engine.faulted());
  // Explicit OFF is also valid after reboot or when no manual group is active.
  boot();command("START W_B OFF");finish();off();assert(!engine.faulted());
  puts("PASS: actual Serial newline parser W_B ON holds beyond 30s with ACKs/heartbeats, zero fake countdown, explicit OFF and boot-time OFF request");
}
static void partialAndLateJoin(){
  boot();online[1]=false;drive(400);command("START W_B ON");held();
  assert(engine.requiredMask()==1&&engine.skippedActions()==1&&remoteBanks[0].commanded==3&&!remoteBanks[1].commanded);
  assert(Serial.output.find("SKIPPED / NOT EXECUTED: PUMP_1")!=std::string::npos);
  assert(Serial.output.find("GPIO ACK: PUMP_1 ON")==std::string::npos);
  online[1]=true;drive(500);assert(masterIO.ready(1,mockNow)&&!remoteBanks[1].commanded);
  command("START W_B ON");held();assert(engine.requiredMask()==3&&engine.skippedActions()==0&&remoteBanks[1].commanded==1);
  command("START W_B OFF");finish();off();
  boot();for(auto &available:online)available=false;drive(400);
  command("START W_B ON");held();assert(engine.requiredMask()==0&&engine.skippedActions()==3);off();
  drive(4000);assert(engine.manualHeld()&&!engine.faulted());command("STOP");finish();off();
  command("START W_F ON");held();assert(localBank.commanded==3&&engine.requiredMask()==16);
  drive(4000);assert(localBank.commanded==3&&engine.manualHeld());command("START W_F OFF");finish();off();
  puts("PASS: missing S2 skipped honestly; late S2 stays OFF until repeated ON; all slaves absent, MASTER-only outputs and partial STOP/OFF");
}
static void safety(){
  for(unsigned at=0;at<3;++at){
    boot();command("START W_B ON");if(at==1)drive(1);if(at==2)held();
    command("STOP");finish();off();assert(!engine.faulted());
    command("RESET");drive(500);assert(!recovering&&!engine.busy()&&!engine.faulted());off();
  }
  for(unsigned fault=0;fault<4;++fault){
    boot();if(fault==1){dropNode=0;dropType=SET_RELAY;}if(fault==2){corruptNode=0;corruptType=SET_RELAY;}
    command("START W_B ON");
    if(fault==0){held();online[1]=false;}
    if(fault==3){held();dropNode=1;dropType=ALL_OFF;command("START W_B OFF");}
    drive(1500);assert(engine.faulted()&&engine.stage()==demo::Stage::Fault);off();
    online[1]=true;dropNode=dropType=corruptNode=corruptType=-1;command("RESET");drive(500);
    assert(!recovering&&!engine.busy()&&!engine.faulted());off();
  }
  for(const char *bad:{"START W_E ON","START W_I ON","START W_O ON","START W_DAILY ON",
      "START UNKNOWN ON","START W_B ON junk","START W_B ON 1 junk","START W_B OFF 1",
      "START W_B ONS","START W_B ON 0","START W_D ON","START W_D ON 2"}){
    boot();command(bad);assert(!engine.busy());off();
  }
  boot();command("START W_B ON");held();command("START W_F ON");assert(std::string(engine.activeName())=="W_B");
  command("START W_F OFF");assert(engine.manualHeld()&&remoteBanks[0].commanded==3);
  command("RESET");assert(!recovering&&engine.manualHeld());command("STOP");finish();off();
  puts("PASS: STOP/RESET without restart, participating loss/missing/bad ON ACK/missing OFF ACK fault, unsupported/malformed/ambiguous commands rejected, exclusive group");
}
static void allPhases(){
  unsigned functions=0,phases=0;
  for(size_t i=0;i<demo::FUNCTION_COUNT;++i){
    const auto &f=demo::functions[i];if(f.status!=demo::DefinitionStatus::RelayDemo)continue;
    boot();uint16_t masks[5]={};
    for(size_t p=0;p<f.phaseCount;++p){
      const std::string line=std::string("START ")+f.id+" ON"+(f.phaseCount>1?" "+std::to_string(p+1):"");
      command(line);held();assert(engine.phaseIndex()==p);
      for(size_t a=0;a<f.phases[p].count;++a){const auto &action=f.phases[p].actions[a];const auto *c=demo::find(action.component);assert(c);
        if(action.on)masks[c->node]|=uint16_t(1U<<c->channel);else masks[c->node]&=uint16_t(~(1U<<c->channel));}
      for(unsigned n=0;n<4;++n)assert(remoteBanks[n].commanded==masks[n]);assert(localBank.commanded==masks[4]);
      drive(f.phases[p].defaultMs/10+100);assert(engine.manualHeld()&&engine.phaseIndex()==p);++phases;
    }
    command(std::string("START ")+f.id+" OFF");finish();off();assert(!engine.faulted());++functions;
  }
  assert(functions==30&&phases==33);
  puts("PASS: all 30 manual functions / 33 explicit phases preserve original ordered actions, carried output masks and GPIO assignments; no automatic phase/timer advance");
}
int main(){wb();partialAndLateJoin();safety();allPhases();return 0;}
