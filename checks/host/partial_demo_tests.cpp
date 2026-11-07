#define TUBATECH_TEST_PARTIAL_DEMO
#define TUBATECH_TEST_EXCLUDE_SLAVE2
#define main baseline_main
#include "integrated_master_tests.cpp"
#undef main

static local::Reply ap(const std::string &path){
  static unsigned id=0;
  return local::handle({"POST",path,"","boot_id="+std::to_string(sessionID)+"&stop_epoch="+
    std::to_string(local::stopEpoch)+"&request_id=partial_demo_"+std::to_string(++id),true});
}
static void absentTraffic(){
  assert(!nodes[1].installed&&!nodes[1].synced&&!nodes[1].waiting&&!nodes[1].outputsArmed);
  for(const auto &send:mockSends)assert(memcmp(send.mac,integrated::macs[2],6)!=0);
  for(const auto &peer:mockPeers)assert(memcmp(peer.peer_addr,integrated::macs[2],6)!=0);
  assert(!remoteBanks[1].commanded);
}
static void running(){unsigned guard=0;while(engine.stage()!=demo::Stage::Running){drive(1);assert(++guard<1000&&!engine.faulted());}}
static void wb(){
  boot();assert(demo::PARTIAL_DEMO_ENABLED&&demo::excludedNodeMask()==2);absentTraffic();
  assert(masterIO.acknowledgement(1,0)==demo::AckResult::Failed);
  assert(!channelTest.start("PUMP_1",mockNow));
  assert(ap("/api/functions/W_B/start").code==202);running();
  assert(engine.requiredMask()==1&&engine.profile().count==3&&engine.skippedActions()==1);
  assert(remoteBanks[0].commanded==3&&!remoteBanks[1].commanded&&!localBank.commanded);
  assert(engine.remainingMs(mockNow)==30000);
  std::ofstream(".build/tests/partial_running.json")<<local::statusJson();
  std::ofstream(".build/tests/partial_functions.json")<<local::functionsJson();
  std::ofstream(".build/tests/partial_components.json")<<local::componentsJson();
  drive(2999);assert(engine.stage()==demo::Stage::Running&&engine.remainingMs(mockNow)==10);
  drive(1);finish();off();assert(engine.partialCompletion()&&!engine.faulted());
  assert(std::string(engine.stateName())=="COMPLETE_PARTIAL");absentTraffic();
  std::ofstream(".build/tests/partial_complete.json")<<local::statusJson();
  // Late/stray frames from a deliberately excluded physical board are ignored.
  Packet stray(MAGIC,sessionID,1,ACK,2,0,0);stray.owner=2;
  esp_now_recv_info_t info={integrated::macs[2]};
  for(unsigned i=0;i<100;++i)onReceive(&info,reinterpret_cast<const uint8_t*>(&stray),sizeof(stray));
  drive(1);assert(!rxOverflow&&!engine.faulted());absentTraffic();
  puts("PASS: partial W_B: V2/V3 ACKed on Slave 1, PUMP_1 skipped at original assignment, 30s then participating shutdown; no Slave 2 traffic or false ACK");
}
static void onlySkippedAndUnchanged(){
  boot();assert(ap("/api/functions/W_B/start").code==202);drive(1);assert(ap("/api/stop").code==202);finish();off();
  online[0]=false;drive(400);assert(!engine.faulted());
  const size_t first=mockSends.size();
  assert(ap("/api/functions/V_C1/start").code==202);running();
  assert(engine.requiredMask()==0&&engine.skippedActions()==3&&engine.remainingMs(mockNow)==30000);
  Packet stray(MAGIC,sessionID,1,ACK,1,0,0);stray.owner=1;
  esp_now_recv_info_t sender={integrated::macs[1]};
  for(unsigned i=0;i<100;++i)onReceive(&sender,reinterpret_cast<const uint8_t*>(&stray),sizeof(stray));
  assert(!rxOverflow); // A skipped board's stale replies cannot interrupt the running phase.
  drive(2999);assert(engine.stage()==demo::Stage::Running&&engine.remainingMs(mockNow)==10);
  drive(1);finish();assert(engine.partialCompletion()&&!engine.faulted());off();absentTraffic();
  for(size_t i=first;i<mockSends.size();++i){Packet p;memcpy(&p,mockSends[i].data.data(),sizeof(p));assert(p.type!=SET_RELAY&&p.type!=ALL_OFF);}
  boot();assert(ap("/api/functions/W_F/start").code==202);running();
  assert(engine.skippedActions()==0&&localBank.commanded==3&&remoteBanks[3].commanded==(1U<<10));
  drive(3000);finish();assert(engine.stage()==demo::Stage::Complete&&!engine.partialCompletion());off();absentTraffic();
  puts("PASS: all-excluded V_C1 preserves full 30s with no output/shutdown commands; unaffected W_F keeps ACKs and ordinary completion");
}
static void stopsRecoveryAndFaults(){
  for(unsigned at=0;at<3;++at){
    boot();assert(ap("/api/functions/W_B/start").code==202);
    if(at==1)drive(1);if(at==2){running();drive(20);}
    assert(ap("/api/stop").code==202);finish();assert(engine.stage()==demo::Stage::Stopped&&!engine.faulted());off();
    assert(ap("/api/reset").code==202);drive(500);assert(!recovering&&engine.stage()==demo::Stage::Idle&&!engine.busy());off();absentTraffic();
  }
  for(unsigned fault=0;fault<3;++fault){
    boot();if(fault==1){dropNode=0;dropType=SET_RELAY;}if(fault==2){corruptNode=0;corruptType=SET_RELAY;}
    assert(ap("/api/functions/W_B/start").code==202);
    if(fault==0){running();online[0]=false;}
    drive(1500);assert(engine.faulted()&&engine.stage()==demo::Stage::Fault);off();absentTraffic();
    online[0]=true;dropNode=dropType=corruptNode=corruptType=-1;
    assert(ap("/api/reset").code==202);drive(500);assert(!recovering&&!engine.faulted()&&!engine.busy());off();absentTraffic();
  }
  boot();localBank.disable();nodes[4].outputsArmed=nodes[4].synced=false;
  assert(ap("/api/functions/W_F/start").code==409);off();absentTraffic(); // MASTER GPIOs are never auto-excluded.
  puts("PASS: STOP before/during activation and timer; RESET/recovery without absent-node waits or restart; participating loss/missing ACK/bad ACK still fault and shut down");
}
static void missingAtStart(){
  boot();online[0]=online[2]=online[3]=false;drive(400);assert(!engine.faulted());
  assert(ap("/api/functions/W_B/start").code==202);running();
  assert(engine.requiredMask()==0&&engine.skippedActions()==3&&engine.remainingMs(mockNow)==30000);
  online[0]=true;drive(2999);assert(engine.stage()==demo::Stage::Running&&remoteBanks[0].commanded==0);
  drive(1);finish();off();assert(engine.partialCompletion());absentTraffic();
  drive(500);assert(masterIO.ready(0,mockNow));
  assert(ap("/api/functions/W_B/start").code==202);running();
  assert(engine.requiredMask()==1&&remoteBanks[0].commanded==3&&engine.skippedActions()==1);
  assert(ap("/api/stop").code==202);finish();off();absentTraffic();
  boot();online[0]=false;nodes[0].stateMask=1;nodes[0].synced=false;
  assert(ap("/api/functions/W_B/start").code==409); // Uncertain/possibly ON is not silently skipped.
  puts("PASS: three missing slaves (plus absent S2) do not block START; late return waits until next run; unsafe outputs and MASTER GPIO readiness stay protected");
}
static void allFunctions(){
  unsigned executed=0;
  for(size_t i=0;i<demo::FUNCTION_COUNT;++i){
    boot();const auto &f=demo::functions[i];
    if(f.status!=demo::DefinitionStatus::RelayDemo){assert(ap(std::string("/api/functions/")+f.id+"/start").code==409);continue;}
    for(size_t p=0;p<f.phaseCount;++p)assert(engine.setTime(int(i),1,p));
    assert(ap(std::string("/api/functions/")+f.id+"/start").code==202);finish();off();absentTraffic();
    assert(engine.stage()==demo::Stage::Complete&&!engine.faulted());
    assert(engine.partialCompletion()==(engine.profile().skippedActionCount>0));
    assert(engine.skippedActions()==engine.profile().skippedActionCount);++executed;
  }
  assert(executed==30);
  puts("PASS: all 30 relay functions complete with explicit exclusions; four pending/reserved functions stay blocked");
}
int main(){wb();onlySkippedAndUnchanged();stopsRecoveryAndFaults();missingAtStart();allFunctions();}
