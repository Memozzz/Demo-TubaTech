# Demonstration package — 1.0.3

**Hardware has not been tested.** This package completes the software preparation only. Keep the original TubaTech app installed; Demo TubaTech uses `com.tubatech.demo`.

## Files to upload when hardware is available

For the initial five-board demonstration, upload **all five matching firmware images** below, slaves first and MASTER last. Do not upload a different role's image. Flash each `.merged.bin` at **0x0** using ESP32 Dev Module defaults, 4 MB flash and Arduino-ESP32 **3.3.12**. Arduino IDE users can instead open the linked `.ino` with all sibling headers intact.

| Role | Factory station MAC | Local image | Arduino source |
|---|---|---|---|
| MASTER | `38:18:2B:8A:9D:30` | `MASTER_38182B8A9D30.merged.bin` | [MASTER .ino](../firmware/01_MASTER/TubaTech_Integrated_Master/TubaTech_Integrated_Master.ino) |
| SLAVE 1 | `68:25:DD:F1:A3:78` | `SLAVE1_6825DDF1A378.merged.bin` | [Slave 1 .ino](../firmware/02_SLAVE1/TubaTech_Integrated_Slave1/TubaTech_Integrated_Slave1.ino) |
| SLAVE 2 | `58:2A:BD:D3:08:F4` | `SLAVE2_582ABDD308F4.merged.bin` | [Slave 2 .ino](../firmware/03_SLAVE2/TubaTech_Integrated_Slave2/TubaTech_Integrated_Slave2.ino) |
| SLAVE 3 | `78:1C:3C:A5:50:80` | `SLAVE3_781C3CA55080.merged.bin` | [Slave 3 .ino](../firmware/04_SLAVE3/TubaTech_Integrated_Slave3/TubaTech_Integrated_Slave3.ino) |
| SLAVE 4 | `38:3E:51:D4:6A:30` | `SLAVE4_383E51D46A30.merged.bin` | [Slave 4 .ino](../firmware/05_SLAVE4/TubaTech_Integrated_Slave4/TubaTech_Integrated_Slave4.ino) |

The five compiled images, their SHA-256 hashes and the privately configured Wi-Fi password are provided in the **local-only bundle**. `PRIVATE-WIFI.txt` contains the password; do not publish it, `wifi_config.h`, or the MASTER image. The MASTER image embeds the credential. GitHub release assets therefore contain the APK, source archive, this guide and software verification manifest, without credential-bearing binaries. To rebuild elsewhere, create your own ignored Wi-Fi configuration as described in [README](../README.md).

Firmware source, wiring and sequence behavior are unchanged from `70e5a701`; the HTTP POST correction is in the app. A board already running these exact matching images does not need reflashing solely for the app update. Actual installed firmware and upload completion are **not verified** while hardware is absent.

## Phone installation and normal use

1. Install the release APK **Demo TubaTech 1.0.3 (build 4)**. The existing app stays installed separately.
2. When MASTER is powered during the later hardware session, join **TubaTech_Master** with the local-only password. Keep the connection when Android reports no internet.
3. Open Demo TubaTech → select a sequence → **START**. Required boards must report ready and OFF. There is no operator login, arming screen or profile confirmation. STOP remains visible and an unreachable STOP is reported as unconfirmed.

All radio traffic uses **channel 6**. MASTER executes the ordered actions and durations; the phone displays its countdown. Receivers command OFF after 2500 ms without accepted MASTER traffic. Phone disconnection does not cancel MASTER's finite timer; a required-slave loss faults the sequence. Reconnection/reset never automatically restarts a sequence.

## Unavailable sequences

- **W_E** — Wine PC1 sensor monitoring: sensors/calibration missing.
- **W_I** — Wine PC2 sensor monitoring: sensors/calibration missing.
- **W_O** — J-to-K composite: route remains undefined.
- **W_DAILY** — daily workflow: availability, sensor and orchestration rules unspecified.

These four entries remain visible but cannot START. The 30 executable entries and 33 phases retain their original actions and durations. See [wiring](WIRING.md) and [sequence table](SEQUENCES.md).

## Later physical session — not performed yet

Follow this order only after hardware availability is announced. Stop at a failed dependent check and record the exact failure.

| Order | Check | Current result |
|---|---|---|
| 6 | Suitable power distribution; stable board 5 V under load before radio diagnosis. Keep existing signal wiring/GPIOs. | NOT TESTED |
| 7 | Read each factory MAC; upload only its matching image; confirm completed flash and successful boot. Start with relay contacts unloaded. | NOT TESTED |
| 8 | MASTER AP appears; phone stays connected without internet; actual MASTER status reaches app. | NOT TESTED |
| 9 | Actual application replies from each of four slaves on channel 6. A MAC read/flash is not a communication pass. | NOT TESTED |
| 10 | Explicitly start each existing one-second component test; correct physical relay ON then OFF, no unintended activation, never auto-run at boot. | NOT TESTED |
| 11 | W_B: Slave 1 GPIO16/17 plus Slave 2 GPIO16, observed ON for the specified 30-second phase, then OFF. | NOT TESTED |
| 12 | All remaining executable sequences; both 15-second W_D/W_H/W_L phases; compare physical actions/timing to table. | NOT TESTED |
| 13 | STOP during activation and timing; phone loss; required-slave loss; MASTER reset/reconnect; no auto-restart; unrelated offline slave independence. | NOT TESTED |
| 14 | Record per-check PASS/FAIL, remaining failures, commanded GPIO ACKs and physically observed relays in separate columns. | NOT TESTED |

The existing diagnostic channel test is an explicit MASTER service command (`TEST <component>`); it needs its existing unloaded-test readiness grant (`BENCH ARM UNLOADED`) before each test and turns OFF one second after ON ACK. These are service diagnostics, not extra screens in normal app operation. No automatic sweep or new control feature was added.
