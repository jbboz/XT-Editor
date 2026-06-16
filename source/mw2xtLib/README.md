# mw2xtLib — Protocol layer

Pure C++17, no JUCE dependencies. Unit-testable in isolation.

**Filled in at:** M1.2 (Protocol layer + unit tests, gate).

**Contents (planned):**
- `SoundData`, `MultiData`, `InstrumentData`, `GlobalData` structs matching Waldorf §3.1–§3.6
- SysEx encoders/decoders for every IDM (SNDP, SNDD, SNDR, MULP, MULD, MULR, GLBR/D/P, WAVR/D, WCTR/D, DISR/D, RMTP, MODR/D, Universal Device Inquiry response)
- WDATA XOR-flip nibble codec and WCTDATA 4-nibble codec (copied from gearmulator's `xtLib/xtState.cpp`)
- Parameter description loader (consumes `parameterDescriptions_xt.json`)

See [`../../docs/spec/sysex-protocol.md`](../../docs/spec/sysex-protocol.md) for the authoritative SysEx reference.
