# Waldorf Microwave II/XT Editor — Roadmap

**Repository:** https://github.com/jbboz/XT-Editor

Living execution document. The architectural reference is `mw2xt_editor_development_plan.md`; this doc is the ordered list of milestones I actually work from, plus the open decisions that gate them.

---

## 1. How to read this doc

- **Decision log** (§3) sits at the top because open decisions block real work. Check it at the start of each session.
- **Milestones** (§4) are grouped by phase and sequenced within each phase. Earlier phases (0–1) are detailed; later phases (4–6) are coarser and will be refined when their predecessors land.
- **Gates** are milestones that must pass before downstream work begins. They are explicitly marked.
- **Effort tags** are rough sizing, not commitments. Re-estimate as you go.
- **Currently in flight** (§5) is a one-line pointer to where work is happening right now.
- **Changelog** (§6) records material changes to scope, ordering, or decisions.

## 2. Effort scale

| Tag | Size |
|---|---|
| S  | ≤ ½ day |
| M  | ~1–3 days |
| L  | ~3–7 days |
| XL | > 1 week — split before starting |

## 3. Open decisions & de-risking log

Status values: `Open` · `Spiking` · `Resolved (YYYY-MM-DD)` · `Deferred`. Resolved entries stay in the table as an audit trail.

Convention: **Blocks** = milestones that cannot start until this decision is resolved. **Notes** indicate which milestone investigates and resolves the decision.

| ID | Decision | Status | Blocks | Notes |
|---|---|---|---|---|
| D-01 | gearmulator component dependency closure (PatchDB, patch-manager UI, skin, MIDI Learn) — per component: copy clean, copy+shim, or reimplement | Open | M1.1, M2.1, M6.1 | Investigated in M0.2. Review gap #1. **Early finding (from M0.1):** skin system has migrated from JSON to RmlUi (RML/RCSS via `juceRmlUi`); patch-manager has two implementations (original + RmlUi). See `references/gearmulator.md`. |
| D-02 | CLAP in MVP or deferred? Requires `clap-juce-extensions` submodule if in. | Open | M1.1 | Decided at start of M1.1. Review gap #4. |
| D-03 | Xenia SysEx timing fidelity — does the 100ms SNDP rate limiter need hardware-only validation? | Open | — | Investigated in M1.3 (the validation step). Review gap #6. |
| D-04 | IDATA byte count vs spec §3.3 (plan says 28; verify before freezing `InstrumentData` struct) | Open | — | Investigated in M1.2 before struct is frozen. Review gap #7. |
| D-05 | MidiKraft-librarian standalone buildability | Open | M1.1 | Investigated in M0.2. Review gap #3. |
| D-06 | Exact filename + completeness of `parameterDescriptions_*.json` in gearmulator | Open | M1.2 | Investigated in M0.3. |

## 4. Milestones

### Phase 0 — Reference & Decoupling Spike

Goal: prove the borrowed-components strategy works before committing to it.

#### M0.1 — Acquire reference material into `references/`
**Effort:** S · **Depends on:** — · **Decisions:** —

**Scope:** Clone read-only into `references/`: gearmulator (Xenia/Microwave sources, parameter JSON, codec, PatchDB, skin, MIDI Learn), Edisyn (`WaldorfMicrowaveXT.java`, `Synth.java`), mwsd (autodetect + DISD parser). Nothing built, nothing compiled.

**Exit criteria:**
- [x] All three repos cloned at fixed commits, commit hashes recorded *(in `references/<name>.md`)*
- [x] Per-repo notes file (`references/<name>.md`) noting license and which files matter
- [x] Wave/wavetable codec source file located: `source/xtLib/xtState.cpp` (primary, ~L1050–1112), `xtRomWaves.cpp` (ROM-side)
- [x] Edisyn `WaldorfMicrowaveXT.java` located at `edisyn/synth/waldorfmicrowavext/`; `Synth.java` at `edisyn/`

**Status:** Completed 2026-06-15.

#### M0.2 — Decoupling spike *(gate)*
**Effort:** L · **Depends on:** M0.1 · **Decisions:** D-01, D-05

**Scope:** For each borrowed gearmulator component — PatchDB, patch-manager UI, skin system, MIDI Learn overlay — trace its real include/dependency closure. Decide per component: *copy clean* / *copy + small shim* / *reimplement*. Separately, verify MidiKraft-librarian builds as a standalone submodule.

**Exit criteria:**
- [ ] Per-component classification recorded in `ATTRIBUTIONS.md` draft notes
- [ ] If `copy + shim`: shim interface sketched
- [ ] If `reimplement`: rough effort added to roadmap as new milestones
- [ ] MidiKraft-librarian: standalone build attempted; result (works / needs sibling repos / doesn't build) recorded
- [ ] D-01 and D-05 marked Resolved with date

**Why it's a gate:** the source-strategy collapses if the borrowed components don't detach cleanly. Phase 1 scaffolding cannot start until the cost is known.

#### M0.3 — Confirm `parameterDescriptions` JSON
**Effort:** S · **Depends on:** M0.1 · **Decisions:** D-06

**Scope:** Verify exact filename, location, and completeness of the gearmulator parameter-description JSON for MW2/XT. This is "the most important single file in the project" — confirm it covers every SDATA/MDATA/IDATA/GDATA index with names, ranges, and value-to-text enumerations.

**Exit criteria:**
- [ ] Filename and path noted in `references/gearmulator/README.md`
- [ ] Coverage spot-check: 5 random parameters cross-checked against spec
- [ ] Any gaps logged as new decisions or accepted with note
- [ ] D-06 marked Resolved

---

### Phase 1 — Core Editor (MVP)

Goal: a working single-patch editor with reliable hardware communication, full skin system, and every parameter tab functional.

#### M1.1 — Project scaffold
**Effort:** M · **Depends on:** M0.2 · **Decisions:** D-02

**Scope:** Fresh JUCE CMake project. Submodules: JUCE, sqlite_orm, MidiKraft-librarian, juce-widgets, and `clap-juce-extensions` (if D-02 chooses CLAP-in-MVP). Create `source/{mw2xtLib,mw2xtEditor,mw2xtUI,mw2xtPlugin,patchManager}/`, `references/`, default-skin directory. Add AGPL-3.0 `LICENSE` and `ATTRIBUTIONS.md`. Empty Standalone + VST3 + AU (+ CLAP) build target on macOS first.

**Exit criteria:**
- [ ] Empty plugin opens in a DAW (Standalone, VST3, AU; CLAP if in scope)
- [ ] Cross-platform CMake works on macOS; Windows/Linux deferred to later milestone if needed
- [ ] `ATTRIBUTIONS.md` exists with provenance notes from M0.2
- [ ] D-02 marked Resolved

#### M1.2 — Protocol layer + unit tests *(gate)*
**Effort:** L · **Depends on:** M1.1 · **Decisions:** D-04

**Scope:** `mw2xtLib`: copy wave/wavetable codec verbatim from gearmulator, copy parameter JSON, write `SoundData` / `MultiData` / `GlobalData` structs with Edisyn-sourced clamping ranges. Confirm IDATA byte count (D-04) before freezing `InstrumentData`. Unit tests cover the correctness-critical paths.

**Exit criteria:**
- [ ] `ctest` green for: SNDP encode/decode (including HH=00h/01h split at index 128)
- [ ] `ctest` green for: SNDD checksum
- [ ] `ctest` green for: wave XOR-flip nibble codec (round-trip on known bytes)
- [ ] `ctest` green for: MULP IDM = 21h
- [ ] Spot-check: decoded parameter values match Edisyn output for the same SDATA bytes (≥5 patches)
- [ ] D-04 marked Resolved

**Why it's a gate:** silent wrong-parameter bugs at the protocol layer poison everything above it.

#### M1.3 — HardwareMidiDevice + dual test harness *(gate)*
**Effort:** L · **Depends on:** M1.2 · **Decisions:** D-03

**Scope:** `HardwareMidiDevice`: Universal Device Inquiry autodetect (from mwsd), SNDP send with rate limiter (100ms coalescing), SNDR/SNDD via MidiKraft-librarian, DISD parser. Stand up Xenia-over-IAC loopback as fast-iteration target alongside real-hardware testing.

**Exit criteria:**
- [ ] Every message type (SNDP/SNDR/SNDD/MULx/GLBx/WAVx/WCTx/DISx/RMTP/MODx) frames byte-identical to Edisyn on the IAC bus (MIDI monitor verified)
- [ ] Autodetect returns correct family/device ID against real XT
- [ ] 100ms SNDP coalescing validated on physical firmware (not just Xenia)
- [ ] D-03 marked Resolved — record whether Xenia is faithful enough for future rate-limiter work or whether hardware is mandatory

**Why it's a gate:** UI work without a verified comms layer just builds bug-laundry on top.

#### M1.4 — PatchModel + EditorController
**Effort:** M · **Depends on:** M1.3 · **Decisions:** —

**Scope:** Wire SDATA/MDATA/GDATA structs to `HardwareMidiDevice`. `juce::UndoManager` integration, dirty-state tracking, A/B compare buffers, CC receive routing, sound/multi mode context. No UI yet — model + controller layer only, exercised via tests.

**Exit criteria:**
- [ ] Programmatic patch edit pushes to undo stack and reaches hardware
- [ ] Incoming SysEx and CC both update model
- [ ] A/B compare swap behaves correctly
- [ ] CC routing covers all 44 XT hardware-knob CCs

#### M1.5 — OSC + FILTER tabs (vertical slice) *(gate)*
**Effort:** M · **Depends on:** M1.4 · **Decisions:** —

**Scope:** First two editor pages with the gearmulator skin system wired in. End-to-end proof of stack: knob in UI → SNDP → audible change on hardware; hardware knob → CC → UI updates. Padlock icons placed on knobs (visual only, inactive until Phase 5).

**Exit criteria:**
- [ ] OSC tab: octave/semitone/detune/keytrack/pitchbend scale/FM amount for both oscillators, sync/link toggles
- [ ] FILTER tab: filter 1 full controls including named type selector (24dB LP, 12dB BP, Waveshaper, etc.) with context-sensitive "special param" label; filter 2 below
- [ ] Skin renders correctly using Xenia assets
- [ ] Manual round-trip (UI→XT and XT→UI) validated on both Xenia and real hardware
- [ ] Undo/redo and A/B compare work on these tabs

**Why it's a gate:** validates the full vertical before fanning out to the remaining tabs. Discovers any architectural issues while the surface is small.

#### M1.6 — Remaining editor tabs
**Effort:** XL — split before starting · **Depends on:** M1.5 · **Decisions:** —

**Scope:** WAVE (with inline waveform preview), AMP, ENV (three envelopes simultaneously with graphical ADSR + 8-stage Wave Env), LFO (waveform icon selectors), ARP (16-step click grid), MISC, NAME (direct text input), GLOBAL, MULTI (8-instrument strip + MDATA header). Padlock icons on all knobs.

**Split when starting:** create M1.6a … M1.6i per tab after M1.5 sizing is known. Don't pre-estimate per-tab effort until one is done.

**Exit criteria (per tab subtask):**
- [ ] All section parameters visible without page navigation
- [ ] Bidirectional sync (UI ↔ XT) verified for every control
- [ ] Tab-specific visual affordances built (e.g., waveform preview, ADSR display, arp grid)

---

### Phase 2 — Patch Library

Goal: database-backed library with hardware bulk transfer and a browser that's a pleasure to use.

#### M2.1 — PatchDB wired
**Effort:** M · **Depends on:** M1.6 · **Decisions:** D-01

**Scope:** PatchDB SQLite layer from gearmulator (ROM-loading path stripped per D-01 resolution). Data-source management, auto-scan, schema migrations.

**Exit criteria:**
- [ ] DB created on first launch; survives plugin restart
- [ ] Manual import of a .syx file appears as patch in DB
- [ ] Auto-scan picks up new files in configured source folders

#### M2.2 — Browser panel
**Effort:** L · **Depends on:** M2.1 · **Decisions:** —

**Scope:** Grid + list views, full-text search, tag browser, three-state rating (favourite / neutral / hidden). Drag patch to hardware edit buffer. Drag to bank slot. Panel slides in as overlay; main editor remains visible.

**Exit criteria:**
- [ ] Both views render and switch
- [ ] Search filters in real time
- [ ] Drag-to-edit-buffer sends SNDD to BB=20h NN=00h and patch becomes the active edit
- [ ] Drag-to-bank sends SNDD to a specific bank slot

#### M2.3 — Bulk receive/send
**Effort:** M · **Depends on:** M2.2 · **Decisions:** —

**Scope:** MidiKraft-librarian sequential bank-by-bank bulk operations with progress bar (256-patch dump can take 30+ seconds on real hardware). File export as .syx and .mid.

**Exit criteria:**
- [ ] Full bank receive succeeds on real XT with progress feedback
- [ ] Full bank send round-trips: receive → modify one patch → send → re-receive matches
- [ ] Export to .syx and .mid produces files that re-import correctly

#### M2.4 — A/B compare via DB load
**Effort:** S · **Depends on:** M2.3 · **Decisions:** —

**Scope:** Load a library patch into the A/B compare buffer (already plumbed in M1.4). Highlight parameters that differ from the current edit.

**Exit criteria:**
- [ ] Changed-parameter highlight is visible on all editor tabs
- [ ] Swap A↔B sends SNDD and updates UI

#### M2.5 — Secondary MIDI device routing
**Effort:** S · **Depends on:** M2.3 · **Decisions:** —

**Scope:** Optional second MIDI device — typically a controller keyboard — routed through the editor for note-on/off.

**Exit criteria:**
- [ ] Device selector in settings
- [ ] Notes from secondary device reach the XT

---

### Phase 3 — Wavetable & Waveform Editor

Goal: draw, import, and upload custom waves and wavetables with a visualizer the hardware cannot offer.

#### M3.1 — Waveform editor
**Effort:** L · **Depends on:** M1.6 · **Decisions:** —

**Scope:** 64-sample canvas with pencil and line tools. Harmonic editor (additive). Audio file import with auto-trim to single cycle.

**Exit criteria:**
- [ ] Drawing on canvas updates the waveform model
- [ ] Harmonic editor and canvas stay synchronised
- [ ] Audio file import produces a usable single-cycle wave

#### M3.2 — Wavetable editor
**Effort:** M · **Depends on:** M3.1 · **Decisions:** —

**Scope:** 64-entry control table. Drag wave indices to table positions. Spectral interpolation preview between adjacent entries.

**Exit criteria:**
- [ ] Drag-and-drop builds a valid WCTDATA structure
- [ ] Interpolation preview matches what the XT will produce

#### M3.3 — Wave visualizer
**Effort:** M · **Depends on:** M3.2 · **Decisions:** —

**Scope:** 3D waterfall or stacked 2D display of all wavetable positions. Updates as the startwave knob moves on the WAVE tab.

**Exit criteria:**
- [ ] Visualizer renders without dropping the audio thread
- [ ] startwave knob movement scrubs the visualizer position

#### M3.4 — ROM wave browser
**Effort:** S · **Depends on:** M3.1 · **Decisions:** —

**Scope:** 300 ROM waves browsable read-only with visual preview.

**Exit criteria:**
- [ ] All 300 ROM waves visible
- [ ] Selecting one previews shape (no upload — read-only)

#### M3.5 — WAVR/WAVD + WCTR/WCTD upload/download
**Effort:** M · **Depends on:** M3.1, M3.2 · **Decisions:** —

**Scope:** User wave upload/download (indices 1000–1249) and user wavetable upload/download (indices 96–128) via WAVR/WAVD and WCTR/WCTD.

**Exit criteria:**
- [ ] Upload user wave → XT plays the uploaded wave at the right index
- [ ] Download user wave → editor reproduces what's on the XT byte-for-byte
- [ ] Same for wavetables

#### M3.6 — LCD mirror panel
**Effort:** S · **Depends on:** M1.3 · **Decisions:** —

**Scope:** Real-time XT LCD + LED state via DISD parser (ported from mwsd per D-01 resolution).

**Exit criteria:**
- [ ] Pressing buttons on XT updates the mirror display in real time
- [ ] LED state matches hardware

---

### Phase 4 — Modulation Matrix

Goal: a 2D grid replacing the hardware's slot-by-slot matrix navigation.

#### M4.1 — 2D grid view
**Effort:** L · **Depends on:** M1.6 · **Decisions:** —

**Scope:** 31 sources (§3.12) × 36 destinations (§3.13). Occupied cells show their amount value. Empty cells are click-to-add. Drag to set amount. All changes send SNDP immediately.

**Exit criteria:**
- [ ] Grid renders all 31 × 36 cells legibly
- [ ] Click-to-add creates a slot and sends SNDP
- [ ] Drag-amount updates value in real time without spamming firmware

#### M4.2 — 16-slot mapping with stacked-cell handling
**Effort:** M · **Depends on:** M4.1 · **Decisions:** —

**Scope:** 16 mod slots mapped onto the grid; multiple slots per cell shown stacked. Double-click to inspect/expand a stacked cell.

**Exit criteria:**
- [ ] Stacked cells visually distinguishable from single
- [ ] Inspector lists all slots in a cell with edit/remove
- [ ] Slot index assignment is stable across edits

#### M4.3 — Modifier slots panel
**Effort:** M · **Depends on:** M4.1 · **Decisions:** —

**Scope:** 4 modifier slots with source1, source2, operation (16 types), parameter. Modifier delay source and time.

**Exit criteria:**
- [ ] All 4 modifiers editable
- [ ] All 16 operations selectable with correct labels
- [ ] Delay source and time send SNDP correctly

---

### Phase 5 — Exploration Engine

Goal: the feature that defines this editor against every existing MW2/XT tool.

#### M5.1 — Port mutation weight table from Edisyn
**Effort:** S · **Depends on:** M0.1 · **Decisions:** —

**Scope:** Port per-parameter mutation weights from `WaldorfMicrowaveXT.java` into `ExplorationEngine`. Apache-2.0 attribution headers in every file.

**Exit criteria:**
- [ ] Weight table covers every SDATA parameter
- [ ] Apache-2.0 attribution present per file

#### M5.2 — Port six algorithms
**Effort:** L · **Depends on:** M5.1 · **Decisions:** —

**Scope:** Port from Edisyn `Synth.java` into `ExplorationEngine` (MIDI-decoupled, pure `SoundData` in/out): mutate, merge, nudge, morph, hill-climb, constriction.

**Exit criteria:**
- [ ] Each algorithm has a unit test producing deterministic output for a fixed seed
- [ ] Mutation respects per-section enable/disable
- [ ] Hill-climb converges on a synthetic fitness target in a test
- [ ] Morph interpolation is continuous (no discontinuities at corners)

#### M5.3 — Activate padlock buttons
**Effort:** S · **Depends on:** M5.2, M1.6 · **Decisions:** —

**Scope:** Wire parameter-lock mask to all padlock icons placed during Phase 1. Locked parameters excluded from all exploration operations.

**Exit criteria:**
- [ ] Padlock click toggles lock state and persists
- [ ] All six algorithms honour the lock mask

#### M5.4 — Exploration panel UI
**Effort:** L · **Depends on:** M5.2 · **Decisions:** —

**Scope:** Slide-in overlay with four sub-panels — Mutate, Merge/Nudge, Morph (2D XY pad), Hill-Climber (candidate grid + grade buttons + inline 3-4 step guide). MIDI CC assignable to morph X/Y axes.

**Exit criteria:**
- [ ] All four sub-panels functional end-to-end against real XT
- [ ] Morph XY sends SNDP continuously without firmware overrun
- [ ] Hill-climber: grade → evolve → next generation produces audibly different candidates
- [ ] Inline guide text present in hill-climber

---

### Phase 6 — Polish & Release

#### M6.1 — MIDI Learn overlay
**Effort:** M · **Depends on:** M1.6 · **Decisions:** D-01

**Scope:** Port right-click → assign CC overlay from gearmulator 2.2.2. All 44 XT hardware-knob CCs become assignable.

**Exit criteria:**
- [ ] Right-click any parameter shows overlay
- [ ] Learn mode captures next incoming CC
- [ ] Assignments persist across plugin restart

#### M6.2 — RMTP remote control
**Effort:** S · **Depends on:** M1.3 · **Decisions:** —

**Scope:** Wire Store, Compare, Recall, and other front-panel button simulations through RMTP (§3.7).

**Exit criteria:**
- [ ] Store/Compare/Recall buttons in editor produce the same XT behaviour as physical presses

#### M6.3 — Plugin automation
**Effort:** M · **Depends on:** M1.6 · **Decisions:** —

**Scope:** Expose SDATA parameters as VST/AU/CLAP automation parameters routed through the SNDP rate limiter.

**Exit criteria:**
- [ ] DAW automation lane writes work without overrunning firmware
- [ ] Parameter values round-trip correctly through DAW automation

#### M6.4 — Program change receive + clipboard copy/paste
**Effort:** S · **Depends on:** M2.1 · **Decisions:** —

**Scope:** Incoming Program Change highlights matching DB patch and optionally loads it. Patch copy/paste as JSON or raw bytes.

**Exit criteria:**
- [ ] PC message → matching patch highlighted in browser
- [ ] Copy/paste round-trips through system clipboard

#### M6.5 — Community skin support
**Effort:** S · **Depends on:** M1.5 · **Decisions:** —

**Scope:** Skin selector in settings, `skins/` folder alongside binary, skin hot-reload.

**Exit criteria:**
- [ ] Dropping a skin folder makes it appear in the selector
- [ ] Selecting a skin switches without restart

#### M6.6 — Xenia compatibility
**Effort:** S · **Depends on:** M1.3 · **Decisions:** —

**Scope:** Optional MIDI loopback routing to Xenia. Documented setup.

**Exit criteria:**
- [ ] Editor talks to Xenia identically to talking to real XT
- [ ] Setup documented for macOS/Windows/Linux

#### M6.7 — Release build pipeline
**Effort:** L · **Depends on:** all prior milestones · **Decisions:** —

**Scope:** CI pipeline for macOS (signing + notarization), Windows (signing), Linux (packaging). Versioned release artifacts.

**Exit criteria:**
- [ ] Tag push produces signed/notarized macOS Standalone + VST3 + AU (+ CLAP)
- [ ] Same for Windows
- [ ] Linux tarball or .deb/.AppImage
- [ ] AGPL-3.0 source bundle included in every release

---

## 5. Currently in flight

*(Update this line as work shifts. Keep it to one milestone.)*

→ **M0.2 — Decoupling spike** (gate). M0.1 complete. Early findings from M0.1 already inform the spike: skin system uses RmlUi now, patch-manager has two implementations.

## 6. Changelog

| Date | Change |
|---|---|
| 2026-06-15 | Initial roadmap created. Sequenced milestones M0.1 → M6.7 across Phases 0–6. Six open decisions logged (D-01 … D-06). |
| 2026-06-15 | M0.1 completed. References cloned (gearmulator @26cec55, edisyn @49f13d5, mwsd @391d99b). D-06 partially resolved (`parameterDescriptions_xt.json` located; completeness spot-check is M0.3). D-01 updated with skin/patch-manager findings. |
