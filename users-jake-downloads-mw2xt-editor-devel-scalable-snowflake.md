# MW2/XT Editor — Plan Review & Next Steps

## Context

`~/Downloads/mw2xt_editor_development_plan.md` is a thorough, well-structured development plan for a Waldorf Microwave II/XT JUCE editor. The working directory (`/Users/jake/Documents/Dev/XT-Editor`) is currently **empty** — this is greenfield. The task is to review the plan for soundness and produce a concrete, sequenced set of next steps to begin execution. Test targets confirmed available: **both real XT hardware and Xenia loopback**. This document is the deliverable; scaffolding happens in a later pass.

---

## Verdict

The plan is execution-ready in its structure, phasing, and source strategy. The "fresh project, copy specific files, don't fork" decision is correct and the phase ordering is sound. The risks below are not reasons to change the plan — they are **assumptions to validate in the first work session** before they cost rework. The single highest-leverage change is to insert a **Phase 0 spike** that proves the borrowed components actually detach cleanly, before committing to them.

---

## Review — gaps and risks to resolve early

These are ordered by how much downstream rework they cause if wrong.

1. **"Cleanly separable" is an unproven assumption (highest risk).** The plan treats gearmulator's PatchDB, patch-manager UI, skin system, and MIDI Learn overlay as drop-in copyable units. In practice these typically depend on gearmulator's `baseLib` / `juceUiLib` / `synthLib` substrate. If coupling is deeper than the plan assumes, the "copy don't fork" strategy partially collapses. **Resolve by spiking each borrowed component's real dependency closure before Phase 1 (see Next Steps Step 1).**

2. **Confirm gearmulator actually contains MW2/XT assets at the referenced paths.** The plan cites `source/mw2xt/`, `source/xeniaJucePlugin/`, and `parameterDescriptions_mw2xt.json`. Xenia is the Microwave-family emulation in gearmulator, so this is plausible — but the exact file existence, name, and completeness of the parameter JSON must be verified, since it is called "the most important single file in the project."

3. **MidiKraft-librarian's dependency graph.** It is C++/JUCE but is part of the larger MidiKraft/KnobKraft tree and likely pulls `MidiKraft-base` and siblings. Verify it builds standalone as a submodule, or scope exactly which translation units are needed, before relying on it for the SNDR→SNDD handshake.

4. **CLAP format dependency is unstated.** JUCE has no native CLAP target; CLAP requires `clap-juce-extensions` (free/u-he). The plan lists CLAP as a shipping format throughout but never adds this submodule. Add it to the Phase 1 submodule list or drop CLAP from the MVP.

5. **License combination is fine but state it precisely.** AGPL-3.0 final work incorporating GPL-3.0 (gearmulator/Xenia skin assets) is permitted — GPLv3 §13 explicitly allows linking with AGPLv3, and Apache-2.0 is one-way compatible into (A)GPLv3. The plan's conclusion is correct; just ensure `ATTRIBUTIONS.md` records per-file origin and that Edisyn-derived files carry the Apache-2.0 attribution header as the plan already notes.

6. **Elevate Xenia loopback to a first-class early test harness.** Since both targets are available, use **Xenia-over-IAC for fast iteration** (no physical setup, repeatable, scriptable) and **real hardware for confirmation** at each milestone. The plan mentions Xenia only as an afterthought in Phase 6; in practice it is the cheapest way to de-risk the protocol layer (Step 4 below). Note one caveat to verify: confirm Xenia mirrors real-hardware SysEx timing/coalescing behavior — if it is more permissive than firmware, the 100ms SNDP rate limiter still must be validated on metal.

7. **Minor consistency item.** IDATA size appears as both `28` (architecture) and is referenced against §3.3 — confirm the byte count against the spec and Edisyn before freezing the `InstrumentData` struct.

---

## Next Steps (near-term, actionable)

> **Superseded by `ROADMAP.md` (2026-06-15).** Steps 0–5 below have been absorbed into milestones M0.1 → M1.5 with explicit dependencies, exit criteria, and gates. Use the roadmap as the working document; this section is retained for the original review reasoning.

**Step 0 — Acquire reference material into `references/` (no build).**
Clone read-only: gearmulator (for the Xenia/Microwave sources, parameter JSON, codec, PatchDB, skin, MIDI Learn), Edisyn (`WaldorfMicrowaveXT.java`, `Synth.java`), mwsd (autodetect + DISD parser). Locate and confirm the exact path of `parameterDescriptions_*.json` and the wave/wavetable nibble codec. This is read/verify only — nothing compiled yet.

**Step 1 — Decoupling spike (de-risks the whole source strategy).**
For each borrowed gearmulator component (PatchDB, patch-manager UI, skin system, MIDI Learn overlay), trace its real include/dependency closure. Decide per component: *copy clean*, *copy + small shim*, or *reimplement*. Verify MidiKraft-librarian builds as a standalone submodule. Write the findings into `ATTRIBUTIONS.md` provenance notes. **Do not start Phase 1 scaffolding until this is known** — it determines what "copy specific files" actually means.

**Step 2 — Project scaffold (plan §9.1).**
Fresh JUCE CMake project; submodules: JUCE, sqlite_orm, MidiKraft-librarian, juce-widgets, **and clap-juce-extensions** (per gap #4). Create `references/`, `source/{mw2xtLib,mw2xtEditor,mw2xtUI,mw2xtPlugin,patchManager}/`, default-skin directory. Add AGPL-3.0 `LICENSE` and `ATTRIBUTIONS.md`. Confirm it builds an empty Standalone + VST3 + AU (+ CLAP) on macOS first.

**Step 3 — Protocol layer + unit tests (plan §9.2). Gate.**
`mw2xtLib`: copy wave/wavetable codec **verbatim**, copy parameter JSON, write `SoundData`/`MultiData`/`GlobalData` structs with Edisyn-sourced clamping. Unit tests for: SNDP encode/decode (including the **HH=00h/01h split at index 128**), SNDD checksum, wave XOR-flip nibble codec, **MULP IDM = 21h**. **Do not proceed until tests pass.**

**Step 4 — Hardware device + dual test harness (plan §9.3). Gate.**
`HardwareMidiDevice`: Universal Device Inquiry autodetect, SNDP send with rate limiter, SNDR/SNDD via MidiKraft-librarian, DISD parser. Stand up the **Xenia-over-IAC loopback** for fast iteration; confirm every message type against it with a MIDI monitor, then **confirm on real hardware** before any UI work. Specifically validate the 100ms coalescing on physical firmware (per gap #6).

**Step 5 — Proof-of-concept slice (plan §9.4–9.5).**
PatchModel + EditorController (undo, A/B compare, CC routing), then OSC + FILTER tabs with the skin system wired in. End-to-end: edit a knob in the UI → SNDP → audible change on hardware; turn a hardware knob → CC → UI updates. This validates the full vertical stack before fanning out to the remaining tabs.

After Step 5 the plan's Phase/Sequence ordering (remaining tabs → library → wavetable → mod matrix → exploration → polish) can proceed as written.

---

## Verification (per gate)

- **Step 3:** `ctest` green for codec/checksum/SNDP/MULP round-trips; spot-check a few decoded parameter values against Edisyn output for the same SDATA bytes.
- **Step 4:** With a MIDI monitor on the IAC bus, every message type (SNDP/SNDR/SNDD/MULx/GLBx/WAVx/WCTx/DISx/RMTP/MODx) frames byte-identical to Edisyn; autodetect returns the correct device family/ID; then repeat the critical subset on the physical XT.
- **Step 5:** Manual round-trip on both targets — UI→hardware and hardware→UI — with undo/redo and A/B compare behaving correctly. Use the real XT for final sign-off on each gate.

---

## Open items to confirm before/during Step 0–1

- Exact filename + completeness of the gearmulator parameter-description JSON for MW2/XT.
- Real dependency closure of PatchDB / patch-manager UI / skin / MIDI Learn (Step 1 output).
- MidiKraft-librarian standalone buildability.
- IDATA byte count vs spec §3.3 (gap #7).
- Whether Xenia's SysEx timing is faithful enough to trust for rate-limiter testing, or whether that must be hardware-only.
