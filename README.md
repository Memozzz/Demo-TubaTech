# Demo TubaTech

Separate Android controller for the **66 wired, active-LOW relay outputs** on one ESP32 MASTER and four ESP-NOW slaves. App label: **Demo TubaTech**. Android application ID: **`com.tubatech.demo`**, separate from the original `com.example.tubatech` app. Both can remain installed.

Normal operation: join **TubaTech_Master** Wi-Fi → choose a sequence → **START**. No operator login, arming screen, unloaded confirmation, profile approval or Serial setup is required. MASTER executes the sequence and owns all timers; phone timers never turn relays OFF. STOP stays visible. Without hardware, the app opens and allows sequence browsing, reports OFFLINE honestly and disables START.

The original [Memozzz/Tubatech](https://github.com/Memozzz/Tubatech) repository is unchanged. This repository is independent. See [provenance and attribution](NOTICE.md).

## Authoritative definitions

[`definitions/control.json`](definitions/control.json) is the source of truth for all board identities, 66 output assignments and 34 sequence entries. It preserves **30 executable relay sequences / 33 ordered phases** plus four pending entries. Generated views:

- [Wiring map](docs/WIRING.md): component → board/GPIO → module/input, with polarity.
- [Complete sequence table](docs/SEQUENCES.md): ordered ON/OFF actions and each phase duration, including shutdown behavior and source notes.

`python tools/generate_definitions.py` generates the app catalog and the three definition headers in all five standalone sketches. `--check` detects drift without writing files. Do not edit generated tables or headers independently. [Source fingerprints](reference/SOURCE_FINGERPRINTS.json) also verify that ordered sequences and mappings retain the reviewed TubaTech source semantics.

W_B drives V2/V3 on Slave 1 GPIO16/17 and PUMP_1 on Slave 2 GPIO16 for **30 seconds**, after output ACKs. W_D/W_H/W_L preserve both **15-second** phases. The other executable single-phase defaults are 30 seconds. Acknowledgements report commanded GPIO state; relay contacts and fluid behavior are unmeasured.

**Incomplete source entries:** W_E/W_I require sensor definitions and calibration; W_O has an unresolved J→K composite route; W_DAILY lacks availability/sensor/orchestration rules. These entries are visible and cannot START. The specific source notes are retained in the sequence table and reference specification.

## Boards and radio

| Board | Factory station MAC | Sketch folder under `firmware/` | Assigned outputs |
|---|---|---|---|
| MASTER | `38:18:2B:8A:9D:30` | `01_MASTER/TubaTech_Integrated_Master` | 11 |
| SLAVE 1 | `68:25:DD:F1:A3:78` | `02_SLAVE1/TubaTech_Integrated_Slave1` | 14 |
| SLAVE 2 | `58:2A:BD:D3:08:F4` | `03_SLAVE2/TubaTech_Integrated_Slave2` | 14 |
| SLAVE 3 | `78:1C:3C:A5:50:80` | `04_SLAVE3/TubaTech_Integrated_Slave3` | 14 |
| SLAVE 4 | `38:3E:51:D4:6A:30` | `05_SLAVE4/TubaTech_Integrated_Slave4` | 13 |

Fixed **Wi-Fi and ESP-NOW channel 6**. MASTER AP: `TubaTech_Master`; local API: `http://192.168.4.1`. Slave 2's complete MAC, including `:F4`, comes from the linked upstream wiring guide. Each image checks its board identity. Do not mix board images or legacy 16-byte radio packets with this 20-byte V3 protocol.

Outputs start OFF before radio initialization. Every output command needs an application ACK. MASTER command ACK timeout: **1200 ms**. Slave communication watchdog: outputs OFF after **2500 ms** without accepted MASTER traffic. STOP shuts mapped pumps OFF before ALL_OFF on required boards. A STOP epoch rejects a delayed earlier START. An unrelated offline slave does not block a sequence that does not use it. If the phone disconnects after START, MASTER retains its finite timer and shutdown; losing a required slave faults the operation. Fault reset verifies OFF and never restarts automatically.

## Required tools

Tested toolchain:

| Tool | Version / target |
|---|---|
| Flutter / Dart | **3.44.1 / 3.12.1** |
| Android SDK / build tools | **API 36 / 36.0.0** |
| Android Gradle plugin / Gradle wrapper | **9.0.1 / 9.1.0** |
| Java | **JDK 21** used for Android build |
| Android device | **API 24 (Android 7) minimum**; target API 36 |
| Arduino CLI | **1.5.1** used for compilation; Arduino IDE can also upload |
| Espressif Arduino-ESP32 | **3.3.12**, board `esp32:esp32:esp32` / ESP32 Dev Module |
| Python | **3.14.5** tested; Python 3.10+ standard library, no external packages |
| Host C++ checks | C++17; tested MSVC 2022; GCC/Clang runner provided |

## Build Android

From a fresh clone with Flutter and Android SDK installed:

The test suite also uses Python and a C++17 compiler to run the actual MASTER HTTP framing parser against real socket requests. The test builds its parser adapter automatically; on Windows use MSVC 2022 Build Tools or a C++ Native Tools prompt, and on Linux/macOS install `g++` or `clang++`.

```sh
python tools/check_sources.py
cd demo_app
flutter pub get
flutter analyze
flutter test
flutter build apk --release
```

APK output: `demo_app/build/app/outputs/flutter-apk/app-release.apk` (ARMv7, ARM64 and x86_64). Flutter generates machine-specific `android/local.properties`; it is intentionally untracked. `pubspec.lock` and Gradle wrapper files are committed. The release build uses a **local Android development signing key** for sideloading; private signing keys and key configuration are never committed. Rebuilding on another machine may use a different development key.

Install the APK from [Releases](https://github.com/Memozzz/Demo-TubaTech/releases). Permit installation from the app used to open the APK if Android requests it. The original TubaTech app stays installed. Offline sequence browsing works immediately, without boards or Wi-Fi.

## Configure and build firmware

Create the local Wi-Fi credential before compiling MASTER:

```sh
python tools/configure_wifi.py
```

This prompts privately and writes the ignored `firmware/01_MASTER/TubaTech_Integrated_Master/wifi_config.h`. Use an 8–63 character device AP password. Retain it privately for joining `TubaTech_Master`; neither the app nor repository contains the value. This is a build-time setting, not a Serial setup step during normal use. For hardware-free compilation, `python tools/configure_wifi.py --generate` creates a random ignored configuration without displaying it. Do not publish this file or firmware binaries containing its credential.

With Arduino CLI installed and on PATH:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
python tools/build_firmware.py
```

You can pass `--cli /path/to/arduino-cli`. On Windows the build script also finds the Arduino IDE bundled CLI. It compiles all five images without a connected board, writes logs to `checks/logs/`, and puts binaries under ignored `deliverables/firmware/`.

## Upload and connect later

When hardware is available, open each matching `.ino` with its sibling headers intact. Select **ESP32 Dev Module**, Espressif core **3.3.12**, default 4 MB flash and default partition layout. Upload the four slaves, then MASTER. Alternatively flash the matching `.ino.merged.bin` at **0x0** using an ESP32 flashing tool. MACs and channel are already configured; no scan or Serial pairing is required.

On Android, join **TubaTech_Master** using the privately configured device password. Keep that Wi-Fi connection if Android reports no internet. Open **Demo TubaTech**, choose a sequence, press START. The app explicitly routes local requests over this Wi-Fi even if mobile data is active. Required boards must report ready and OFF. STOP remains available; if MASTER is unreachable it displays **unconfirmed** and retries upon reconnection.

## Verification

See [the verification report](docs/VERIFICATION.md). Completed software checks cover definitions, all 66 mappings, all executable functions, timing, ACK/timeout handling, 132 STOP checkpoints, offline UI and an emulator launch. All five firmware images and the APK compile.

**Physical operation has not passed testing.** Pending work includes electrical polarity/interface confirmation, actual board communication, boot/reset/power-loss OFF behavior, actual relay timing, STOP and radio-loss shutdown. No relay motion, contacts, flow or sensor/process result is claimed.

To rerun focused checks:

```sh
python tools/check_sources.py
python tools/run_host_checks.py
```

On Windows with Visual Studio C++ tools, use `tools\run_host_checks.cmd` instead. Run Flutter checks from `demo_app/` as shown above. After staging, `python tools/audit_tracked_files.py` checks tracked source for local configuration, signing keys and common credential patterns. Build outputs, raw snapshots, local SDK paths and credentials remain outside Git.

For the focused HTTP regression check alone, run `flutter test test/http_transport_test.dart --reporter expanded` from `demo_app/`. It captures real START/STOP/RESET requests before server-side HTTP normalization and passes the raw bytes to the unchanged firmware `HttpFrame`. It also reproduces the old chunked POST rejection. No physical hardware is required.
