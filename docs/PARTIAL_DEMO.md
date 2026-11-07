# Explicit partial relay demo

Slave 2 / TETRIARY is now enabled with replacement station MAC `20:50:0D:8C:80:C4`, supplied on 8 November 2026. No board is explicitly excluded by default. See [direct MASTER control](MASTER_DIRECT.md) for operation without the app. Following the requested simpler workflow, **START also skips any other slave that is not ready at that moment**, so the demo can run with one, two, three or all four slaves absent. This choice is frozen for that run: a late board joins on the next START. Unexpected loss of a board already participating still faults and shuts down through the existing ACK/watchdog path. A board with uncertain/potentially ON outputs is never silently skipped.

## One configuration

Edit only `partial_demo` in [`definitions/control.json`](../definitions/control.json):

```json
"partial_demo": {
  "enabled": true,
  "skip_unready_at_start": true,
  "excluded_board_ids": []
}
```

Board IDs remain MASTER=0, S1=1, S2=2, S3=3 and S4=4. Only slave IDs 1–4 may be explicitly excluded. Run `python tools/generate_definitions.py` after editing. It generates MASTER's `demo_config.h` and the app's catalog metadata; do not edit the generated header independently. GPIOs, relay/module inputs, function actions, ordering and phase durations are unchanged. Only the replacement S2 MAC changes.

S2 has already been restored in this configuration. To explicitly omit it again, put `2` in the list, regenerate, rebuild and upload MASTER. Other restored slaves participate automatically on the next START after they report ready. Set `skip_unready_at_start` to `false` for explicit-exclusions-only behavior, or `enabled` to `false` for the original full mode. No component is moved to another board. The updated app reads MASTER's live configuration; rebuilding it again is optional if only the offline preview configuration changes.

## Execution and status

- Normal flow stays **join TubaTech_Master → select sequence → START**. No new arming, approval or confirmation step.
- Profiles retain every original component, including skipped ones. START snapshots ready slave owners and freezes its skip mask. Only required participating owners enter ACK, STOP and recovery/shutdown barriers. MASTER's local GPIO readiness is never auto-skipped.
- Each action on an explicitly excluded board is labelled **SKIPPED / NOT EXECUTED**, never sent and never assigned a successful ACK. ON and OFF actions are both skipped; their original definitions remain intact.
- A phase with only skipped actions still runs its full specified duration. It does not issue artificial commands or wait for an absent owner. Timers remain on MASTER.
- Explicitly excluded boards are **NOT INSTALLED**, keep their original MAC/ID and have null commanded/acknowledged output states. No peer, SYNC, heartbeat, relay, ALL_OFF or recovery traffic is sent to them. Their stray packets are ignored. Other slaves missing at START are marked skipped for that run; background discovery resumes between runs so repaired/reset boards can participate later.
- Participating outputs retain matching application ACKs, timeouts, fault latching, pump-before-valve shutdown, watchdog backup and explicit RESET with no automatic restart. Unrelated unused offline nodes retain their prior behavior.
- Completion after any skipped actions is **COMPLETE_PARTIAL**, with `partial_completion=true` and a skipped-action count. An unaffected function retains ordinary COMPLETE. STOP/OFF acknowledgement scope excludes uninstalled boards.
- The app shows the configured exclusion, each skipped component and partial completion. Offline browsing is labelled a configured preview; connected behavior follows MASTER, including disabled/full mode. Demo TubaTech 1.0.4 follows the live MASTER configuration. Its earlier offline preview can still show the previous explicit S2 exclusion; no app is needed for Serial commands.

**W_E, W_I, W_O and W_DAILY remain unavailable.** No sensor-pending or unsupported function is enabled.

## Verified in software

The explicit-S2-absence tests below use a host-test-only exclusion override. The production configuration enables the replacement S2; additional direct-command tests cover that configuration with all five boards modelled ready.

- W_B without S2: S1 GPIO16/V2 and GPIO17/V3 execute with real application-ACK checks in the host radio/GPIO model. PUMP_1 stays mapped to S2 GPIO16 and is skipped, with null ACK/commanded state. The phase lasts 30 seconds; shutdown finishes without S2 traffic and reports partial completion.
- V_C1 consists only of S2 outputs: all three actions are skipped, the full 30 seconds elapse, and no output/shutdown commands are fabricated, even with an unrelated old profile owner offline.
- With S1/S3/S4 also missing, W_B still starts and keeps its 30-second phase with all three actions skipped. A returning S1 remains unused during that run and executes V2/V3 on the next START. An unsafe output state blocks START instead of being skipped.
- W_F is unaffected: normal outputs/ACKs, full timing and ordinary COMPLETE. All 30 relay functions complete in partial-mode host checks; four unavailable entries remain blocked.
- STOP before/during activation and during timing, RESET/recovery, excluded-node stray replies, missing participating owner, missing/corrupt ACK and unexpected participating-node loss are covered. Missing/corrupt participating ACKs and link loss still produce FAULT, not partial success.
- The disabled-mode baseline covers all 30 functions, 132 STOP checkpoints, every output and four receiver drivers. JSON contracts verify NOT INSTALLED, null skipped ACKs, original assignments, function availability and partial completion. Flutter checks cover gating, skipped labels, completion, STOP and live disabled-mode compatibility; real HTTP framing checks remain in the suite.

These are software checks. No physical relay operation with this new configuration is claimed.

## Upload exactly these changes

For this replacement package, upload the new MASTER sketch and S2 sketch with all their sibling headers. All five generated board maps now contain S2's replacement identity. S1/S3/S4 receiver behavior is unchanged, but upload the supplied matching sketches if those boards were reset. See [the exact role/MAC upload table and Serial commands](MASTER_DIRECT.md).

No Android rebuild is needed for direct MASTER control. The installed 1.0.4 app also reads live MASTER exclusions when connected; its offline catalog is an earlier preview.

Keep any compiled MASTER image and local Wi-Fi configuration private: the binary embeds the configured credential.
