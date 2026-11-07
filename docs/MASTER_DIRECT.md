# Direct MASTER control - no Android app needed

Slave 2 replacement: **20:50:0D:8C:80:C4**, reported by the user on 8 November 2026. S2 is enabled again. All board IDs, 66 GPIO/relay assignments, active-LOW polarity, function actions and phase durations stay unchanged. Every radio uses **channel 6**.

## Upload

Use Arduino IDE, **ESP32 Dev Module**, Espressif **esp32 3.3.12**, default 4 MB flash/partition settings. Open the matching `.ino` and keep all its sibling `.h` files. Upload the slaves first, then MASTER. The current matching package includes all five sketches; use this package instead of an older download. MASTER and replacement S2 must be reflashed. Reflash S1/S3/S4 too if their code was reset. Their generated peer maps now also record the replacement S2.

| Board | Factory station MAC | Sketch |
|---|---|---|
| MASTER | 38:18:2B:8A:9D:30 | [01_MASTER](../firmware/01_MASTER/TubaTech_Integrated_Master/TubaTech_Integrated_Master.ino) |
| S1 / SECONDARY | 68:25:DD:F1:A3:78 | [02_SLAVE1](../firmware/02_SLAVE1/TubaTech_Integrated_Slave1/TubaTech_Integrated_Slave1.ino) |
| S2 / TETRIARY | 20:50:0D:8C:80:C4 | [03_SLAVE2](../firmware/03_SLAVE2/TubaTech_Integrated_Slave2/TubaTech_Integrated_Slave2.ino) |
| S3 | 78:1C:3C:A5:50:80 | [04_SLAVE3](../firmware/04_SLAVE3/TubaTech_Integrated_Slave3/TubaTech_Integrated_Slave3.ino) |
| S4 | 38:3E:51:D4:6A:30 | [05_SLAVE4](../firmware/05_SLAVE4/TubaTech_Integrated_Slave4/TubaTech_Integrated_Slave4.ino) |

Use the station MAC, not the AP MAC. Each sketch checks its own factory identity and locks outputs OFF on a mismatch. Slaves communicate directly over ESP-NOW: they have no IP address to configure and do not join MASTER's Wi-Fi. MASTER still offers `TubaTech_Master` at `192.168.4.1` for later HTTP/app use. Its private build-time Wi-Fi configuration remains local only.

## First check connections without activating relays

Start with relay contacts unloaded and confirm stable 5 V at the boards under load before diagnosing radio failures. Keep the existing signal wiring. Power the boards, then open **MASTER's Serial Monitor at 115200 baud, Newline**. Allow a few seconds for the OFF handshake, then send:

```text
NODES
```

Expect SECONDARY, TETRIARY, SLAVE 3, SLAVE 4 and MASTER LOCAL each to show **READY** with recent `last_ACK_ms`. READY requires verified application replies and an output-enabled session. It does not prove a physical relay moved. Nothing is activated by NODES. Use `RADIO` for radio details and `STATUS` for full JSON. Check `NODES` again after several seconds: ACK ages should refresh. A flash success or radio-delivery counter alone is not a link pass.

## Call a function on MASTER

Send each line separately:

```text
LIST
CHECK W_B
START W_B
STATUS
STOP
```

`START W_B` alone is sufficient: no app, login, arming or profile confirmation. MASTER sends V2 and V3 to **S1 GPIO16/17**, then PUMP_1 to **S2 GPIO16**, verifying each application ACK. It holds the phase for **30 seconds after the phase actions are acknowledged**, then performs pump OFF and participating-board ALL_OFF with ACK verification. Let a separate run finish without STOP to check the full duration. `STATUS` reports the active phase, remaining milliseconds, participating mappings and skipped actions. Relays follow active-LOW polarity.

Use `START <ID>` for any supported ID from LIST. W_D/W_H/W_L retain two 15-second phases. **W_E, W_I, W_O and W_DAILY remain unavailable.** See [the complete sequence table](SEQUENCES.md) and [66-output wiring table](WIRING.md).

`STOP` interrupts activation or timing and requests acknowledged shutdown. After a fault, wait for shutdown to finish, restore the failed participating board, then send `RESET`. RESET re-verifies OFF sessions and never restarts a function. A new explicit START is required. Closing Serial Monitor or disconnecting the app does not cancel a finite MASTER timer; use STOP to request early shutdown.

## Missing boards

The current `partial_demo` configuration enables all four slaves (`excluded_board_ids: []`) and skips slaves that are safely unavailable at START. That decision is frozen for the run; a board returning late joins the next run. Skipped actions retain their original pinning and phase durations, are never acknowledged as executed, and completion is marked **PARTIAL DEMO COMPLETE**. If a board that is participating disappears or stops acknowledging, the existing fault/shutdown response remains in force. Uncertain or potentially ON outputs are not silently skipped. MASTER GPIO readiness remains required for functions using MASTER.

For an all-board communication demonstration, require all five READY rows before START; otherwise a partial run can intentionally succeed with outputs skipped. No physical link or relay test has yet been marked passed for this replacement package.
