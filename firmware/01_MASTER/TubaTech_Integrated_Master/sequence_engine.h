#pragma once
#include "bench_profiles.h"
#include <initializer_list>

namespace demo {
enum class SendResult { Sent, Busy, Failed };
enum class AckResult { Pending, Verified, Failed };
enum class Stage { Idle, ValvesOn, PumpsOn, Running, PumpsOff, AllOff, Complete, Stopped, Fault };

// Network adapter contract. Safety cancellation must retire pending tickets;
// quarantined nodes receive no heartbeat so their local watchdog can expire.
struct SequenceIO {
  virtual ~SequenceIO() = default;
  virtual uint8_t configuredMask() const { return DEFAULT_BENCH_MASK; }
  virtual uint8_t unsafeNodes() const { return 0; }
  virtual bool ready(uint8_t node, uint32_t now) const = 0;
  virtual bool allOffKnown(uint8_t node) const = 0;
  virtual SendResult relay(uint8_t node, uint8_t channel, bool on, uint32_t &ticket) = 0;
  virtual SendResult allOff(uint8_t node, uint32_t &ticket) = 0;
  virtual AckResult acknowledgement(uint8_t node, uint32_t ticket) const = 0;
  virtual void cancelPending() = 0;
  virtual void quarantine(uint8_t node, uint32_t now) = 0;
  virtual bool quarantined(uint8_t node) const = 0;
  virtual void report(const char *message) = 0;
};

class SequenceEngine {
public:
  explicit SequenceEngine(SequenceIO &io) : io_(io) {
    for (size_t i=0; i<FUNCTION_COUNT; ++i) for(size_t p=0;p<functions[i].phaseCount&&p<MAX_PHASES;++p) durations_[i][p] = functions[i].phases[p].defaultMs;
  }
  Stage stage() const { return stage_; }
  bool faulted() const { return fault_; }
  bool busy() const {
    return stage_ == Stage::ValvesOn || stage_ == Stage::PumpsOn || stage_ == Stage::Running || shuttingDown();
  }
  bool shuttingDown() const { return stage_ == Stage::PumpsOff || stage_ == Stage::AllOff; }
  const char *activeName() const { return active_ >= 0 ? functions[active_].id : "none"; }
  const char *reason() const { return reason_; }
  const BenchProfile &profile() const { return profile_; }
  bool benchArmed() const { return benchArmed_; }
  uint8_t benchMask() const {return benchMask_;}
  void disarmBench(){benchArmed_=false;profile_.confirmed=false;}
  bool configureBench(uint8_t mask,uint32_t now){
    if(busy()||fault_||!mask||(mask&~io_.configuredMask())||io_.unsafeNodes()||!readyOff(now,mask))return false;
    uint32_t revision=profile_.revision+1;profile_=BenchProfile{};profile_.revision=revision;benchMask_=mask;disarmBench();return true;
  }
  void armBench() { if(!busy() && !fault_) {benchArmed_=true;io_.report("BENCH armed: unloaded contacts only; confirm wiring before START");} }
  uint8_t requiredMask() const {return profile_.requiredMask;}
  uint8_t resetMask() const {return fault_?uint8_t(profile_.requiredMask|shutdownMask_):io_.unsafeNodes();}
  bool ownsNode(uint8_t node) const {return busy()&&((shuttingDown()?shutdownMask_:profile_.requiredMask)&(1U<<node));}
  bool readyOff(uint32_t now,uint8_t mask=0) const {
    if(!mask)mask=profile_.requiredMask;
    for(uint8_t n=0;n<ACTIVE_NODES;++n) if((mask&(1U<<n))&&(!io_.ready(n,now)||!io_.allOffKnown(n)))return false;
    return true;
  }
  bool loadProfile(int index,uint32_t now) {
    if(busy()||fault_||!benchArmed_||index<0||size_t(index)>=FUNCTION_COUNT)return false;
    const auto &f=functions[index];
    if(f.status!=DefinitionStatus::RelayDemo){io_.report(f.note);return false;}
    if(!f.phases||!f.phaseCount||f.phaseCount>MAX_PHASES)return false;
    BenchProfile candidate;
    if(!candidate.build(f,io_.configuredMask()&benchMask_)){io_.report("CAPACITY/MAPPING ERROR: requires more than the configured independent bench channels or has unknown names");return false;}
    if(!readyOff(now,candidate.requiredMask)){io_.report("BLOCKED: required node missing or outputs not verified OFF");return false;}
    candidate.function=index;candidate.revision=profile_.revision+1;
    if(!candidate.revision)candidate.revision=1;
    profile_=candidate;
    io_.report("Profile loaded without changing outputs; confirm this revision before START");return true;
  }
  bool confirmProfile(uint32_t revision,bool unloaded,uint32_t now) {
    if(busy()||fault_||!benchArmed_||!unloaded||profile_.function<0||revision!=profile_.revision||!readyOff(now))return false;
    profile_.confirmed=true;for(size_t i=0;i<profile_.count;++i)profile_.slots[i].mappingVerified=true;return true;
  }
  size_t phaseIndex() const {return phase_;}
  const char *phaseName() const {return active_>=0 && phase_<functions[active_].phaseCount?functions[active_].phases[phase_].name:"none";}
  uint32_t duration(size_t index,size_t phase=0) const { return index<FUNCTION_COUNT&&phase<MAX_PHASES?durations_[index][phase]:0; }
  uint32_t remainingMs(uint32_t now) const {
    if(stage_!=Stage::Running||active_<0)return 0;
    uint32_t elapsed=uint32_t(now-runAt_),total=duration(active_,phase_);return elapsed>=total?0:total-elapsed;
  }
  bool setTime(int index, uint32_t seconds, size_t phase=0) {
    if (index < 0 || static_cast<size_t>(index) >= FUNCTION_COUNT || busy() || fault_ ||
        phase>=functions[index].phaseCount || phase>=MAX_PHASES || functions[index].status != DefinitionStatus::RelayDemo || seconds < 1 || seconds > 3600) return false;
    durations_[index][phase] = seconds * 1000;
    return true;
  }
  bool start(int index, uint32_t now) {
    if (busy() || fault_) { io_.report("BLOCKED: active operation, shutdown, or latched FAULT"); return false; }
    if (index < 0 || static_cast<size_t>(index) >= FUNCTION_COUNT) { io_.report("Unknown function"); return false; }
    const auto &f = functions[index];
    if (f.status != DefinitionStatus::RelayDemo) {
      io_.report("PENDING: sensor integration/calibration or source clarification required");
      io_.report(f.note); return false;
    }
    if(!benchArmed_ || profile_.function!=index || !profile_.confirmed ||
       !validRegistry(profile_.slots,profile_.count,ACTIVE_NODES)) {
      io_.report("BLOCKED: load and confirm an unloaded bench profile first");return false;
    }
    // Exclusive prototype: all enabled nodes must be ready and all outputs OFF.
    for (uint8_t n=0; n<ACTIVE_NODES; ++n) {
      if ((profile_.requiredMask&(1U<<n))&&(!io_.ready(n, now) || !io_.allOffKnown(n))) {
        fail("START verification failed: node unavailable or outputs not OFF", now); return false;
      }
    }
    active_ = index; fault_ = false; reason_ = "none"; stopped_ = false;
    stage_ = Stage::ValvesOn; phase_=0; profile_.confirmed=false; cursor_ = 0; pending_ = false; phaseAt_ = now;
    io_.report("EXECUTING PHASE ACTIONS (application ACK required)");
    return true;
  }
  // Fixed, dedicated wiring: one explicit START prepares the existing engine.
  // No GPIO is changed by preparation. Only this function's owners must be OFF
  // and freshly acknowledged. The existing runner still gates every output ACK.
  bool startDemo(int index, uint32_t now) {
    if(busy()||fault_||index<0||size_t(index)>=FUNCTION_COUNT)return false;
    BenchProfile candidate;
    if(!candidate.build(functions[index],io_.configuredMask()&benchMask_)||
       !readyOff(now,candidate.requiredMask)) {
      io_.report("START blocked: required controller unavailable, outputs not OFF, or function unmapped/pending");
      return false;
    }
    candidate.function=index;candidate.revision=profile_.revision+1;
    if(!candidate.revision)candidate.revision=1;
    candidate.confirmed=true;
    profile_=candidate;benchArmed_=true;
    return start(index,now);
  }
  void stop(uint32_t now) {
    if (shuttingDown()) return;
    stopped_ = true;
    beginShutdown(now);
    io_.report("STOP requested; stopping pump relays before closing valves");
  }
  void fail(const char *reason, uint32_t now) {
    if (!fault_) { reason_ = reason; io_.report(reason); }
    fault_ = true; benchArmed_=false; profile_.confirmed=false;
    if (!shuttingDown()) beginShutdown(now);
  }
  bool reset(uint32_t now) {
    if (busy()) { io_.report("RESET blocked: shutdown/operation still active"); return false; }
    for(uint8_t n=0;n<ACTIVE_NODES;++n)if((resetMask()&(1U<<n))&&(!io_.ready(n,now)||!io_.allOffKnown(n)))return false;
    fault_ = false; benchArmed_=false; profile_.confirmed=false; stage_ = Stage::Idle; active_ = -1; reason_ = "none";
    io_.report("RESET verified: IDLE, required nodes ready and OFF");
    return true;
  }
  void tick(uint32_t now) {
    if (!busy()) return;
    if (shuttingDown()) { shutdownTick(now); return; }
    for (uint8_t n=0; n<ACTIVE_NODES; ++n) {
      if ((profile_.requiredMask&(1U<<n))&&!io_.ready(n, now)) { fail("FAULT: node readiness lost", now); return; }
    }
    if (stage_ == Stage::Running) {
      if (static_cast<uint32_t>(now-runAt_) >= duration(active_,phase_)) {
        if(++phase_<functions[active_].phaseCount){stage_=Stage::ValvesOn;cursor_=0;phaseAt_=now;io_.report("NEXT PHASE");}
        else beginShutdown(now);
      }
      return;
    }
    if (pending_) {
      const auto ack = io_.acknowledgement(pendingNode_, ticket_);
      if (ack == AckResult::Failed || static_cast<uint32_t>(now-phaseAt_) >= STEP_TIMEOUT) {
        fail("FAULT: sequence output ACK missing or failed verification", now); return;
      }
      if (ack == AckResult::Pending) return;
      pending_ = false; ++cursor_; phaseAt_ = now;
    }
    const auto &phase=functions[active_].phases[phase_];
    while(cursor_<phase.count){
      const auto &action=phase.actions[cursor_];
      if(action.kind==ActionKind::WaitForSensor){fail("FAULT: sensor condition is not integrated/calibrated",now);return;}
      const Component *c=profile_.findName(action.component);
      if(!c){fail("FAULT: missing required component; no action may be skipped",now);return;}
      issue(*c,action.on,now);return;
    }
    stage_=Stage::Running;runAt_=now;io_.report("RUNNING PHASE: timer starts after action ACKs");
  }

  static const char *stageName(Stage s) {
    switch (s) {
      case Stage::Idle: return "IDLE"; case Stage::ValvesOn: return "PHASE_ACTIONS";
      case Stage::PumpsOn: return "STARTING_PUMPS"; case Stage::Running: return "RUNNING";
      case Stage::PumpsOff: return "STOPPING_PUMPS"; case Stage::AllOff: return "CLOSING_VALVES";
      case Stage::Complete: return "COMPLETE"; case Stage::Stopped: return "STOPPED";
      case Stage::Fault: return "FAULT";
    }
    return "UNKNOWN";
  }
private:
  static constexpr uint32_t STEP_TIMEOUT = 2000;
  // Beyond the receiver's 2500 ms local watchdog, measured after last send was fenced.
  static constexpr uint32_t WATCHDOG_GRACE = 2800;
  SequenceIO &io_;
  Stage stage_ = Stage::Idle;
  int active_ = -1;
  static constexpr size_t MAX_PHASES=4;
  uint32_t durations_[FUNCTION_COUNT][MAX_PHASES] = {};
  BenchProfile profile_;
  size_t phase_=0;
  bool benchArmed_=false;
  uint8_t benchMask_=DEFAULT_BENCH_MASK;
  uint32_t phaseAt_ = 0, runAt_ = 0, shutdownAt_ = 0, ticket_ = 0;
  size_t cursor_ = 0;
  uint8_t pendingNode_ = 0,shutdownMask_=0;
  bool pending_ = false, fault_ = false, stopped_ = false;
  const char *reason_ = "none";

  void beginShutdown(uint32_t now) {
    shutdownMask_=uint8_t(profile_.requiredMask|io_.unsafeNodes());
    if(!shutdownMask_)for(uint8_t n=0;n<ACTIVE_NODES;++n)if(io_.ready(n,now))shutdownMask_|=uint8_t(1U<<n);
    io_.cancelPending(); // Fences any outstanding ON before issuing newer OFF.
    stage_ = Stage::PumpsOff; cursor_ = 0; pending_ = false;
    shutdownAt_ = phaseAt_ = now;
  }
  void issue(const Component &c, bool on, uint32_t now) {
    const auto sent = io_.relay(c.node, c.channel, on, ticket_);
    if (sent == SendResult::Sent) { pending_ = true; pendingNode_ = c.node; phaseAt_ = now; }
    else if (sent == SendResult::Failed || static_cast<uint32_t>(now-phaseAt_) >= STEP_TIMEOUT)
      fail("FAULT: output send failed or stayed BUSY", now);
    // BUSY leaves cursor unchanged; the next loop retries without advancing.
  }
  void shutdownFailure(uint8_t node, uint32_t now) {
    if (!fault_) { reason_ = "FAULT: shutdown could not be acknowledged"; io_.report(reason_); }
    fault_ = true; benchArmed_=false; profile_.confirmed=false;
    // Adapter sends one priority ALL_OFF attempt, then silences this node.
    io_.quarantine(node, now);
    shutdownAt_ = now; pending_ = false; ++cursor_; phaseAt_ = now;
  }
  void shutdownTick(uint32_t now) {
    if (pending_) {
      const auto ack = io_.acknowledgement(pendingNode_, ticket_);
      if (ack == AckResult::Failed || static_cast<uint32_t>(now-phaseAt_) >= STEP_TIMEOUT) {
        shutdownFailure(pendingNode_, now); return;
      }
      if (ack == AckResult::Pending) return;
      pending_ = false; ++cursor_; phaseAt_ = now;
    }
    if (stage_ == Stage::PumpsOff) {
      while (cursor_ < profile_.count) {
        const auto &c = profile_.slots[cursor_];
        if (!mapped(c) || c.kind != Kind::Pump || io_.quarantined(c.node)) { ++cursor_; continue; }
        const auto sent = io_.relay(c.node, c.channel, false, ticket_);
        if (sent == SendResult::Sent) { pending_ = true; pendingNode_ = c.node; phaseAt_ = now; }
        else if (sent == SendResult::Failed || static_cast<uint32_t>(now-phaseAt_) >= STEP_TIMEOUT)
          shutdownFailure(c.node, now);
        return;
      }
      for (uint8_t n=0; n<ACTIVE_NODES; ++n)
        if ((shutdownMask_&(1U<<n)) && io_.quarantined(n) && static_cast<uint32_t>(now-shutdownAt_) < WATCHDOG_GRACE) return;
      stage_ = Stage::AllOff; cursor_ = 0; phaseAt_ = now;
      io_.report("CLOSING VALVES: verifying ALL OFF on each node");
    }
    while (cursor_ < ACTIVE_NODES) {
      const uint8_t node = static_cast<uint8_t>(cursor_);
      if (!(shutdownMask_&(1U<<node)) || io_.quarantined(node)) { ++cursor_; continue; }
      const auto sent = io_.allOff(node, ticket_);
      if (sent == SendResult::Sent) { pending_ = true; pendingNode_ = node; phaseAt_ = now; }
      else if (sent == SendResult::Failed || static_cast<uint32_t>(now-phaseAt_) >= STEP_TIMEOUT)
        shutdownFailure(node, now);
      return;
    }
    stage_ = fault_ ? Stage::Fault : stopped_ ? Stage::Stopped : Stage::Complete;
    io_.report(fault_ ? "FAULT LATCHED: shutdown attempted; RESET required" : stopped_ ? "STOPPED: all required outputs acknowledged OFF" : "COMPLETE: all required outputs acknowledged OFF");
  }
};
} // namespace demo
