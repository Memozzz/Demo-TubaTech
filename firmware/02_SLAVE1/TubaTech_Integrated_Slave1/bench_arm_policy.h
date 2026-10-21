#pragma once
// MASTER-controlled unloaded bench policy approved by the user on 2026-10-21.
// Set to 0 to restore separate per-receiver physical Serial arming.
// It never starts outputs: the verified OFF handshake only makes the receiver
// available. MASTER still requires physical bench arm + profile confirmation.
#ifndef TUBATECH_MASTER_ONLY_UNLOADED_ARM
#define TUBATECH_MASTER_ONLY_UNLOADED_ARM 1
#endif
