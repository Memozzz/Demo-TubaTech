# Sequence table

Generated from [`definitions/control.json`](../definitions/control.json), the authoritative definition. Do not edit this view.

Actions below execute left-to-right, waiting for matching application ACKs. Durations start after all phase actions are acknowledged. State carries forward between phases.

Finish / STOP: issue OFF to each mapped pump in first-occurrence order, then ALL_OFF to each required board. No phone timer switches relays OFF.

| ID | Title | Phase | Seconds | Ordered actions |
|---|---|---|---|---|
| W_A | Wine pour initial | 1. Wine pour initial | 30 | V1 ON |
| W_B | Wine initial -> PC1 | 1. Wine initial -> PC1 | 30 | V2 ON → V3 ON → PUMP_1 ON |
| W_C | Wine clean initial | 1. Wine clean initial | 30 | CV1 ON → DV1 ON → HP_PUMP ON → PUMP_5 ON |
| W_D | Wine F1/FF1 backflush | 1. First flush | 15 | CV1 OFF → DV1 OFF → V2 OFF → CV2 ON → FV2 ON → HP_PUMP ON → PUMP_1 ON → PUMP_5 ON |
| W_D | Wine F1/FF1 backflush | 2. Reverse backflush | 15 | PUMP_1 OFF → CV3 ON → FV1 ON |
| W_E | Wine PC1 monitor | **SensorsPending** | — | pH and ethanol sensors not installed/calibrated. |
| W_F | Wine PC1 -> PC2 | 1. Wine PC1 -> PC2 | 30 | V4 ON → V5 ON → PUMP_2 ON |
| W_G | Wine clean PC1 | 1. Wine clean PC1 | 30 | CV4 ON → DV2 ON → HP_PUMP ON → PUMP_5 ON |
| W_H | Wine F2/FF2 backflush | 1. First flush | 15 | CV4 OFF → DV2 OFF → V4 OFF → CV5 ON → FV4 ON → HP_PUMP ON → PUMP_2 ON → PUMP_5 ON |
| W_H | Wine F2/FF2 backflush | 2. Reverse backflush | 15 | PUMP_2 OFF → CV6 ON → FV3 ON |
| W_I | Wine PC2 monitor | **SensorsPending** | — | pH and ethanol sensors not installed/calibrated. |
| W_J | Wine PC2 -> PC1 | 1. Wine PC2 -> PC1 | 30 | V6 ON → V7 ON → PUMP_3 ON |
| W_K | Wine clean PC2 | 1. Wine clean PC2 | 30 | CV7 ON → DV3 ON → HP_PUMP ON → PUMP_5 ON |
| W_L | Wine F3/FF3 backflush | 1. First flush | 15 | CV7 OFF → DV3 OFF → V6 OFF → CV8 ON → FV5 ON → HP_PUMP ON → PUMP_3 ON → PUMP_5 ON |
| W_L | Wine F3/FF3 backflush | 2. Reverse backflush | 15 | PUMP_3 OFF → CV9 ON → FV6 ON |
| W_M1 | Wine PC1 -> final | 1. Wine PC1 -> final | 30 | V8 ON → PUMP_4 ON |
| W_M2 | Wine PC2 -> final | 1. Wine PC2 -> final | 30 | V9 ON → PUMP_4 ON |
| W_N | Wine clean final transfer hose | 1. Wine clean final transfer hose | 30 | CV8 ON → DV4 ON → HP_PUMP ON → PUMP_4 ON → PUMP_5 ON |
| W_O | Wine post-transfer J -> K | **Ambiguous** | — | J is product transfer, not cleaning; clarify composite route. |
| W_P | Wine clean output | 1. Wine clean output | 30 | CV10 ON → DV5 ON → HP_PUMP ON → PUMP_5 ON |
| W_DAILY | Wine daily alternation | **Reserved** | — | Availability, sensors and one-week orchestration are not validated. |
| W_DISPENSE | Wine product dispensing | 1. Wine product dispensing | 30 | V10 ON |
| V_A | Vinegar pour initial | 1. Vinegar pour initial | 30 | VV1 ON |
| V_B | Mother vinegar dosing | 1. Mother vinegar dosing | 30 | DCP1 ON |
| V_C1 | Vinegar initial -> PC1 | 1. Vinegar initial -> PC1 | 30 | VV2 ON → VV3 ON → VPUMP_1 ON |
| V_C2 | Vinegar initial -> PC2 | 1. Vinegar initial -> PC2 | 30 | VV2 ON → VV4 ON → VPUMP_1 ON |
| V_D1 | Vinegar clean initial | 1. Vinegar clean initial | 30 | CVV1 ON → DVV1 ON → HP_PUMP ON → VPUMP_4 ON |
| V_D2 | Vinegar clean initial hoses | 1. Vinegar clean initial hoses | 30 | CVV2 ON → DVV2 ON → HP_PUMP ON |
| V_E1 | Vinegar aerate PC1 | 1. Vinegar aerate PC1 | 30 | AV1 ON → AP ON |
| V_E2 | Vinegar aerate PC2 | 1. Vinegar aerate PC2 | 30 | AV2 ON → AP ON |
| V_F1 | Vinegar PC1 -> output | 1. Vinegar PC1 -> output | 30 | VV5 ON → VV7 ON → VPUMP_2 ON |
| V_F2 | Vinegar PC2 -> output | 1. Vinegar PC2 -> output | 30 | VV6 ON → VV8 ON → VPUMP_3 ON |
| V_G1 | Vinegar clean PC1 | 1. Vinegar clean PC1 | 30 | CVV3 ON → DVV3 ON → HP_PUMP ON → VPUMP_4 ON |
| V_G2 | Vinegar clean PC2 | 1. Vinegar clean PC2 | 30 | CVV5 ON → DVV4 ON → HP_PUMP ON → VPUMP_4 ON |
| V_H1 | Vinegar clean PC1 hoses | 1. Vinegar clean PC1 hoses | 30 | CVV4 ON → DVV5 ON → HP_PUMP ON → VPUMP_2 ON |
| V_H2 | Vinegar clean PC2 hoses | 1. Vinegar clean PC2 hoses | 30 | CVV6 ON → DVV5 ON → HP_PUMP ON → VPUMP_3 ON |
| V_I | Vinegar clean output | 1. Vinegar clean output | 30 | CVV7 ON → DVV6 ON → HP_PUMP ON → VPUMP_4 ON |

## Source notes

- **W_A**: Manual bench intake only; pH/ethanol/ultrasonic inspection not implemented.
- **W_D**: Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation.
- **W_E**: pH and ethanol sensors not installed/calibrated.
- **W_H**: Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation.
- **W_I**: pH and ethanol sensors not installed/calibrated.
- **W_J**: V2 explicitly normalizes old PUMP 3 to PUMP_3.
- **W_L**: Unloaded demonstration only; continued HP_PUMP/PUMP_5 is not validated hydraulic operation.
- **W_O**: J is product transfer, not cleaning; clarify composite route.
- **W_DAILY**: Availability, sensors and one-week orchestration are not validated.
- **W_DISPENSE**: Final source specifies manual V10. Timed unloaded demonstration; no dispensing volume claimed.
- **V_A**: User clarification 2026-10-18: IV2 was a typo for VV1. Sensor gates pending.
- **V_B**: Bench pulse only; no ultrasonic dosing decision or validated dose.
- **V_E1**: Bench only; temperature/pH/ethanol control unimplemented.
- **V_E2**: Bench only; temperature/pH/ethanol control unimplemented.
- **V_F2**: User clarification 2026-10-18: vinegar VPUMP_3 is distinct from wine PUMP_3.
