# Wiring map — 66 assigned outputs

Generated from [`definitions/control.json`](../definitions/control.json), the authoritative definition. Do not edit this view.

All outputs are active-LOW: LOW = ON, HIGH = OFF. GPIO ACKs do not measure relay contacts.

| Module | Input | Component | Board | GPIO | Bank channel (zero-based) |
|---|---|---|---|---|---|
| R01 | IN1 | V1 | SLAVE 4 | 19 | 3 |
| R01 | IN2 | V4 | MASTER | 16 | 0 |
| R01 | IN3 | V5 | MASTER | 17 | 1 |
| R01 | IN4 | V6 | SLAVE 4 | 21 | 4 |
| R01 | IN5 | V7 | SLAVE 1 | 13 | 12 |
| R01 | IN6 | V8 | SLAVE 4 | 22 | 5 |
| R01 | IN7 | V9 | SLAVE 2 | 13 | 12 |
| R01 | IN8 | V10 | SLAVE 2 | 14 | 13 |
| R01 | IN9 | CV1 | MASTER | 18 | 2 |
| R01 | IN10 | CV2 | MASTER | 19 | 3 |
| R01 | IN11 | CV3 | MASTER | 21 | 4 |
| R01 | IN12 | CV4 | SLAVE 1 | 14 | 13 |
| R01 | IN13 | CV5 | MASTER | 13 | 12 |
| R01 | IN14 | CV6 | MASTER | 14 | 13 |
| R01 | IN15 | CV7 | SLAVE 1 | 18 | 2 |
| R01 | IN16 | CV8 | SLAVE 1 | 19 | 3 |
| R02 | IN1 | CV9 | SLAVE 1 | 21 | 4 |
| R02 | IN2 | CV10 | SLAVE 4 | 26 | 8 |
| R02 | IN3 | DV1 | SLAVE 4 | 27 | 9 |
| R02 | IN4 | DV2 | SLAVE 1 | 27 | 9 |
| R02 | IN5 | DV3 | SLAVE 1 | 32 | 10 |
| R02 | IN6 | DV4 | SLAVE 2 | 17 | 1 |
| R02 | IN7 | DV5 | SLAVE 2 | 18 | 2 |
| R02 | IN8 | FV1 | MASTER | 22 | 5 |
| R02 | IN9 | FV2 | MASTER | 23 | 6 |
| R02 | IN10 | FV3 | SLAVE 1 | 22 | 5 |
| R02 | IN11 | FV4 | SLAVE 1 | 23 | 6 |
| R02 | IN12 | FV5 | SLAVE 1 | 25 | 7 |
| R02 | IN13 | FV6 | SLAVE 1 | 26 | 8 |
| R02 | IN14 | PUMP_2 | SLAVE 4 | 32 | 10 |
| R02 | IN15 | PUMP_3 | SLAVE 1 | 33 | 11 |
| R02 | IN16 | PUMP_4 | SLAVE 2 | 19 | 3 |
| R03 | IN1 | PUMP_5 | MASTER | 33 | 11 |
| R03 | IN2 | VV1 | SLAVE 2 | 21 | 4 |
| R03 | IN3 | VV2 | SLAVE 2 | 22 | 5 |
| R03 | IN4 | VV3 | SLAVE 2 | 23 | 6 |
| R03 | IN5 | VV4 | SLAVE 2 | 25 | 7 |
| R03 | IN6 | VV5 | SLAVE 4 | 23 | 6 |
| R03 | IN7 | VV6 | SLAVE 3 | 13 | 12 |
| R03 | IN8 | VV7 | SLAVE 4 | 25 | 7 |
| R03 | IN9 | VV8 | SLAVE 4 | 13 | 12 |
| R03 | IN10 | CVV1 | SLAVE 2 | 26 | 8 |
| R03 | IN11 | CVV2 | SLAVE 3 | 17 | 1 |
| R03 | IN12 | CVV3 | SLAVE 3 | 19 | 3 |
| R03 | IN13 | CVV4 | SLAVE 3 | 22 | 5 |
| R03 | IN14 | CVV5 | SLAVE 3 | 23 | 6 |
| R03 | IN15 | CVV6 | SLAVE 4 | 14 | 13 |
| R03 | IN16 | CVV7 | SLAVE 4 | 17 | 1 |
| R04 | IN1 | V2 | SLAVE 1 | 16 | 0 |
| R04 | IN2 | V3 | SLAVE 1 | 17 | 1 |
| R04 | IN3 | DVV1 | SLAVE 2 | 27 | 9 |
| R04 | IN4 | DVV2 | SLAVE 3 | 18 | 2 |
| R05 | IN1 | PUMP_1 | SLAVE 2 | 16 | 0 |
| R05 | IN2 | DVV3 | SLAVE 3 | 21 | 4 |
| R05 | IN3 | DVV4 | SLAVE 3 | 25 | 7 |
| R05 | IN4 | DVV5 | SLAVE 4 | 16 | 0 |
| R06 | IN1 | DVV6 | SLAVE 4 | 18 | 2 |
| R06 | IN2 | VPUMP_1 | SLAVE 2 | 33 | 11 |
| R06 | IN3 | VPUMP_2 | SLAVE 3 | 32 | 10 |
| R06 | IN4 | VPUMP_3 | SLAVE 3 | 33 | 11 |
| R07 | IN1 | VPUMP_4 | SLAVE 3 | 26 | 8 |
| R07 | IN2 | AV1 | SLAVE 3 | 14 | 13 |
| R07 | IN3 | AV2 | SLAVE 3 | 16 | 0 |
| R07 | IN4 | HP_PUMP | MASTER | 32 | 10 |
| R08 | IN1 | AP | SLAVE 3 | 27 | 9 |
| R08 | IN2 | DCP1 | SLAVE 2 | 32 | 10 |

R09 IN1/IN2 and R10 IN1/IN2 are the four unassigned relay inputs. Slave 4 GPIO33 remains spare. MASTER GPIO25/26/27 remain sensor reservations. GPIO4 is excluded.
