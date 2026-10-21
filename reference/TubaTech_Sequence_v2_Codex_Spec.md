# TubaTech Sequence V2 — Arduino ESP32 + Flutter integration specification

Date: 2026-10-18. **User's latest notes supersede the old 27-function library.**

## Goals

- Preserve the existing, tested-in-software Arduino-ESP32 3.3.12 Master/Secondary/Tetriary ESP-NOW packet protocol, node MAC addresses, nonblocking ACK handling, watchdogs, fault latch, and safeguards.
- Implement a **bench demo mode** that allows testing *each* process function using only eight unloaded relay outputs on existing two receiver boards. Before each function, dynamically assign a unique demonstration channel to each *distinct actuated component used in that function*. For multi-stage function sequences, preserve mapping for the **entire function** (do not change component-to-relay identity while a function runs). Show the mapping in app and Serial Monitor. If more than eight distinct actuated components are needed across one function, reject it with a clear explanation or require a separate profile; do not alias concurrently used components. Keep all outputs OFF when assigning a new function. Function state machine may have sub-stages, transitions, ACK gates, adjustable simulated durations. No physical loads connected. Do not require users to physically rewire control pins between demos.
- Implement separately **real-hardware mode**, where each logical component has unique fixed verified node/channel assignments, missing components prevent execution. Never use bench aliases to actuate physical equipment. Require explicit service configuration and physical enable/interlock rather than just a phone mode toggle.
- Provide **Flutter app local control** by attaching HTTP/WebSocket API to actual ESP-NOW Master (not replacing sequencer with legacy `src/main.cpp`). Preserve ESP-NOW channel 6; softAP on channel 6 without station association that silently changes channel, or verify station same channel; don't do blocking long Wi-Fi operations in control loop. Use authenticated endpoints with device pairing; do not reuse hard-coded `[historical credential omitted]` legacy credentials, GET provisioning or public MQTT command execution. Separate device provisioning from authenticated operator commands. API: GET /api/status (active function and stage, countdown, per-component mapped node/channel and state, each-node heartbeat/fault); GET /api/functions; POST /api/functions/<id>/start; POST /api/stop; POST /api/demo/duration; GET /api/sensors; POST /api/sensors/calibration (authenticated admin; validated). Add Flutter services and connect existing buttons, don't create throwaway replacement UI. Unauthorized endpoints can't change outputs.
- Sensor placeholders now; expose quality/calibration flags and `not_installed`, `uncalibrated`, `invalid`, `valid` rather than fictitious values. pH, ethanol, ultrasound, temperatures each need board/node/channel location to be decided and actual calibration procedures from teammates. No real process pass/fail or dosing decisions without validated calibration and measurement technique. MQ-3 gas reading is not an accurate direct liquid ethanol percentage; cannot prove vinegar completion without titratable acidity.
- Separate transfer, CIP, aeration and dosing phases. Do not execute the real physical equipment based solely on arbitrary 30s timers. Ensure pump-off/valve-off ordering for regular stop; emergency mechanisms ultimately require hardwired interlocks and E-stop.

## Current known hardware

MASTER station MAC 38:18:2B:8A:9D:30. SECONDARY station MAC 68:25:DD:F1:A3:78; TETRIARY station MAC 58:2A:BD:D3:08:F4. All ESP-NOW channel 6. Secondary/Tetriary 4 relay inputs each: GPIO16->IN1, GPIO17->IN2, GPIO18->IN3, GPIO19->IN4, active-LOW. GND common locally with module, verified suitable power and level compatibility, unloaded contacts. Four planned slaves TOTAL: two additional slave MACs unknown, do not hardcode imaginary addresses. GPIO mappings for additional 4-relay modules can use same GPIO16/17/18/19 on each independent slave if boards are identical and tested. Only 16 output channels total with 4x4, *not enough* for all permanent equipment: eventual extra outputs require approved expanders or dedicated relay boards and wider verified protocol; reserve sensor board resources separately.

## BENCH mode stable 8-slot relay wiring (not actual plumbing wires)

Slots S1..S4: SECONDARY IN1 GPIO16, IN2 GPIO17, IN3 GPIO18, IN4 GPIO19.
Slots S5..S8: TETRIARY IN1 GPIO16, IN2 GPIO17, IN3 GPIO18, IN4 GPIO19.
Before START, software assigns slots S1..S8 to the distinct actuated components in that selected function, reports mapping, and verifies all OFF. Keep same slots through every stage of that function, avoiding alias collisions. An actuator that is only *explicitly ordered CLOSED* without previously opening can be modeled in dry-run as verified OFF; it need not consume a slot, but output this fact. Even if distinct components exceed 8, do NOT alias; report requires additional channels / split test, or explicitly test phase-by-phase with new mappings only when all relays OFF and operator acknowledges reconfiguration.

## Wine V2 functions (user supplied, keep identity)

- W_A `Wine pour initial`: MANUAL START V1 intake, pH/ethanol/ultrasonic sensor inspection pending. In bench demo V1 operates for adjustable duration; no simulated sensor approval claimed; real automation blocked without sensors.
- W_B `Wine initial -> PC1`: valves V2 + V3 OPEN then PUMP_1 ON; simulated run; pump OFF then valves OFF.
- W_C `Wine clean initial`: CV1, DV1 OPEN; HP_PUMP, PUMP_5 ON; timed; pumps OFF, valves OFF.
- W_D `Wine initial hose/backflush F1/FF1`: explicitly force CV1, DV1, V2 OFF; stage 1 open CV2,FV2 then HP_PUMP,PUMP_1,PUMP_5 ON; timed; stage 2 turn PUMP_1 OFF, open CV3,FV1 (HP_PUMP and PUMP_5 remain active per user's description, subject to confirmation of hydraulic safety); timed; end pumps OFF then ALL OFF. **Do not collapse into one flat valve+pump list.**
- W_E `Wine PC1 monitor`: pH and ethanol placeholders, no actuators until sensors connected.
- W_F `Wine PC1->PC2`: V4,V5 OPEN, PUMP_2 ON; timed; stop/off.
- W_G `Wine clean PC1`: CV4,DV2 OPEN; HP_PUMP,PUMP_5 ON; timed; stop/off.
- W_H `Wine PC1 path/backflush F2/FF2`: force CV4,DV2,V4 OFF; stage1 CV5,FV4 OPEN; HP_PUMP,PUMP_2,PUMP_5 ON; timed; stage2 PUMP_2 OFF, CV6,FV3 OPEN; timed; stop/off.
- W_I `Wine PC2 monitor`: pH and ethanol placeholders.
- W_J `Wine PC2->PC1`: V6,V7 OPEN; PUMP_3 ON; timed; off (normalize source `PUMP 3` spelling with documentation).
- W_K `Wine clean PC2`: CV7,DV3 OPEN; HP_PUMP,PUMP_5 ON; timed; off.
- W_L `Wine PC2 path/backflush F3/FF3`: force CV7,DV3,V6 OFF; stage1 CV8,FV5 OPEN; HP_PUMP,PUMP_3,PUMP_5 ON; timed; stage2 PUMP_3 OFF, CV9,FV6 OPEN; timed; off.
- W_M1 `Wine PC1->final`: V8 OPEN + PUMP_4 ON, timed, off.
- W_M2 `Wine PC2->final`: V9 OPEN + PUMP_4 ON, timed, off. Manual selection based on actual vessel, don't auto-guess.
- W_N `Wine clean final transfer hose`: CV8,DV4 OPEN; HP_PUMP,PUMP_4,PUMP_5 ON; timed; off.
- W_O `Wine post-transfer repeat J->K`: AMBIGUOUS because W_J is product transfer, not cleaning/backflush. Add clearly blocked composite placeholder until clarified; never automatically transfer wine for cleaning.
- W_P `Wine clean output`: CV9,DV5 OPEN; HP_PUMP,PUMP_5 ON; timed; off.
- `W_DAILY` future higher-level one-week recirculation/alternating container workflow; no auto operation without sensor/availability criteria and confirmation. User product dispensing button is also a reserved function needing physical route definition.

## Vinegar V2 functions (user supplied, distinct from Wine)

- V_A `Vinegar pour initial`: VV1 (user also mentions IV2 in explanatory line; **must clarify** whether IV2 is separate or alias) manual intake; pH/ethanol/ultrasound gates pending.
- V_B `Mother vinegar dosing`: DCP1 (MOTHERV_VALVE/PUMP), ultrasonic feedback and dosing threshold pending; bench relay pulse only, no automatic real dosing.
- V_C1 `Initial -> vinegar PC1`: VV2,VV3 OPEN; VPUMP_1 ON; timed; off.
- V_C2 `Initial -> vinegar PC2`: VV2,VV4 OPEN; VPUMP_1 ON; timed; off.
- V_D1 `Clean vinegar initial`: CVV1,DVV1 OPEN; HP_PUMP,VPUMP_4 ON; timed; off.
- V_D2 `Clean vinegar initial hoses`: CVV2,DVV2 OPEN; HP_PUMP ON; timed; off.
- V_E1 `Aerate vinegar PC1`: AV1 OPEN; AP ON; sensor T/pH/ethanol placeholders. 30-sec bench simulation only; real aeration control later validated.
- V_E2 `Aerate vinegar PC2`: AV2 OPEN; AP ON; sensor T/pH/ethanol placeholders.
- V_F1 `Vinegar PC1 -> output`: VV5,VV7 OPEN; VPUMP_2 ON; timed; off.
- V_F2 `Vinegar PC2 -> output`: VV6,VV8 OPEN; source says `PUMP_3` while H uses `VPUMP_3`; **clarify exact actuator naming** rather than silently conflating wine PUMP_3 with vinegar VPUMP_3. Candidate VPUMP_3 for bench marked unresolved.
- V_G1 `Clean vinegar PC1`: CVV3,DVV3 OPEN; HP_PUMP,VPUMP_4 ON; timed; off.
- V_G2 `Clean vinegar PC2`: CVV5,DVV4 OPEN; HP_PUMP,VPUMP_4 ON; timed; off.
- V_H1 `Clean vinegar PC1 hoses`: CVV4,DVV5 OPEN; HP_PUMP,VPUMP_2 ON; timed; off.
- V_H2 `Clean vinegar PC2 hoses`: CVV6,DVV5 OPEN; HP_PUMP,VPUMP_3 ON; timed; off.
- V_I `Clean vinegar output`: CVV7,DVV6 OPEN; HP_PUMP,VPUMP_4 ON; timed; off.

## Staging and timers

The current generic engine opens all valves then all pumps and has only one run duration. Extend definitions to named ordered stages with ON/OFF actions, ACK barrier per output command (or per awaited group), stage-specific bench durations (e.g. first flush 10s, second flush 10s), and shutdown. Default 30s for single-stage functions; split multi-stage demo 15s + 15s unless explicitly adjusted by user. This is a proposed demo split ONLY and is not validated CIP timing. UI displays exact stage and timer, relay states and source names. The function registry source of truth should drive Flutter button list and descriptors rather than manually duplicating names, optionally via API GET /api/functions.

## Sensor hooks and tomorrow's work

Define sensors with `sensor_id`, `vessel`, `node_id`, `pin/bus`, `raw`, `unit`, `calibrated`, `quality`, `last_updated`, calibration version/time and raw-vs-calibrated value. Reserve configurable sources: wine_initial pH/ethanol/ultrasonic, wine_PC1 pH/ethanol, wine_PC2 pH/ethanol, vinegar_initial pH/ethanol/ultrasonic, vinegar_PC1 and PC2 pH/ethanol/temperature. Actual probe count/pins and where sensors connect are NOT finalized; don't pretend physical nodes or pins exist. Provide simulated sample readings only under explicit Demo badge with no thresholds used for equipment actuation. Allow teammates to upload their calibration constants via protected interface in future; temperature sensor DS18B20, pH PH-4502C E201, MQ-3 gas ethanol prototype, ultrasonic model TBD. DCP1 dosing and automatic fermentation completion remain disabled until validated. Future dedicated sensor MCU possible, but with one Master plus 4 ESP32 slaves total, carefully reconcile slots before adding a sensor-only fifth external MCU.

## App validation

Flutter is in local sibling directory `C:/Projects/Tubatech/tubatech` per repo README, not in uploaded GitHub branch. Codex with VS Code filesystem access must open both projects explicitly. First run `flutter doctor -v`, `flutter devices`, `flutter pub get` from Flutter root; select USB Android with developer options/USB debugging enabled and run `flutter run -d <device_id>`; if no Android phone, use configured emulator. The current Flutter app might render but cannot execute relay functions until authenticated Wi-Fi Master API is actually implemented. Local phone joins device setup AP, no cloud broker for first demo. Protect esp-now channel 6 across AP/STA configuration, and if no secure pairing ready, make UI read-only and leave remote actuation disabled. Provide explicit 'DISCONNECTED' and 'UNVERIFIED' states.

## Acceptance criteria

1. Arduino IDE 3.3.12 compile both receiver boards + Master (not PlatformIO).
2. Unit/host tests preserve session guard and safety behavior, and test W_D/H/L two-stage OFF/ON transitions.
3. Eight unloaded relays demonstrate W_B, W_C, W_D, etc. via bench mapping with no fabricated equipment and explicit mapping preview. Never silently activate an unmapped actuator in real mode.
4. App on phone displays same function IDs, stages, timer, per-node/link health, named component states (not just binary 0000), all status from Master rather than fake local timers.
5. Phone STOP works and fault is shown/latches, recovery requires explicit action; loss of phone does not stop Master timers or disable local safety.
6. Sensor placeholders show not installed / uncalibrated and are ready for later real drivers with recorded calibration, no fake pass/fail thresholds.
7. Never claim physically tested or phone-ready without evidence of actual device uploads and app execution.
