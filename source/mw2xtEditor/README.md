# mw2xtEditor — Controller layer

Owns the in-memory patch model, the hardware MIDI device, the undo stack, and the exploration engine.

**Filled in at:** M1.3 (`HardwareMidiDevice`, gate) and M1.4 (`PatchModel` + `EditorController`).

**Contents (planned):**
- `PatchModel` — current `SoundData` / `MultiData` + `InstrumentData[8]` / `GlobalData`
- `HardwareMidiDevice` — Universal Device Inquiry autodetect, SNDP rate limiter (100 ms coalescing), bulk SNDR→SNDD state machine, DISD parser
- `EditorController` — `juce::UndoManager`, A/B compare buffers, CC routing, mode context
- `ExplorationEngine` (added in M5.1–M5.2) — Edisyn-ported mutate/merge/nudge/morph/hill-climb/constrict

Depends on `mw2xtLib` for protocol types; depends on JUCE for `MidiOutput`/`MidiInput`/`UndoManager`.
