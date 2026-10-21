#define TUBATECH_HOST_TEST
#include <cassert>
#include <new>
#include <fstream>
#include "../../firmware/01_MASTER/TubaTech_Integrated_Master/TubaTech_Integrated_Master.ino"
static integrated::RelayBank remoteBanks[]={integrated::RelayBank(1),integrated::RelayBank(2),integrated::RelayBank(3),integrated::RelayBank(4)};
static bool online[4]={true,true,true,true};
static size_t consumed=0;
static int dropNode=-1,dropType=-1,corruptNode=-1,corruptType=-1;
static unsigned heartbeatCount=0;
static void radio(){
  for(auto &b:remoteBanks)b.tick(mockNow);
  while(consumed<mockSends.size()){
    auto sent=mockSends[consumed++];int index=-1;for(int n=0;n<4;++n)if(!memcmp(sent.mac,integrated::macs[n+1],6))index=n;assert(index>=0);
    if(!online[index])continue;
    assert(sent.data.size()==20);Packet p;memcpy(&p,sent.data.data(),sizeof(p));assert(p.owner==index+1);
    if(p.type==HEARTBEAT)++heartbeatCount;
    auto reply=remoteBanks[index].receive(p,mockNow);
    if(index==dropNode&&p.type==dropType)continue;
    if(index==corruptNode&&p.type==corruptType)reply.stateMask^=uint16_t(1U<<13);
    esp_now_recv_info_t info={integrated::macs[index+1]};onReceive(&info,reinterpret_cast<const uint8_t*>(&reply),sizeof(reply));
  }
}
static void drive(unsigned count){for(unsigned i=0;i<count;++i){mockNow+=10;loop();radio();}}
static void boot(bool arm=true){
  if(rxQueue){delete rxQueue;rxQueue=nullptr;}
  engine.~SequenceEngine();new(&engine)demo::SequenceEngine(masterIO);channelTest=ChannelTest{};
  for(unsigned i=0;i<5;++i){Node fresh={nodes[i].name,{}};memcpy(fresh.mac,integrated::macs[integrated::ownerForIndex(uint8_t(i))],6);nodes[i]=fresh;}
  for(unsigned i=0;i<4;++i){remoteBanks[i].begin();online[i]=true;}
  localBank.begin();radioReady=recovering=serialOverflow=rxOverflow=false;serialLine="";mockNow=100;mockStationMac="38:18:2B:8A:9D:30";mockSends.clear();mockPeers.clear();Serial.output.clear();consumed=0;dropNode=dropType=corruptNode=corruptType=-1;mockSendFailure=false;heartbeatCount=0;
  setup();drive(20);assert(mockPeers.size()==4);for(const auto &n:nodes)assert(n.synced&&n.outputsArmed==bool(TUBATECH_MASTER_ONLY_UNLOADED_ARM));
  // Explicitly model a disarmed bank independently of the configured boot policy.
  if(!arm){for(auto &b:remoteBanks)b.enabled=false;localBank.enabled=false;drive(90);for(const auto &n:nodes)assert(!n.outputsArmed);}
  if(arm){for(auto &b:remoteBanks)assert(b.arm());assert(localBank.arm());drive(90);for(uint8_t i=0;i<5;++i)assert(masterIO.ready(i,mockNow));}
}
static void off(){for(const auto &b:remoteBanks)assert(!b.commanded);assert(!localBank.commanded);}
static void prepare(const char *name){handleCommand("BENCH ARM UNLOADED");assert(engine.loadProfile(demo::functionIndex(name),mockNow));assert(engine.confirmProfile(engine.profile().revision,true,mockNow));}
static void finish(){for(unsigned i=0;i<3000&&(engine.busy()||channelTest.busy());++i)drive(1);assert(!engine.busy()&&!channelTest.busy());}
static local::Reply post(const std::string &path,const std::string &fields="",const std::string &token="operator"){
 static unsigned id=0;return local::handle({"POST",path,token,fields+(fields.empty()?"":"&")+"boot_id="+std::to_string(sessionID)+"&stop_epoch="+std::to_string(local::stopEpoch)+"&request_id=integration_"+std::to_string(++id)});
}
static void functionsAndStop(){
  unsigned phases=0;
  for(size_t index=0;index<demo::FUNCTION_COUNT;++index){const auto &f=demo::functions[index];
    boot();if(f.status!=demo::DefinitionStatus::RelayDemo){handleCommand("BENCH ARM UNLOADED");assert(!engine.loadProfile(int(index),mockNow));continue;}
    prepare(f.id);for(size_t p=0;p<f.phaseCount;++p)assert(engine.setTime(int(index),1,p));assert(engine.start(int(index),mockNow));finish();assert(engine.stage()==demo::Stage::Complete&&!engine.faulted());off();
    for(size_t phase=0;phase<f.phaseCount;++phase)for(unsigned at=0;at<4;++at){boot();prepare(f.id);for(size_t p=0;p<f.phaseCount;++p)engine.setTime(int(index),1,p);assert(engine.start(int(index),mockNow));unsigned guard=0;
      while(!(engine.phaseIndex()==phase&&engine.stage()==demo::Stage::ValvesOn)){drive(1);assert(++guard<2000&&engine.busy());}
      if(at==1)drive(1);
      if(at>=2){while(engine.stage()!=demo::Stage::Running){drive(1);assert(++guard<2000&&engine.busy());}if(at==3)drive(10);}
      requestStop();finish();assert(engine.stage()==demo::Stage::Stopped&&!engine.faulted());off();++phases;
    }
  }
  assert(phases==132);
}
static void transportFaults(){
  boot(false);handleCommand("BENCH ARM UNLOADED");assert(!engine.loadProfile(demo::functionIndex("W_B"),mockNow));off();
  boot();prepare("W_B");assert(engine.start(demo::functionIndex("W_B"),mockNow));drive(30);assert(remoteBanks[0].commanded==3&&remoteBanks[1].commanded==1);assert(!localBank.commanded);unsigned before=heartbeatCount;drive(200);assert(heartbeatCount>before);requestStop();finish();off();
  for(unsigned mode=0;mode<4;++mode){boot();prepare("W_B");if(mode==0){dropNode=0;dropType=SET_RELAY;}if(mode==1){corruptNode=0;corruptType=SET_RELAY;}
    assert(engine.start(demo::functionIndex("W_B"),mockNow));if(mode>=2){drive(30);if(mode==2)online[1]=false;else{dropNode=1;dropType=ALL_OFF;requestStop();}}
    drive(1500);assert(engine.stage()==demo::Stage::Fault&&engine.faulted());assert(Serial.output.find("COMPLETE:")==std::string::npos);off();
  }
  boot();online[3]=false;drive(400);assert(!engine.faulted());prepare("W_B");assert(engine.start(demo::functionIndex("W_B"),mockNow));drive(30);requestStop();finish();off();
  handleCommand("BENCH ARM UNLOADED");assert(!engine.loadProfile(demo::functionIndex("W_A"),mockNow)); // V1 moved to Slave 4.
  boot();prepare("W_C");assert(engine.requiredMask()&(1U<<4));assert(engine.start(demo::functionIndex("W_C"),mockNow));drive(60);assert(localBank.commanded);localBank.disable();drive(1500);assert(engine.faulted());off();
}
static void individualAndApi(){
  boot();local::access.operatorToken="operator";local::access.adminToken="admin";
  assert(post("/api/functions/W_B/start","revision=0","").code==401);
  assert(post("/api/profiles/load","function=W_B").code==409);
  handleCommand("BENCH ARM UNLOADED");assert(post("/api/profiles/load","function=W_B").code==200);auto rev=engine.profile().revision;
  assert(post("/api/profiles/confirm","revision="+std::to_string(rev)+"&unloaded=true").code==200);
  assert(post("/api/functions/W_B/start","revision="+std::to_string(rev)).code==202);drive(30);assert(post("/api/stop").code==202);finish();off();
  assert(post("/api/mode","mode=REAL").code==403);
  for(const auto &c:demo::components){handleCommand("BENCH ARM UNLOADED");assert(channelTest.start(c.name,mockNow));assert(post("/api/profiles/load","function=W_B").code==409);finish();assert(!engine.faulted());off();}
  handleCommand("BENCH ARM UNLOADED");assert(channelTest.start("PUMP_1",mockNow));drive(1);assert(post("/api/stop").code==202);finish();off();
  std::ofstream(".build/tests/integrated_status.json")<<local::statusJson();std::ofstream(".build/tests/integrated_functions.json")<<local::functionsJson();
}
static void wifiPhoneWorkflow(){
  boot();const std::string token(64,'a');local::access.operatorToken=token;local::access.adminToken=std::string(64,'b');
  const auto sentBefore=mockSends.size();
  assert(local::handle({"POST","/api/connect","","client=tubatech_flutter"}).code==403);
  assert(local::handle({"POST","/api/connect","","apClient=true"}).code==403);
  auto joined=local::handle({"POST","/api/connect","","client=tubatech_flutter",true});
  assert(joined.code==200&&joined.body.find(token)!=std::string::npos&&joined.body.find(local::access.adminToken)==std::string::npos);
  assert(!engine.benchArmed()&&mockSends.size()==sentBefore);off();
  assert(post("/api/bench/arm","unloaded=true","").code==401);
  assert(post("/api/bench/arm","",token).code==400);
  assert(post("/api/bench/arm","unloaded=true&mode=REAL",token).code==403);
  assert(local::handle({"POST","/api/bench/arm",token,"unloaded=true&boot_id=0&request_id=stale_arm_01"}).code==409);
  assert(!engine.benchArmed());off();
  assert(post("/api/bench/arm","unloaded=true",token).code==200);
  assert(engine.benchArmed()&&!engine.profile().confirmed&&mockSends.size()==sentBefore);off();
  assert(post("/api/profiles/load","function=W_B",token).code==200);
  const auto revision=std::to_string(engine.profile().revision);
  assert(post("/api/functions/W_B/start","revision="+revision,token).code==409);
  assert(post("/api/profiles/confirm","revision="+revision+"&unloaded=true",token).code==200);
  assert(engine.duration(demo::functionIndex("W_B"),0)==30000);
  assert(post("/api/functions/W_B/start","revision="+revision,token).code==202);
  drive(30);assert(engine.stage()==demo::Stage::Running);
  assert(post("/api/bench/arm","unloaded=true",token).code==409);
  drive(2000);assert(engine.busy());
  for(unsigned i=0;i<1500&&engine.busy();++i)drive(1);
  assert(engine.stage()==demo::Stage::Complete&&!engine.faulted());off();
  assert(local::statusJson().find("\"phone_bench_arm_supported\":true")!=std::string::npos);
  puts("PASS: AP-only operator enrollment, no automatic arming/output, explicit phone attestation, authenticated revision-confirmed 30-second W_B and acknowledged shutdown (host stubs)");
}
static void directPhoneStart(){
  auto ap=[](const std::string &path,const std::string &extra=""){
    static unsigned id=0;
    return local::handle({"POST",path,"",extra+(extra.empty()?"":"&")+"boot_id="+std::to_string(sessionID)+"&stop_epoch="+std::to_string(local::stopEpoch)+"&request_id=direct_phone_"+std::to_string(++id),true});
  };
  boot();online[2]=online[3]=false;drive(400);assert(!engine.faulted());
  assert(!engine.benchArmed());off();
  assert(local::handle({"GET","/api/status","","",true}).code==200);
  assert(ap("/api/functions/W_B/start").code==202); // No token, arm, profile, confirmation or revision.
  assert(ap("/api/functions/W_B/start").code==409); // Exclusive execution.
  while(engine.stage()!=demo::Stage::Running){drive(1);assert(!engine.faulted());}
  assert(remoteBanks[0].commanded==3&&remoteBanks[1].commanded==1&&!localBank.commanded);
  const auto began=mockNow;drive(2999);assert(engine.stage()==demo::Stage::Running);
  drive(1);finish();assert(uint32_t(mockNow-began)>=30000&&engine.stage()==demo::Stage::Complete);off();
  assert(ap("/api/functions/W_B/start").code==202);drive(30);
  assert(ap("/api/stop").code==202);finish();assert(engine.stage()==demo::Stage::Stopped);off();
  boot();online[0]=false;drive(400);assert(ap("/api/functions/W_B/start").code==409);off();
  boot();dropNode=0;dropType=SET_RELAY;
  assert(ap("/api/functions/W_B/start").code==202);drive(1500);
  assert(engine.faulted()&&engine.stage()==demo::Stage::Fault);off();
  puts("PASS: direct AP START without operator/arm/profile gates; unrelated offline nodes ignored, 30-second W_B, STOP, missing owner and missing ACK (host stubs)");
}
static void stopFencesDelayedStart(){
  boot();
  const std::string delayed="boot_id="+std::to_string(sessionID)+"&stop_epoch="+std::to_string(local::stopEpoch)+"&request_id=delayed_start_0001";
  assert(local::handle({"POST","/api/stop","","",true}).code==202);
  finish();off();
  assert(local::handle({"POST","/api/functions/W_B/start","",delayed,true}).code==409);
  assert(!engine.busy());off();
  const std::string fresh="boot_id="+std::to_string(sessionID)+"&stop_epoch="+std::to_string(local::stopEpoch)+"&request_id=fresh_start_0001";
  assert(local::handle({"POST","/api/functions/W_B/start","",fresh,true}).code==202);
  drive(30);requestStop();finish();off();
  puts("PASS: STOP epoch rejects delayed START; fresh explicit START remains available");
}
static void backflushMappingAndTimers(){
  for(const char *id:{"W_D","W_H","W_L"}){
    boot();assert(engine.startDemo(demo::functionIndex(id),mockNow));
    uint16_t expected[5]={};
    const auto &f=demo::functions[demo::functionIndex(id)];
    for(size_t phase=0;phase<f.phaseCount;++phase){
      unsigned guard=0;while(engine.stage()!=demo::Stage::Running){drive(1);assert(++guard<1000&&!engine.faulted());}
      assert(engine.phaseIndex()==phase&&engine.remainingMs(mockNow)==15000);
      for(size_t a=0;a<f.phases[phase].count;++a){
        const auto &action=f.phases[phase].actions[a];const auto *c=demo::find(action.component);assert(c);
        if(action.on)expected[c->node]|=uint16_t(1U<<c->channel);else expected[c->node]&=uint16_t(~(1U<<c->channel));
      }
      for(unsigned n=0;n<4;++n)assert(remoteBanks[n].commanded==expected[n]);
      assert(localBank.commanded==expected[4]);
      drive(1499);assert(engine.stage()==demo::Stage::Running&&engine.phaseIndex()==phase&&engine.remainingMs(mockNow)==10);
      drive(1);assert(engine.stage()!=demo::Stage::Running);
    }
    finish();assert(engine.stage()==demo::Stage::Complete);off();
  }
  puts("PASS: W_D/W_H/W_L full 15s + 15s timings, accumulated valve/pump masks, pump OFF between phases, final all OFF");
}
int main(){functionsAndStop();transportFaults();individualAndApi();wifiPhoneWorkflow();directPhoneStart();stopFencesDelayedStart();backflushMappingAndTimers();puts("PASS: actual integrated Master + real bank adapter: 30 functions, 132 STOP checkpoints over 33 phases, all 66 single-channel tests, four peers, local outputs, W_B pins, API gates, required/optional loss, ACK faults and watchdog OFF (host GPIO/radio stubs)");}
