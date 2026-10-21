#pragma once
#include "api_policy.h"
#include "sensor_registry.h"

// Included after the existing Master transport/engine declarations. This adapter
// invokes the same engine used by Serial; it never sends relay packets itself.
namespace local {
inline Access access;
// A STOP fences any START already in flight over another HTTP connection.
// The phone must use the epoch from its latest live status. STOP needs no epoch.
inline uint32_t stopEpoch=1;
inline std::string statusJson(){
  std::string out="{\"demo_control_version\":1,\"stop_epoch\":"+std::to_string(stopEpoch)+",\"direct_demo_start_supported\":true,\"wifi_connect_supported\":true,\"phone_bench_arm_supported\":true,\"channel_test_active\":"+std::string(channelTest.busy()?"true":"false")+",\"catalog_version\":\"final-66-2026-10-19\",\"layout_id\":\"five-board-fixed-v1\",\"hardware_verified\":false,\"bench_node_mask\":"+std::to_string(engine.benchMask())+",\"required_node_mask\":"+std::to_string(engine.requiredMask())+",\"boot_id\":"+std::to_string(sessionID)+",\"mode\":\"FIXED RELAY TIMED CONTROL\",\"state\":"+quote(demo::SequenceEngine::stageName(engine.stage()))+
    ",\"active_function\":"+quote(engine.activeName())+",\"phase\":"+quote(engine.phaseName())+",\"phase_index\":"+std::to_string(engine.phaseIndex())+
    ",\"remaining_ms\":"+std::to_string(engine.remainingMs(millis()))+",\"busy\":"+(engine.busy()||channelTest.busy()?"true":"false")+
    ",\"fault\":"+(engine.faulted()?"true":"false")+",\"reason\":"+quote(engine.reason())+",\"recovering\":"+(recovering?"true":"false")+
    ",\"bench_armed\":"+(engine.benchArmed()?"true":"false")+",\"all_off_verified\":"+(engine.readyOff(millis())?"true":"false")+",\"nodes\":[";
  for(uint8_t i=0;i<demo::ACTIVE_NODES;++i){if(i)out+=',';const auto &n=nodes[i];
    out+="{\"session_verified\":"+std::string(n.synced&&!n.isolated?"true":"false")+",\"outputs_armed\":"+std::string(n.outputsArmed?"true":"false")+",\"installed\":"+std::string(n.installed?"true":"false")+",\"mac\":"+quote(n.installed?nodecfg::macText(n.mac):"UNPROVISIONED")+",\"status\":"+quote(!n.installed?"NOT INSTALLED":masterIO.ready(i,millis())?"READY":n.synced?"SESSION VERIFIED / OUTPUTS DISARMED":"OFFLINE / UNVERIFIED")+",\"node_id\":"+std::to_string(integrated::ownerForIndex(i))+",\"name\":"+quote(n.name)+",\"ready\":"+(masterIO.ready(i,millis())?"true":"false")+
      ",\"isolated\":"+(n.isolated?"true":"false")+",\"pending\":"+(n.waiting?"true":"false")+",\"command_uncertain\":"+((n.uncertainMask||(n.waiting&&n.pendingType==SET_RELAY))?"true":"false")+",\"last_ack_age_ms\":"+(n.lastACK?std::to_string(uint32_t(millis()-n.lastACK)):"null")+",\"acknowledged_mask\":"+(masterIO.ready(i,millis())&&!n.uncertainMask?std::to_string(n.stateMask):"null")+"}";
  }
  const auto &p=engine.profile();
  out+="],\"profile\":{\"function\":"+quote(p.function<0?"none":demo::functions[p.function].id)+",\"revision\":"+std::to_string(p.revision)+",\"confirmed\":"+(p.confirmed?"true":"false")+",\"mappings\":[";
  for(size_t i=0;i<p.count;++i){if(i)out+=',';const auto &c=p.slots[i];const auto &n=nodes[c.node];
    const bool verified=masterIO.ready(c.node,millis())&&!n.waiting;
    out+="{\"component\":"+quote(c.name)+",\"category\":"+quote(c.category)+",\"node_id\":"+std::to_string(integrated::ownerForIndex(c.node))+",\"board\":"+quote(n.name)+",\"channel\":"+std::to_string(c.channel)+",\"gpio\":"+std::to_string(c.gpio)+",\"relay_module\":"+std::to_string(c.module)+",\"relay_input\":"+std::to_string(c.relayInput)+
      ",\"mapping_verified\":"+(c.mappingVerified?"true":"false")+",\"installed_equipment\":false,\"commanded_on\":"+(masterIO.ready(c.node,millis())?(((n.waiting?n.expectedMask:n.stateMask)&(1U<<c.channel))?"true":"false"):"null")+",\"acknowledged_on\":"+(verified?((n.stateMask&(1U<<c.channel))?"true":"false"):"null")+",\"command_pending\":"+(n.waiting?"true":"false")+"}";
  }
  out+="],\"off_only_modeled\":[";bool first=true;
  if(p.function>=0){const auto &f=demo::functions[p.function];for(size_t phase=0;phase<f.phaseCount;++phase)for(size_t a=0;a<f.phases[phase].count;++a){const auto &action=f.phases[phase].actions[a];if(!action.on&&!p.findName(action.component)){if(!first)out+=',';first=false;out+=quote(action.component);}}}
  return out+"]}}";
}
inline const char *definitionStatus(demo::DefinitionStatus s){
  switch(s){case demo::DefinitionStatus::RelayDemo:return "RELAY DEMO";case demo::DefinitionStatus::SensorsPending:return "PENDING SENSORS";case demo::DefinitionStatus::Ambiguous:return "PENDING CLARIFICATION";default:return "RESERVED";}
}
inline size_t requiredChannels(const demo::FunctionDefinition &f){
  std::vector<const char*> names;
  for(size_t p=0;p<f.phaseCount;++p)for(size_t a=0;a<f.phases[p].count;++a){const char *name=f.phases[p].actions[a].component;bool seen=false;for(const auto *n:names)if(demo::sameName(n,name))seen=true;if(!seen)names.push_back(name);}
  return names.size();
}
inline std::string functionsJson(){
  std::string out="{\"functions\":[";
  for(size_t i=0;i<demo::FUNCTION_COUNT;++i){if(i)out+=',';const auto &f=demo::functions[i];demo::BenchProfile p;
    bool capacity=f.status==demo::DefinitionStatus::RelayDemo&&p.build(f,engine.benchMask()&masterIO.configuredMask());
    unsigned available=0;for(unsigned n=0;n<5;++n)if((engine.benchMask()&masterIO.configuredMask())&(1U<<n))available+=integrated::channelCount(integrated::ownerForIndex(uint8_t(n)));
    out+="{\"capacity_error\":"+std::string(f.status==demo::DefinitionStatus::RelayDemo&&!capacity?"true":"false")+",\"available_channels\":"+std::to_string(available)+",\"real_executable\":false,\"real_blocked_reason\":\"Loaded operation disabled; relay interface, startup and safety validation pending\",\"id\":"+quote(f.id)+",\"title\":"+quote(f.title)+",\"status\":"+quote(definitionStatus(f.status))+",\"executable\":"+(capacity?"true":"false")+",\"note\":"+quote(f.note)+",\"required_channels\":"+std::to_string(requiredChannels(f))+",\"phases\":[";
    for(size_t phase=0;phase<f.phaseCount;++phase){if(phase)out+=',';const auto &s=f.phases[phase];
      out+="{\"name\":"+quote(s.name)+",\"seconds\":"+std::to_string(engine.duration(i,phase)/1000)+",\"actions\":[";
      for(size_t a=0;a<s.count;++a){if(a)out+=',';out+="{\"component\":"+quote(s.actions[a].component)+",\"on\":"+(s.actions[a].on?"true":"false")+"}";}out+="]}";
    }out+="]}";
  }return out+"]}";
}
inline std::string componentsJson(){
  std::string out="{\"components\":[";bool first=true;
  for(const auto &c:demo::components){if(!first)out+=',';first=false;
    out+="{\"component\":"+quote(c.name)+",\"category\":"+quote(c.category)+",\"node_id\":"+std::to_string(integrated::ownerForIndex(c.node))+",\"output_index\":"+std::to_string(c.channel)+",\"gpio\":"+(c.gpio<0?"null":std::to_string(c.gpio))+",\"driver\":"+quote(c.driver==1?"PROPOSED DirectGPIO":"UNSUPPORTED")+",\"address\":"+(c.address?std::to_string(c.address):"null")+",\"driver_channel\":"+std::to_string(c.driverChannel)+",\"relay_module\":"+std::to_string(c.module)+",\"relay_input\":"+std::to_string(c.relayInput)+",\"assignment_status\":\"PROPOSED\",\"installed\":false,\"mapping_verified\":false,\"driver_available\":false,\"commanded_on\":null}";

  }return out+"]}";
}
inline Reply reply(int code,const char *message){return {code,"{\"message\":"+quote(message)+"}"};}
inline Reply handle(const Request &r){
  Fields f;if(r.method=="POST"&&!form(r.body,f))return reply(400,"Malformed or duplicate fields");
  if(r.path=="/api/connect"&&r.method=="POST"){
    // Knowledge of the private WPA Wi-Fi password authorizes operator access.
    // Only a socket accepted on the AP interface can enroll. No admin access,
    // output arming, profile loading or relay commands occur during connection.
    if(!r.apClient)return reply(403,"Join TubaTech_Master Wi-Fi first");
    return {200,"{\"direct_wifi_access\":true,\"machine\":\"TubaTech\",\"master_mac\":\"38:18:2B:8A:9D:30\",\"catalog_version\":\"final-66-2026-10-19\",\"token\":"+quote(access.operatorToken)+",\"role\":\"operator\"}"};
  }
  if(r.path=="/api/pair"&&r.method=="POST"){
    Role role=access.redeem(get(f,"code"),millis());
    if(role==Role::None)return reply(403,"Physical Serial pairing code invalid, expired, consumed or locked");
    return {200,"{\"token\":"+quote(role==Role::Admin?access.adminToken:access.operatorToken)+",\"role\":"+quote(role==Role::Admin?"admin":"operator")+",\"boot_id\":"+std::to_string(sessionID)+"}"};
  }
  // Joining the private machine AP is sufficient for demo control. Admin
  // calibration retains its separate credential; no operator enrollment gate.
  const Role role=r.apClient&&access.role(r.bearer)!=Role::Admin?Role::Operator:access.role(r.bearer);
  if(role==Role::None)return reply(401,"Join TubaTech_Master Wi-Fi first");
  if(r.method=="GET"){
    if(r.path=="/api/status")return {200,statusJson()};
    if(r.path=="/api/functions")return {200,functionsJson()};
    if(r.path=="/api/sensors")return {200,sensors::json()};
    if(r.path=="/api/components")return {200,componentsJson()};
    return reply(404,"Unknown endpoint");
  }
  if(r.method!="POST")return reply(405,"Method not allowed");
  if(r.path=="/api/stop"){++stopEpoch;if(!stopEpoch)++stopEpoch;requestStop();return reply(202,"STOP accepted; wait for acknowledged shutdown status");}
  if(r.path=="/api/mode"||(!get(f,"mode").empty()&&get(f,"mode")!="BENCH"))return reply(403,"Real hardware mode is disabled");
  if(r.path=="/api/sensors/calibration"&&role!=Role::Admin)return reply(403,"Admin pairing required");
  if(channelTest.busy())return reply(409,"Unloaded individual channel test active; STOP remains available");
  if(!access.fresh(f,sessionID))return reply(409,"Stale boot, missing request ID or replay; refresh status");
  if(r.path=="/api/reset"){
    if(engine.busy())return reply(409,"Shutdown or operation active");
    beginRecovery();return reply(202,"RESET verifying fresh sessions; no automatic restart");
  }
  if(recovering)return reply(409,"RESET still verifying nodes");
  if(r.path=="/api/bench/arm"){
    if(get(f,"unloaded")!="true")return reply(400,"Confirm all relay contacts are unloaded");
    if(engine.busy()||engine.faulted()||masterIO.unsafeNodes()||!localBank.linked||localBank.commanded)
      return reply(409,"Require idle, fault-free, verified local OFF session; no unsafe outputs");
    if(!localBank.arm())return reply(409,"Local OFF verification unavailable");
    engine.armBench();
    return {200,statusJson()};
  }
  if(r.path=="/api/profiles/load"){
    if(!engine.loadProfile(demo::functionIndex(get(f,"function").c_str()),millis()))return reply(409,"Profile blocked: physical BENCH ARM, idle, ready/OFF, supported function and capacity required");
    return {200,statusJson()};
  }
  uint32_t revision=0;
  if(r.path=="/api/profiles/confirm"){
    if(!number(get(f,"revision"),revision)||!engine.confirmProfile(revision,get(f,"unloaded")=="true",millis()))return reply(409,"Confirm current profile revision, unloaded contacts and ready/OFF state");
    return {200,statusJson()};
  }
  const std::string prefix="/api/functions/",suffix="/start";
  if(r.path.compare(0,prefix.size(),prefix)==0&&r.path.size()>prefix.size()+suffix.size()&&r.path.compare(r.path.size()-suffix.size(),suffix.size(),suffix)==0){
    uint32_t epoch=0;
    if(!number(get(f,"stop_epoch"),epoch)||epoch!=stopEpoch)
      return reply(409,"START cancelled by STOP or stale status; refresh before a new START");
    int index=demo::functionIndex(r.path.substr(prefix.size(),r.path.size()-prefix.size()-suffix.size()).c_str());
    const bool direct=r.apClient||get(f,"direct")=="true";
    if(direct?!engine.startDemo(index,millis()):(!number(get(f,"revision"),revision)||revision!=engine.profile().revision||!engine.start(index,millis())))
      return reply(409,"START blocked: required controllers must respond with outputs OFF; check pending function, active operation or latched fault");
    return reply(202,"START accepted; follow actual Master state");
  }
  if(r.path=="/api/demo/duration"){
    uint32_t seconds=0,phase=0;
    if(!number(get(f,"seconds"),seconds)||!number(get(f,"phase"),phase)||!engine.setTime(demo::functionIndex(get(f,"function").c_str()),seconds,phase))return reply(400,"Use supported function, zero-based phase and 1..3600 seconds while inactive and fault-free");
    return reply(200,"Bench duration updated in RAM");
  }
  if(r.path=="/api/sensors/calibration"){
    if(engine.busy()||engine.faulted()||!sensors::saveDraft(f))return reply(400,"Invalid calibration draft, active process or fault");
    return reply(200,"Unvalidated calibration draft stored in RAM; sensor remains NOT CALIBRATED");
  }
  return reply(404,"Unknown endpoint");
}
} // namespace local
