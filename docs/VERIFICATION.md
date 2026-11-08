# Verification report

## Manual Serial ON/OFF - 8 November 2026

Explicit `START <function> ON` holds a source-defined phase until matching OFF/STOP or a fault. Multi-phase functions use explicit consecutive phase numbers. The existing bare START timed path and original sequence/wiring fingerprints are unchanged. Actual Serial-loop host tests cover W_B held for 60 seconds without a timer, exact GPIO ACKs/masks, all 30 functions / 33 phases, missing S2, all slaves missing, MASTER-only outputs, late S2 requiring repeated ON, OFF without prior ON, STOP/RESET, exclusive operation, invalid/unsupported commands and participating loss/missing/corrupt ACK faults. Full-mode, timed and explicit-exclusion regressions pass. All five sketches compile on Arduino-ESP32 3.3.12. Receiver behavior, fixed channel 6, MACs and relay polarity are unchanged. No Android rebuild is required.

No physical test has been marked passed. See [manual commands, per-board uploads and remaining checks](MANUAL_SERIAL.md).

## Direct MASTER and replacement S2 - 8 November 2026

S2 is enabled at user-supplied station MAC `20:50:0D:8C:80:C4`. Production exclusions are empty; safe missing-at-START skipping remains enabled. The earlier explicit-S2-absence suite now uses a host-only override. A local recoverable snapshot preserves the previous uncommitted work.

Completed software checks: current production configuration verifies five modelled board sessions; read-only NODES reports role/MAC/readiness/ACK age; Serial START runs W_B for its full 30 seconds with S1 GPIO16/17 and replacement S2 GPIO16, followed by acknowledged shutdown. All 30 executable functions pass Serial dispatch; W_D/W_H/W_L retain both full 15-second phases. Serial STOP during activation/timing, RESET without restart, missing-at-start S2, MASTER-only execution, and unexpected participating S2 loss are covered. Full-mode and explicit-exclusion regressions remain passing; generated definitions and original 66-output/sequence fingerprints pass. These tests use host radio/GPIO stubs, not physical boards.

The matching five ESP32 sketches are compiled with Arduino-ESP32 3.3.12. All 20 Flutter regression tests, including real HTTP START/STOP/RESET framing, pass after regeneration. No app code change or Android rebuild is needed for direct Serial operation. Power stability, physical uploads/boot, all four live slave replies, relay movements and measured timing remain pending. See [direct commands and board-specific sketches](MASTER_DIRECT.md).

## Partial demo update — 30 October 2026

See [the partial-demo report](PARTIAL_DEMO.md) for the focused changes, coverage and upload scope. Explicitly excluded S2 actions are skipped without remapping; following the later request, START also snapshots and skips other slaves missing at that moment. Tests cover Master-only W_B timing, late-board return only on the next START, W_B without S2, a fully skipped 30-second V_C1 phase, unaffected W_F, all 30 relay functions, STOP/RESET/recovery, unsafe-output rejection, stale replies and participating-node loss/missing or invalid ACKs. All 20 Flutter tests pass. The full-mode baseline and original sequence/wiring fingerprints are retained. Physical relay testing of this update is pending.

Software checks completed on 22 October 2026. No boards or relays were connected. The reviewed commit is the commit targeted by the release; reviewers can use the release tag or `git rev-parse HEAD` to inspect the exact source.

## Demonstration preparation — 1.0.3 (build 4)

Steps 1–5 were checked in order. The corrected real HTTP START/STOP/RESET tests pass against MASTER's unchanged parser. Authoritative definitions and actual peer configuration agree on all five MACs, channel 6, 66 GPIO/module/input assignments, active-LOW drivers and boot-OFF levels. Ordered actions and durations retain the source fingerprints. W_E, W_I, W_O and W_DAILY remain unavailable.

The eight app-flow checks and actual-sketch host checks pass: direct AP START, no login/arming/profile-confirmation UI, visible STOP, ACK gates, watchdog shutdown, 132 STOP checkpoints, 30-second W_B and the two 15-second backflush phases. These are software checks with explicit GPIO/radio stubs.

The updated APK and all five board-specific Arduino-ESP32 3.3.12 images are built from the release source. Firmware, GPIO assignments, sequence definitions and app control behavior are unchanged; changes are versioning, verification and packaging. The local bundle identifies each image by role/MAC and keeps the Wi-Fi credential private. The GitHub release has the APK, credential-free source, upload guide and exact-commit manifest. [The physical-session checklist](DEMO_UPLOAD.md) remains entirely NOT TESTED.

## HTTP correction — 1.0.2 (build 3)

Review of `70e5a701` found that the Android HTTP client streamed POST forms without Content-Length, while MASTER requires a fixed length and rejects Transfer-Encoding. The transport now encodes the form once into UTF-8 bytes, sets Content-Length to that byte count before sending, and adds exactly those bytes. Content-Type remains exactly `application/x-www-form-urlencoded`.

Six focused tests use a real HttpClient and raw loopback TCP server, plus a test-only executable compiled against MASTER's actual unchanged `api_policy.h`. START, STOP and RESET requests have the expected byte length, no Transfer-Encoding and exact encoded body (including Unicode/reserved characters), and pass the actual HttpFrame parser. Empty STOP forms send Content-Length zero. Negative controls reproduce the previous chunked serializer's rejection and reject missing-length or changed-Content-Type POSTs. These checks validate HTTP framing, not physical relay execution.

The complete Flutter suite (14 tests), analyzer and rebuilt release APK pass. All firmware files, GPIO assignments, sequence definitions and generated tables remain identical to `70e5a701`. Firmware compilation and emulator observations below are retained baseline results; hardware tests remain pending.

## Completed software checks

- All **66** owner/GPIO/module/input assignments were checked against the requested upstream wiring source. No duplicate GPIO ownership or relay input ownership. Active-LOW polarity preserved.
- Authoritative `definitions/control.json` generates the app asset and all five firmware definition headers. Generated wiring and sequence tables match. Regression fingerprints preserve all **34** sequence identities and **33** executable phases, including durations, action order and source notes.
- Actual MASTER and four receiver sketches compiled/executed in host tests with explicit GPIO/radio stubs: **30** executable functions, **132** STOP checkpoints, every owned output, application ACK matching, replay rejection, command faults and receiver watchdog OFF.
- W_B: correct Slave 1 GPIO16/17 and Slave 2 GPIO16 masks; full **30-second** MASTER timer. Missing a required owner blocks START; unrelated offline Slaves 3/4 do not.
- W_D/W_H/W_L: exact accumulated masks, both full **15-second** stages, transfer pump OFF between stages with source-specified shared pumps remaining ON, final all OFF.
- Direct AP START requires no login/arming/profile confirmation. A second START while busy is rejected. STOP interrupts operation, follows pump-before-valve shutdown and rejects an earlier delayed START through the STOP epoch.
- Flutter checks cover catalog/mapping, required-board gating, one START POST, late START response after STOP, unreachable STOP with reconnect retry, explicit fault recovery, honest offline UI, MASTER countdown with no phone relay timer, and 360×640 offline vinegar browsing with visible STOP.
- `flutter analyze`, focused Flutter tests and universal release APK build pass. Application ID is `com.tubatech.demo`, minimum Android API 24, target API 36.
- Espressif Arduino-ESP32 **3.3.12** compilation passes for MASTER and all four matching slaves. Local random Wi-Fi build configuration is ignored and unpublished.
- The baseline **1.0.1 (build 2)** APK was installed and cold-launched in a Pixel 6 Android emulator: all five boards OFFLINE, START disabled, STOP unconfirmed while unreachable, and device Wi-Fi password instructions contain no credential value. No AndroidRuntime/Flutter error was logged. APK signature verification passed. This offline observation did not exercise real POST framing; use 1.0.2 for the correction above.
- No credentials, signing keys, personal Wi-Fi settings, local SDK paths, raw snapshots or build caches are committed. Development APK signing uses the local SDK key outside the repository.

Local build/check logs are retained under ignored `checks/logs/`. Host ACKs and masks are **software stubs**, not physical measurements. No fake hardware success or animation replaces control.

## Pending physical checks

- Confirm all board identities and each actual component/GPIO/module/input destination.
- Confirm relay input electrical compatibility and active-LOW behavior; OFF through startup, reset and power loss.
- Verify all five boards communicate on fixed channel 6 and actual application ACKs reflect the commanded GPIO state.
- Measure each sequence's real relay timing and transitions, including W_D/W_H/W_L.
- Verify actual STOP and communication-loss shutdown; inspect actual relay contacts/load behavior separately.
- Sensors, dosing feedback, flow, fermentation/process quality and calibration remain unverified and are not fabricated.

W_E/W_I remain pending sensors; W_O remains an undefined composite; W_DAILY remains reserved. No physical operation has been marked passed.
