# Attribution and source provenance

This independent demo reuses the TubaTech five-board integration, component/sequence definitions, ACK/session/watchdog code, relay drivers and Android project scaffolding supplied by **Memozzz / the TubaTech authors**.

- Upstream: https://github.com/Memozzz/Tubatech
- Reviewed branch: `review/tubatech-five-board-integration`
- Reviewed upstream commit: `160e5099bff3408bf41a0bdff83635b9d752f2cb`
- Wiring source: `TubaTech_Arduino/07_FIVE_BOARD_INTEGRATION/WIRING_BY_RELAY.md` at that commit.
- Executable source: the integrated MASTER/slave sketches and `function_library.h` / `component_registry.h` in that package.
- Additional source: the existing local TubaTech Flutter Android scaffolding and authoritative final sequence catalog supplied in this workspace.
- Earlier original standalone firmware provenance is recorded by upstream at commit `fceba1eebc63b8c65cda856d6af1d2cb2bc172a4`; no standalone blocking delay sequencers were substituted for the MASTER engine.

The current demo adds the separate Android application ID and direct local control UI, offline browsing, matching fixed-wiring definitions, STOP request fencing, private local build configuration and review/build tooling. Original function identities, component labels, GPIOs, active-LOW polarity, ordered actions and specified default durations are preserved.

No repository-level LICENSE, LICENCE, COPYING, NOTICE or AUTHORS file was present in the reviewed upstream tree, and GitHub reported no detected license. No new license grant or upstream relicensing is implied by this copy. Existing code comments and authorship/provenance are retained. Reference specifications are included for traceability; one historical example credential was replaced with an omission marker.

Flutter, Dart, Android, Gradle and other dependencies retain their own licenses and notices. Flutter's BSD license is retained in `LICENSES/Flutter-BSD.txt`; Dart's SDK license is retained in `LICENSES/Dart.txt`. Flutter includes dependency license notices in the built APK. Gradle wrapper files and their existing comments are preserved. Upstream private repository visibility is retained for this independent repository.
