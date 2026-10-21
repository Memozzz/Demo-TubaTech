
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "component_registry.h"
#include "sequence_engine.h"
#include "node_config.h"
#include "relay_bank.h"
#include "station_identity.h"
#include "radio_diagnostics.h"
integrated::RelayBank localBank(0);

const uint8_t CHANNEL = 6;
// Versioned packet constants/layout are in relay_protocol.h.
const unsigned long HEARTBEAT_INTERVAL = 800;
const unsigned long ACK_TIMEOUT = 1200;

struct Node {
  const char *name;
  uint8_t mac[6];
  uint32_t sequence = 0;
  uint32_t pendingSequence = 0;
  unsigned long sentAt = 0;
  unsigned long lastHeartbeatSent = 0;
  unsigned long lastACK = 0;
  uint16_t stateMask = 0;
  bool outputsArmed=false;
  bool waiting = false;
  bool synced = false, isolated = false, confirmNeeded = false;
  uint8_t pendingType = 0; uint16_t expectedMask = 0;
  uint8_t pendingChannel=0;
  uint16_t uncertainMask=0, acceptableUncertainty=0;
  uint32_t completed = 0, failed = 0, nonce = 0;
  bool installed=true;uint32_t retryAt=0;
};

Node nodes[] = {
  {"SECONDARY",
   {0x68, 0x25, 0xDD, 0xF1, 0xA3, 0x78}},
  {"TETRIARY",
   {0x58, 0x2A, 0xBD, 0xD3, 0x08, 0xF4}},
  {"SLAVE 3",{0x78,0x1C,0x3C,0xA5,0x50,0x80}},
  {"SLAVE 4",{0x38,0x3E,0x51,0xD4,0x6A,0x30}},
  {"MASTER LOCAL",{0x38,0x18,0x2B,0x8A,0x9D,0x30}}
};
static_assert(sizeof(nodes) / sizeof(nodes[0]) == demo::ACTIVE_NODES,
              "Registry and enabled ESP-NOW peers must agree");

uint32_t sessionID;
bool radioReady = false;

struct Received {
  uint8_t sender[6];
  Packet packet;
};

QueueHandle_t rxQueue = nullptr;
volatile bool rxOverflow = false;
String serialLine;
bool serialOverflow = false, recovering = false;

struct MasterIO : demo::SequenceIO {
  uint8_t configuredMask() const override;
  uint8_t unsafeNodes() const override;
  bool ready(uint8_t node, uint32_t now) const override;
  bool allOffKnown(uint8_t node) const override;
  demo::SendResult relay(uint8_t node, uint8_t channel, bool on, uint32_t &ticket) override;
  demo::SendResult allOff(uint8_t node, uint32_t &ticket) override;
  demo::AckResult acknowledgement(uint8_t node, uint32_t ticket) const override;
  void cancelPending() override;
  void quarantine(uint8_t node, uint32_t now) override;
  bool quarantined(uint8_t node) const override;
  void report(const char *message) override { Serial.println(message); }
};
MasterIO masterIO;
demo::SequenceEngine engine(masterIO);

void beginRecovery();
bool provisionSlave4(const char *text);
#include "channel_test.h"
#include "master_api.h"
#include "local_wifi.h"


void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {
  radio_diag::observeRx(info,data,len);
  if (!info || !data || len != sizeof(Packet) || !rxQueue) return;
  bool known=false;for(unsigned i=0;i<4;++i)if(!memcmp(info->src_addr,nodes[i].mac,6))known=true;
  if(!known)return;

  Received r;
  memcpy(r.sender, info->src_addr, 6);
  memcpy(&r.packet, data, sizeof(Packet));

  if (!validEnvelope(r.packet)) return;
  radio_diag::validRx();
  if (xQueueSend(rxQueue, &r, 0) != pdTRUE) rxOverflow = true;
}

bool sendCommand(int nodeIndex, uint8_t type,
                 uint8_t channel = 0, uint8_t value = 0) {
  if (!radioReady || nodeIndex < 0 || nodeIndex >= int(demo::ACTIVE_NODES)) return false;
  Node &n = nodes[nodeIndex];
  if (!n.installed || n.waiting || n.isolated || n.sequence == UINT32_MAX) return false;
  if(type==SET_RELAY&&value&&n.uncertainMask)return false;
  if (type == SET_RELAY && (!n.synced || channel >= 14 || !(integrated::bankMask(integrated::ownerForIndex(uint8_t(nodeIndex)))&(1U<<channel)) || value > 1)) return false;
  Packet p = {MAGIC, sessionID, ++n.sequence, type, channel, value, 0};
  if (type == CONFIRM) {
    // CONFIRM answers the exact SYNC ticket, not a newly allocated command.
    --n.sequence; p.sequence = n.pendingSequence;
    p.channel = uint8_t(n.nonce); p.value = uint8_t(n.nonce >> 8); p.stateMask = uint8_t(n.nonce >> 16);
  }
  p.owner=integrated::ownerForIndex(uint8_t(nodeIndex));
  if(nodeIndex==4){
    Packet reply=localBank.receive(p,millis());Received received;memcpy(received.sender,n.mac,6);received.packet=reply;
    if(xQueueSend(rxQueue,&received,0)!=pdTRUE){rxOverflow=true;return false;}
  }else {
    int result=esp_now_send(n.mac,(uint8_t *)&p,sizeof(p));
    radio_diag::sendResult(nodeIndex,result);
    if(result!=ESP_OK)return false;
  }
  n.pendingSequence = p.sequence; n.pendingType = type; n.sentAt = millis(); n.waiting = true;
  n.pendingChannel=channel;n.acceptableUncertainty=n.uncertainMask;
  n.expectedMask = n.stateMask;
  if (type == SET_RELAY) {
    n.acceptableUncertainty&=uint16_t(~(1U<<channel));
    if (value) n.expectedMask |= uint16_t(1U << channel);
    else n.expectedMask &= uint16_t(~(1U << channel));
  } else if (type == ALL_OFF || type == SYNC || type == CONFIRM) {n.expectedMask = 0;n.acceptableUncertainty=0;}
  n.lastHeartbeatSent = n.sentAt; // Any fresh accepted packet refreshes node watchdog.
  if (type != HEARTBEAT) Serial.printf("[%s] SENT type=%u seq=%lu\n", n.name, type, (unsigned long)p.sequence);
  return true;
}

uint8_t MasterIO::configuredMask() const {uint8_t mask=0;for(unsigned n=0;n<demo::ACTIVE_NODES;++n)if(nodes[n].installed)mask|=uint8_t(1U<<n);return mask;}
uint8_t MasterIO::unsafeNodes() const {uint8_t mask=0;for(unsigned n=0;n<demo::ACTIVE_NODES;++n)if(nodes[n].installed&&(nodes[n].isolated||nodes[n].stateMask||(nodes[n].waiting&&nodes[n].pendingType==SET_RELAY)))mask|=uint8_t(1U<<n);return mask;}
bool MasterIO::ready(uint8_t node, uint32_t now) const {
  const Node &n = nodes[node];
  return n.installed && n.outputsArmed && n.synced && !n.isolated && uint32_t(now-n.lastACK) < 2500;
}
bool MasterIO::allOffKnown(uint8_t node) const {
  const Node &n = nodes[node];
  return n.installed && n.synced && n.stateMask == 0 && (!n.waiting || n.pendingType == HEARTBEAT);
}
demo::SendResult MasterIO::relay(uint8_t node, uint8_t channel, bool on, uint32_t &ticket) {
  Node &n = nodes[node];
  if (n.isolated || !n.synced) return demo::SendResult::Failed;
  if (n.waiting) return demo::SendResult::Busy;
  if (!sendCommand(node, SET_RELAY, channel, on)) return demo::SendResult::Failed;
  ticket = n.pendingSequence; return demo::SendResult::Sent;
}
demo::SendResult MasterIO::allOff(uint8_t node, uint32_t &ticket) {
  Node &n = nodes[node];
  if (n.isolated) return demo::SendResult::Failed;
  if (n.waiting) return demo::SendResult::Busy;
  if (!sendCommand(node, ALL_OFF)) return demo::SendResult::Failed;
  ticket = n.pendingSequence; return demo::SendResult::Sent;
}
demo::AckResult MasterIO::acknowledgement(uint8_t node, uint32_t ticket) const {
  const Node &n = nodes[node];
  if (n.failed == ticket || n.isolated) return demo::AckResult::Failed;
  if (n.completed == ticket) return demo::AckResult::Verified;
  return demo::AckResult::Pending;
}
void MasterIO::cancelPending() {
  for (Node &n : nodes) {
    if (n.waiting) {
      n.failed = n.pendingSequence;
      // STOP retires the ticket, not the possible execution of that command.
      // A later OFF/heartbeat fences it by sequence. Only this one known bit
      // may differ in that ACK; an OFF target and final ALL_OFF are always exact.
      if(n.pendingType==SET_RELAY)n.uncertainMask|=uint16_t(1U<<n.pendingChannel);
    }
    n.waiting = false;
  }
}
void MasterIO::quarantine(uint8_t node, uint32_t) {
  Node &n = nodes[node];
  if (n.waiting) n.failed = n.pendingSequence;
  n.waiting = false;
  // Best effort priority OFF, then NO further packets: local watchdog is backup.
  if (n.installed && !n.isolated && n.sequence != UINT32_MAX && radioReady) {
    Packet off = {MAGIC, sessionID, ++n.sequence, ALL_OFF, 0, 0, 0};
    off.owner=integrated::ownerForIndex(node);
    if(node==4)localBank.disable();else esp_now_send(n.mac, (const uint8_t *)&off, sizeof(off));
  }
  n.isolated = true; n.synced = false; n.confirmNeeded = false;
  Serial.printf("[%s] QUARANTINED; OFF attempted, heartbeats disabled\n", n.name);
}
bool MasterIO::quarantined(uint8_t node) const { return nodes[node].isolated; }

void networkFault(int node, const char *reason,bool connectivity=false) {
  Node &n=nodes[node];
  if(connectivity&&!engine.ownsNode(uint8_t(node))&&!n.stateMask&&n.pendingType!=SET_RELAY&&n.pendingType!=ALL_OFF){
    n.failed=n.pendingSequence;n.waiting=n.synced=n.confirmNeeded=false;n.retryAt=millis()+2000;
    Serial.printf("[%s] OFFLINE (unused); other operations remain available\n",n.name);return;
  }
  if(!engine.ownsNode(uint8_t(node)))masterIO.quarantine(uint8_t(node),millis());
  nodes[node].failed = nodes[node].pendingSequence;
  nodes[node].waiting = false;
  recovering = false;
  Serial.printf("[%s] %s\n", nodes[node].name, reason);
  // Don't restart shutdown on every error; retain the original FAULT reason.
  engine.fail(reason, millis());
}

void processReplies() {
  Received r;
  for (int count=0; count<20 && rxQueue && xQueueReceive(rxQueue, &r, 0) == pdTRUE; ++count) {
    for (uint8_t i = 0; i < demo::ACTIVE_NODES; ++i) {
      Node &n = nodes[i];
      if (!n.installed || memcmp(r.sender, n.mac, 6) != 0 || r.packet.session != sessionID ||
          !n.waiting || r.packet.sequence != n.pendingSequence || n.isolated || r.packet.owner!=integrated::ownerForIndex(i)) continue;
      if (r.packet.type == CHALLENGE && n.pendingType == SYNC) {
        n.nonce = uint32_t(r.packet.channel) | (uint32_t(r.packet.value)<<8) | (uint32_t(r.packet.stateMask)<<16);
        if (!n.nonce) { networkFault(i, "FAULT: invalid session challenge"); continue; }
        n.waiting = false; n.confirmNeeded = true;
        continue;
      }
      if (r.packet.type == REJECT) {
        // A rebooted, unused receiver reports OFF and needs a fresh handshake.
        // Required-node rejection, nonzero outputs or invalid identity still fault.
        bool cleanOffline=r.packet.channel==integrated::ownerForIndex(i)&&r.packet.value==1&&r.packet.stateMask==0;
        networkFault(i, "Receiver rejected session/command; fresh OFF handshake required",cleanOffline);continue;
      }
      if (r.packet.type != ACK || n.pendingType == SYNC) continue;
      // Full commanded mask AND node identity, not radio delivery status.
      if (r.packet.channel != integrated::ownerForIndex(i) || r.packet.value != 0 ||
          ((r.packet.stateMask^n.expectedMask)&uint16_t(~n.acceptableUncertainty)) ||
          (r.packet.stateMask&uint16_t(~integrated::bankMask(integrated::ownerForIndex(i))))) {
        networkFault(i, "FAULT: ACK output mask or node identity mismatch"); continue;
      }
      n.waiting = false; n.lastACK = millis(); n.stateMask = r.packet.stateMask;
      n.uncertainMask=n.acceptableUncertainty=0;
      n.completed = r.packet.sequence; n.outputsArmed=(r.packet.flags&1)!=0;
      if (n.pendingType == CONFIRM) { n.synced = true; n.confirmNeeded = false; }
      if (n.pendingType != HEARTBEAT) Serial.printf("[%s] VERIFIED ACK #%lu mask=0x%02X\n",
        n.name, (unsigned long)r.packet.sequence, n.stateMask);
    }
  }
}

void printStatus() {
  Serial.println(local::statusJson().c_str());
}

void checkWB() {
  demo::BenchProfile preview;preview.build(demo::functions[demo::functionIndex("W_B")]);
  for(size_t i=0;i<preview.count;++i){const auto &c=preview.slots[i];Serial.printf("%s -> %s IN%u GPIO%d\n",c.name,nodes[c.node].name,c.relayInput,c.gpio);}
  Serial.println("Preview only. BENCH ARM UNLOADED, PROFILE W_B, CONFIRM <revision> UNLOADED, START W_B.");
}

void printList() {
  for (size_t i=0; i<demo::FUNCTION_COUNT; ++i) {
    const auto &f = demo::functions[i];
    Serial.printf("%s | %s | %s\n",f.id,local::definitionStatus(f.status),f.note);
    for(size_t p=0;p<f.phaseCount;++p)Serial.printf(" phase %u %s: %lus\n",unsigned(p+1),f.phases[p].name,(unsigned long)(engine.duration(i,p)/1000));
  }
}

void beginRecovery() {
  if (engine.busy()||channelTest.busy()) { Serial.println("RESET blocked while operation/shutdown active"); return; }
  sessionID = esp_random(); if (!sessionID) sessionID = 1;
  for (Node &n : nodes) {
    n.waiting = n.synced = n.isolated = n.confirmNeeded = n.outputsArmed = false;
    n.sequence = n.pendingSequence = n.completed = n.failed = n.nonce = 0;
    n.uncertainMask=n.acceptableUncertainty=0;
    n.lastACK = n.lastHeartbeatSent = 0;n.retryAt=0;
  }
  localBank.disable();
  recovering = true;
  Serial.println("RESET: re-verifying fresh sessions and all outputs OFF; no automatic restart");
}

bool provisionSlave4(const char *){Serial.println("Fixed package uses confirmed Slave 4 MAC 38:3E:51:D4:6A:30; edit/rebuild configuration to replace hardware");return false;}

void handleCommand(String line) {
  line.trim(); line.toUpperCase();
  if (line == "STATUS") { printStatus(); return; }
  if (line == "RADIO") { radio_diag::report(); return; }
  if(line=="STOP"||line=="ALL OFF"){requestStop();return;}
  if(channelTest.busy()&&!(line=="OUTPUTS DISARM")){Serial.println("BUSY: channel test active; use STOP");return;}
  if(line.startsWith("TEST ")){if(!channelTest.start(line.substring(5).c_str(),millis()))Serial.println("TEST blocked: require unloaded ARM, verified owner, all outputs OFF, idle and no fault");return;}
  if (line == "LIST") { printList(); return; }
  if (line == "CHECK W_B") { checkWB(); return; }
  if (line == "STOP" || line == "ALL OFF") { recovering = false; engine.stop(millis()); return; }
  if (line == "RESET") { beginRecovery(); return; }
  if(line.startsWith("PROVISION 4 ")){if(!provisionSlave4(line.substring(12).c_str()))Serial.println("Provision rejected: valid unique physical MAC and inactive verified OFF state required");return;}
  if(line.startsWith("BENCH MASK ")){uint32_t mask=0;if(!local::number(line.substring(11).c_str(),mask)||mask>31||!engine.configureBench(uint8_t(mask),millis()))Serial.println("BENCH MASK rejected: installed, ready/OFF nodes only, inactive and fault-free");return;}
  if(line=="OUTPUTS ARM UNLOADED"||line=="OUTPUTS ARM GPIO_ONLY"){Serial.println(localBank.arm()?"Master-local unloaded outputs armed":"ARM blocked: require verified local session and OFF");return;}
  if(line=="OUTPUTS DISARM"){localBank.disable();engine.fail("Master-local outputs disarmed",millis());return;}
  if(line=="IDENTITY"){Serial.printf("MASTER station MAC %s; expected 38:18:2B:8A:9D:30; fixed-v1; REAL DISABLED\n",WiFi.macAddress().c_str());return;}
  if(line=="PINS"){for(const auto &c:demo::components)Serial.printf("%s owner=%u CH%u GPIO%d R%02u IN%u\n",c.name,integrated::ownerForIndex(c.node),c.channel,c.gpio,c.module,c.relayInput);return;}
  if (line == "BENCH ARM UNLOADED") {if(!recovering&&localBank.arm())engine.armBench();else Serial.println("ARM blocked: local OFF session not verified");return;}
#ifndef TUBATECH_HOST_TEST
  if(line=="PAIR"||line=="PAIR ADMIN"){local::openPair(line=="PAIR ADMIN");return;}
#endif
  if(line.startsWith("PROFILE ")){
    if(recovering||!engine.loadProfile(demo::functionIndex(line.substring(8).c_str()),millis()))Serial.println("PROFILE blocked: require physical BENCH ARM, idle, verified nodes OFF, relay demo and capacity");
    printStatus();return;
  }
  if(line.startsWith("CONFIRM ")){
    unsigned long revision=0;char word[16]={},extra=0;
    if(sscanf(line.c_str(),"CONFIRM %lu %15s %c",&revision,word,&extra)!=2||strcmp(word,"UNLOADED")||recovering||!engine.confirmProfile(uint32_t(revision),true,millis()))Serial.println("CONFIRM rejected: use current revision and UNLOADED, with verified OFF nodes");
    else Serial.println("Profile confirmed; START authorized once for this revision");return;
  }
  if (line.startsWith("START ")) {
    if (recovering) { Serial.println("START blocked: RESET still verifying nodes"); return; }
    engine.startDemo(demo::functionIndex(line.substring(6).c_str()), millis()); return;
  }
  if (line.startsWith("TIME ")) {
    char name[16] = {}, extra = 0; unsigned long seconds = 0,phase=1;
    int fields=sscanf(line.c_str(),"TIME %15s %lu %lu %c",name,&seconds,&phase,&extra);
    if((fields==2||fields==3)&&phase>=1&&phase<=4&&seconds<=3600&&engine.setTime(demo::functionIndex(name),uint32_t(seconds),size_t(phase-1)))Serial.printf("%s phase %lu duration=%lus (RAM only)\n",name,phase,seconds);
    else Serial.println("TIME rejected: TIME <function> <1..3600 seconds> [1-based phase]; inactive/fault-free only");
    return;
  }
  Serial.println("Commands: BENCH ARM UNLOADED, PROFILE W_B, CONFIRM <revision> UNLOADED, START W_B, STOP, STATUS, LIST, TIME W_D 15 2, RESET, PAIR, PAIR ADMIN");
  Serial.println("Raw V1-V8 activation is retired: use named, confirmed profiles to avoid V2 identity collisions.");
}

void setup() {
  Serial.begin(115200);
  if(!integrated::stationIdentity(0)){Serial.println("MASTER MAC mismatch: outputs and radio locked out");return;}
  localBank.begin(); // Inactive GPIO levels before any Wi-Fi startup.

  #ifdef TUBATECH_HOST_TEST
  WiFi.mode(WIFI_STA);
  #else
  WiFi.setSleep(false);WiFi.setAutoReconnect(false);
  if(!WiFi.mode(WIFI_AP_STA)||!WiFi.STA.begin(false)){
    localBank.disable();Serial.println("FAULT: MASTER STA startup failed; outputs OFF");return;
  }
  #endif
  WiFi.disconnect();

#ifndef TUBATECH_HOST_TEST
  if(!local::beginWifi()){Serial.println("Local Wi-Fi initialization failed; outputs remain OFF");return;}
#endif

  Serial.printf("MASTER station MAC %s; expected 38:18:2B:8A:9D:30\n",WiFi.macAddress().c_str());
  for(uint8_t i=0;i<4;++i){nodes[i].installed=nodecfg::positions[i].installed;memcpy(nodes[i].mac,nodecfg::positions[i].mac,6);}
  if (esp_wifi_set_channel(CHANNEL,
                           WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("Channel error");
    return;
  }
#ifndef TUBATECH_HOST_TEST
  uint8_t actualMac[6]={},actualChannel=0;wifi_second_chan_t second=WIFI_SECOND_CHAN_NONE;
  if(esp_wifi_set_ps(WIFI_PS_NONE)!=ESP_OK||esp_wifi_get_mac(WIFI_IF_STA,actualMac)!=ESP_OK||
     memcmp(actualMac,integrated::macs[0],6)||esp_wifi_get_channel(&actualChannel,&second)!=ESP_OK||actualChannel!=CHANNEL){
    localBank.disable();radio_diag::report();Serial.println("FAULT: MASTER radio identity/channel verification failed; outputs OFF");return;
  }
#endif

  rxQueue = xQueueCreate(20, sizeof(Received));
  if (!rxQueue) {
    Serial.println("Queue error");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW error");
    return;
  }

  if (esp_now_register_recv_cb(onReceive) != ESP_OK) {
    Serial.println("Receive callback error");
    return;
  }
  if(!radio_diag::begin()) Serial.println("RADIO diagnostics unavailable; application ACKs remain required");

  for (uint8_t i = 0; i < demo::ACTIVE_NODES; i++) {
    if(i==4||!nodes[i].installed)continue;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, nodes[i].mac, 6);
    peer.channel = CHANNEL;
    peer.encrypt = false;
#ifndef TUBATECH_HOST_TEST
    peer.ifidx=WIFI_IF_STA;
#endif

    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.println("Peer error");
      return;
    }
  }

  sessionID = esp_random(); if (!sessionID) sessionID = 1;
  radioReady = true;
  radio_diag::report();

  Serial.println("TUBATECH INTEGRATED MASTER: fixed-v1, 66 unloaded outputs, REAL DISABLED");
  Serial.println("Unloaded relay demo: join TubaTech_Master, select function, START. STOP remains available.");
  Serial.println("START W_B uses fixed wiring and required-controller ACKs; no arming/profile confirmation step.");
}

void loop() {
  if (!radioReady) return;
  localBank.tick(millis());
  if (rxOverflow) {
    rxOverflow = false; engine.fail("FAULT: receive queue overflow", millis());
  }
  processReplies();
  // Bounded nonblocking serial input. Reject overlong lines rather than run a prefix.
  for (int count=0; count<64 && Serial.available(); ++count) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (serialOverflow) Serial.println("Command rejected: line too long");
      else if (serialLine.length()) handleCommand(serialLine);
      serialLine = ""; serialOverflow = false;
    } else if (serialLine.length() < 63) serialLine += c;
    else serialOverflow = true;
  }
  const uint32_t now = millis();
  for (uint8_t i=0; i<demo::ACTIVE_NODES; ++i) {
    Node &n = nodes[i];
    if (n.waiting && uint32_t(now-n.sentAt) >= ACK_TIMEOUT) networkFault(i, "FAULT: ACK TIMEOUT",true);
  }
  engine.tick(millis());
  channelTest.tick(millis());
  if (recovering) {
    bool verified = true;
    for (uint8_t i=0; i<demo::ACTIVE_NODES; ++i)
      if(engine.resetMask()&(1U<<i))verified = verified && masterIO.ready(i, millis()) && masterIO.allOffKnown(i);
    if (verified && engine.reset(millis())) recovering = false;
  }
  // Continues every loop, including while the sequence timer is running.
  for (uint8_t i=0; i<demo::ACTIVE_NODES; ++i) {
    Node &n = nodes[i];
    if (!n.installed || n.isolated || n.waiting || (n.retryAt&&int32_t(millis()-n.retryAt)<0)) continue;
    if (!n.synced) {
      if (engine.busy() || (engine.faulted() && !recovering)) continue;
      const uint8_t type = n.confirmNeeded ? CONFIRM : SYNC;
      if (!sendCommand(i, type)) networkFault(i, "FAULT: handshake send failed",true);
    } else if (uint32_t(millis()-n.lastHeartbeatSent) >= HEARTBEAT_INTERVAL) {
      if (!sendCommand(i, HEARTBEAT)) networkFault(i, "FAULT: heartbeat send failed",true);
    }
  }
#ifndef TUBATECH_HOST_TEST
  local::pollWifi();
#endif
}
