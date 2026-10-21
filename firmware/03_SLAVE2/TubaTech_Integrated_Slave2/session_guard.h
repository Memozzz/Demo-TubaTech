#pragma once
#include <stdint.h>

// Pure session/replay policy, independent of GPIO and ESP-NOW callbacks.
// The identical header is copied beside both receiver sketches for Arduino IDE.
struct SessionGuard {
  enum Decision { Challenge, Confirmed, Fresh, Duplicate, Rejected };
  bool armed = false, candidate = false, forceOff = false;
  uint32_t session = 0, lastSequence = 0, candidateSession = 0;
  uint32_t candidateSequence = 0, nonce = 0, challengedAt = 0;
  uint8_t lastType = 0, lastChannel = 0, lastValue = 0;

  void disarm() { armed = false; candidate = false; }
  Decision accept(uint8_t type, uint32_t incomingSession, uint32_t sequence,
                  uint8_t channel, uint8_t value, uint16_t mask,
                  uint32_t now, uint32_t freshNonce) {
    forceOff = false;
    if (!incomingSession || !sequence || mask > 255) return reject();
    const uint32_t token = uint32_t(channel) | (uint32_t(value)<<8) | (uint32_t(mask)<<16);
    if (type == 5) { // SYNC only de-energizes; it cannot authorize ON.
      if (token) return reject();
      if (!(candidate && candidateSession == incomingSession &&
            candidateSequence == sequence && uint32_t(now-challengedAt) < 3000)) {
        candidate = true; candidateSession = incomingSession; candidateSequence = sequence;
        nonce = freshNonce & 0xFFFFFF; if (!nonce) nonce = 1;
        challengedAt = now;
      }
      armed = false; forceOff = true;
      return Challenge;
    }
    if (type == 7) { // CONFIRM must echo this node's fresh 24-bit challenge.
      if (candidate && candidateSession == incomingSession && candidateSequence == sequence &&
          token == nonce && uint32_t(now-challengedAt) < 3000) {
        session = incomingSession; lastSequence = sequence; armed = true; candidate = false;
        lastType = type; lastChannel = channel; lastValue = value;
        forceOff = true;
        return Confirmed;
      }
      if (armed && incomingSession == session && sequence == lastSequence && token == nonce && lastType == 7)
        return Duplicate;
      return reject();
    }
    if (type != 1 && type != 2 && type != 3) return reject();
    if (type == 3) forceOff = true; // Even an unarmed OFF request is safe to apply.
    if (!armed || session != incomingSession) return reject();
    if ((type == 2 && (channel >= 14 || value > 1 || mask != 0)) ||
        (type != 2 && token != 0)) return reject();
    if (sequence < lastSequence) return Duplicate; // Never reapply, never renew watchdog.
    if (sequence == lastSequence) {
      if (lastType == type && lastChannel == channel && lastValue == value) return Duplicate;
      return reject(); // Same ticket with different contents is a protocol fault.
    }
    lastSequence = sequence; lastType = type; lastChannel = channel; lastValue = value;
    return Fresh;
  }
private:
  Decision reject() { forceOff = true; disarm(); return Rejected; }
};
