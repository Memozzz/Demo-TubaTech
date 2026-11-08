# Manual function ON/OFF from MASTER

No app is required. Open MASTER's Arduino Serial Monitor at **115200 baud**, select **Newline**, and send commands one at a time. All five board sketches are provided with their sibling headers. Use Espressif **esp32 3.3.12**, **ESP32 Dev Module**, default 4 MB flash/partition settings. Upload slaves first, MASTER last. Keep the existing signal wiring; check stable board power and begin with relay contacts unloaded.

| Board | Factory station MAC | Sketch |
|---|---|---|
| MASTER | 38:18:2B:8A:9D:30 | [01_MASTER](../firmware/01_MASTER/TubaTech_Integrated_Master/TubaTech_Integrated_Master.ino) |
| S1 / SECONDARY | 68:25:DD:F1:A3:78 | [02_SLAVE1](../firmware/02_SLAVE1/TubaTech_Integrated_Slave1/TubaTech_Integrated_Slave1.ino) |
| S2 / TETRIARY | 20:50:0D:8C:80:C4 | [03_SLAVE2](../firmware/03_SLAVE2/TubaTech_Integrated_Slave2/TubaTech_Integrated_Slave2.ino) |
| S3 | 78:1C:3C:A5:50:80 | [04_SLAVE3](../firmware/04_SLAVE3/TubaTech_Integrated_Slave3/TubaTech_Integrated_Slave3.ino) |
| S4 | 38:3E:51:D4:6A:30 | [05_SLAVE4](../firmware/05_SLAVE4/TubaTech_Integrated_Slave4/TubaTech_Integrated_Slave4.ino) |

MASTER requires this update. S1-S4 receiver code is unchanged from the replacement-S2 package; upload these matching receiver sketches if their code was reset or they run an older package. Each sketch checks its own station MAC. ESP-NOW uses **channel 6** and MACs; slaves need no IP setup. MASTER Wi-Fi remains `TubaTech_Master`, `192.168.4.1`, with the private credential kept local. The original TubaTech repository/app remains untouched.

## Commands

```text
NODES
START W_B ON
STATUS
START W_B OFF
```

`NODES` checks board connections without switching outputs. READY requires fresh verified application replies. `LIST` shows supported and unavailable function IDs. `STATUS` reports actual command/ACK states, skipped mappings, active function/phase, and `manual_control` / `manual_held`. A held manual phase is `MANUAL_ON`, with zero countdown because no timer is running.

`START W_B ON` sends the existing ordered actions: V2 on S1 GPIO16 ON, V3 on S1 GPIO17 ON, then PUMP_1 on S2 GPIO16 ON. Each participating action requires a matching GPIO application ACK. The outputs **stay ON until `START W_B OFF`, `STOP`, or a safety fault**. There is no 30-second automatic OFF in this explicit manual command. ACKs describe commanded GPIO state; they do not measure physical relay contacts.

`START W_B OFF` requests pump OFF, then ALL_OFF for the participating boards, with verified shutdown. It also works if no group is active. `STOP` is always available during activation, holding or shutdown. One function is active at a time: turn it OFF before selecting another. Invalid, unknown or unsupported commands do not activate anything. `RESET` is accepted after shutdown and re-verifies OFF sessions; it never restarts a function. Closing Serial Monitor does not itself send OFF; MASTER continues holding while its radio links remain healthy.

## Missing boards

Missing-at-command slave owners are skipped, with the original GPIO assignments preserved. For W_B with S2 missing, S1's V2/V3 still activate and Serial prints `SKIPPED / NOT EXECUTED: PUMP_1`. It never prints a successful PUMP_1 ON ACK. Manual holding/shutdown is marked PARTIAL when applicable. A fully skipped function can be held without fabricated output commands.

A returning board completes an OFF handshake and can show READY, but does not automatically activate. Send the same explicit ON command again to include it. Existing participants remain controlled; their verified ON state does not prevent repeating ON. Loss of an already participating board, a missing/corrupt command ACK, or unacknowledged shutdown still faults and invokes the existing OFF/watchdog response. Potentially ON/uncertain boards are not silently skipped. After restoring the failed board and finishing shutdown, use `RESET`, then a new explicit command. Outputs stay OFF at boot/reset, with no automatic restart.

## Functions with multiple phases

Keep phase actions distinct. Start phase 1, then explicitly advance to phase 2:

```text
START W_D ON 1
START W_D ON 2
START W_D OFF
```

Each phase applies its exact existing ordered ON/OFF actions and holds. State carries between phases as defined. Phase 2 requires phase 1 to have finished applying its actions; phases do not advance automatically. The same syntax applies to W_H and W_L. Bare `START W_D ON` is rejected because the phase is ambiguous. There is no command that blindly turns all phases' components ON together.

The original timed form, `START W_B` without ON/OFF, is preserved: W_B still has a 30-second phase; timed W_D/W_H/W_L still have two 15-second phases. The [authoritative sequence table](SEQUENCES.md), [66-output wiring table](WIRING.md), active-LOW polarity and original definitions are unchanged. **W_E, W_I, W_O and W_DAILY remain unavailable.** No Android rebuild is required for these Serial commands.

## Verification

Host tests feed actual newline commands through MASTER's Serial loop. They cover W_B held beyond 30 seconds, per-output GPIO ACKs, repeated ON, OFF without a prior ON, missing S2, all slaves missing, MASTER-only outputs, late S2 joining only after explicit ON, all 30 functions / 33 phases with source-derived masks, STOP during activation/holding, RESET without restart, malformed commands, unsupported functions, participating loss and missing/corrupt ON/OFF ACKs. The existing timed/full/partial regression suites and source fingerprints remain intact. All five matching sketches compile with Arduino-ESP32 3.3.12.

These are software checks with radio/GPIO stubs. Actual upload/boot, five-board replies, physical relay movement, electrical behavior and measured timing remain unverified for this package.
