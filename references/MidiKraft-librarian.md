# MidiKraft-librarian (christofmuc)

**Source:** https://github.com/christofmuc/MidiKraft-librarian
**Pinned commit:** `ccd50a1af96d9e0ec8739b2f9072b74326af8b04` (2025-11-03 — "Adding a note before archiving")
**License:** AGPL-3.0 (with MIT available for purchase)
**Status:** **ARCHIVED.** The repository's README states it has been merged into the consolidated MidiKraft repo at https://github.com/christofmuc/MidiKraft.

## What the dev plan wanted from it

> The correct solution for the SNDR → SNDD handshake problem. Provides: request/response state machine with configurable timeouts, retry logic, sequential bank-by-bank bulk download, interleaved message handling, and progress callbacks for JUCE progress bars. Do not write a custom handshake state machine.

— `mw2xt_editor_development_plan.md` §2.3

## M0.2 spike finding: standalone build is not feasible

`CMakeLists.txt` declares:

```cmake
target_link_libraries(midikraft-librarian
    juce-utils
    midikraft-base
    nlohmann_json::nlohmann_json
    fmt::fmt)
```

Public interface headers (`Librarian.h`, `PatchHolder.h`, `SynthBank.h`, …) depend on abstract interfaces from `midikraft-base`:

- `Synth` (with sub-capabilities: `BankDumpCapability`, `EditBufferCapability`, `HandshakeLoadingCapability`, `HasBanksCapability`, `LegacyLoaderCapability`, `ProgramDumpCapability`, `SendsProgramChangeCapability`, `StreamLoadCapability`, `DataFileLoadCapability`)
- `MidiController`, `SafeMidiOutput`, `MidiBankNumber`, `MidiHelpers`
- `ProgressHandler`, `Sysex`, `Settings`, `RunWithRetry`
- `StepSequencer`

Using `Librarian` means implementing the full `Synth` capability surface for the Microwave II/XT, plus bringing in `juce-utils` (utility code), `midikraft-base` (the abstract interface library), `nlohmann_json` and `fmt` (third-party). The minimum effort to make this work is roughly:

- Vendor 4+ submodules (juce-utils, midikraft-base, nlohmann_json, fmt) or use the consolidated MidiKraft repo and inherit its full dependency closure
- Implement ~9 capability interfaces against our `HardwareMidiDevice`
- Configure RapidJSON via `MANUALLY_RAPID_JSON` and the bin-resource bootstrap (`createResources.cmake`)
- Maintain compatibility with the archived API surface or migrate to the consolidated MidiKraft repo if/when that API changes

## What we actually need (the underlying capability)

Stripped of the framework, the librarian's job is:

1. **Sequential bank dump** — send SNDR for each of the 256 patches, wait for SNDD, collect into a buffer, time out individual requests with retry, post progress to a JUCE-thread-safe callback
2. **Edit-buffer fetch** — single SNDR(BB=20 NN=00) → SNDD round-trip with timeout
3. **Bulk send** — sequential SNDD writes with optional spacing
4. **Interleave handling** — incoming DISD/MODD/RMTP arriving during a bulk operation must not corrupt the state machine

This is ~200–400 lines of focused JUCE-aware C++ given our `HardwareMidiDevice` already exists. The dev plan's recommendation ("do not write a custom handshake state machine") was based on the assumption that MidiKraft-librarian was a turn-key drop-in. The spike shows otherwise.

## Recommendation

**Write our own focused state machine in M2.3.** Lower total effort than vendoring 4+ submodules and implementing ~9 capability interfaces just to use the librarian's request/response loop. We can still read `Librarian.cpp` for reference patterns (timeout/retry logic, handler stack, partial-message handling) — but won't link the library.

This is reflected in [ROADMAP.md D-05](../ROADMAP.md) and the updated M2.3 scope.

## License obligations

If we end up porting any source from MidiKraft-librarian (rather than just reading it for reference), AGPL-3.0 attribution headers required per file ported. Track in `ATTRIBUTIONS.md`.
