# Edisyn (eclab)

**Source:** https://github.com/eclab/edisyn
**Pinned commit:** `49f13d5c4e546ac9ee98a9ca584dbaeece336f86` (2026-03-17 — "reformatted")
**License:** Apache-2.0

Java-based multi-synth patch editor. The author personally owns an XT, which makes this the single most reliable external reference for hardware behavior — more reliable than the spec document in edge cases. Java code cannot be linked from C++; we port logic with Apache-2.0 attribution headers in every derived file.

## What we use

| Need | Path in edisyn | Notes |
|---|---|---|
| MW II/XT patch editor (Sound) | `edisyn/synth/waldorfmicrowavext/WaldorfMicrowaveXT.java` | Hardware-verified executable copy of the sysex spec. Source of mutation weights and clamping ranges. |
| MW II/XT patch editor (Sound, recognizer) | `edisyn/synth/waldorfmicrowavext/WaldorfMicrowaveXTRec.java` | SysEx recognition for incoming dumps. |
| MW II/XT Multi editor | `edisyn/synth/waldorfmicrowavext/WaldorfMicrowaveXTMulti.java` + `WaldorfMicrowaveXTMultiRec.java` | Multi-mode (MDATA + IDATA) handling — reference for M1.2 struct shapes and M1.6 MULTI tab. |
| Exploration algorithms (base) | `edisyn/Synth.java` | Mutate, merge, nudge, morph, hill-climb, constriction. Ported wholesale into our `ExplorationEngine` in M5.2. |
| Original Microwave I editor (for cross-check) | `edisyn/synth/waldorfmicrowave/WaldorfMicrowave.java` + Multi/Rec siblings | Useful when MW1↔MWII behavioral differences matter. |

## How it's used per milestone

- **M1.2** — cross-check decoded parameter values against Edisyn output for the same SDATA bytes (≥5 patches). Use Edisyn's clamping ranges as authoritative.
- **M1.3** — verify every message frame against Edisyn's encoder/decoder before hardware testing.
- **M5.1** — port mutation weight table from `WaldorfMicrowaveXT.java`.
- **M5.2** — port six exploration algorithms from `Synth.java`.

## License obligations

Apache-2.0 is one-way compatible into (A)GPLv3. Every C++ file containing Edisyn-derived logic must carry an attribution header: original copyright, Apache-2.0 license notice, NOTICE-style mention. Track per-file in `ATTRIBUTIONS.md`.
