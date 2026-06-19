# Waldorf Microwave II/XT Editor — Roadmap

**Repository:** https://github.com/jbboz/XT-Editor

Living execution document. The architectural reference is `mw2xt_editor_development_plan.md`; this doc is the ordered list of milestones I actually work from, plus the open decisions that gate them.

**Related specifications** in [`docs/spec/`](docs/spec/):
- [`editor-requirements.md`](docs/spec/editor-requirements.md) — per-milestone acceptance criteria (the "definition of done"); also contains the **[Testing strategy: Xenia + real hardware](docs/spec/editor-requirements.md#testing-strategy-xenia--real-hardware)** section
- [`sysex-protocol.md`](docs/spec/sysex-protocol.md) — our authoritative SysEx protocol reference, distilled from the Waldorf docs and cross-checked against Edisyn
- [`xenia-setup.md`](docs/xenia-setup.md) — concrete setup guide for the Xenia + IAC Bus development/test harness

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
| D-01 | gearmulator component dependency closure (PatchDB, patch-manager UI, skin, MIDI Learn) — per component: copy clean, copy+shim, or reimplement | **Resolved (2026-06-16)** | M1.1, M2.1, M6.1 | Investigated in M0.2. Classifications: **patchdb** = copy + shim; **patch-manager UI (both variants)** = reimplement (mutually intertwined, both require juceRmlUi + RmlUi + Lua + custom renderers); **skin system** = reimplement using Xenia PNG assets directly under JUCE LookAndFeel; **MIDI Learn core** = copy clean; **MIDI Learn UI** = reimplement. See [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md) for full per-component rationale. |
| D-02 | CLAP in MVP or deferred? Requires `clap-juce-extensions` submodule if in. | **Resolved (2026-06-16): Defer** | — | CLAP is not in the MVP. M1.1 builds Standalone + VST3 + AU only. `clap-juce-extensions` not added as a submodule. CLAP support can be revisited during Phase 6 polish once VST3/AU paths are proven. |
| D-03 | Xenia SysEx timing fidelity — does the 100ms SNDP rate limiter need hardware-only validation? | **Resolved (2026-06-19)** | — | Hardware test (`tools/sndp_rate_test.py`) on real XT. Normal parameters (Cutoff, Resonance, Detune): no drops at 20 ms intervals; no rate limiter needed; Xenia sufficient for all normal SNDP work. Wave parameter (SDATA 14): visible drops at fast rates; 100 ms throttle required; Xenia is NOT faithful for Wave rate testing. Blanket rate limiter removed — Wave-only throttle retained. See [`sysex-protocol.md` §Rate limiting](docs/spec/sysex-protocol.md) and NFR-2 in [`editor-requirements.md`](docs/spec/editor-requirements.md). |
| D-04 | IDATA byte count vs spec §3.3 (plan says 28; verify before freezing `InstrumentData` struct) | **Resolved (2026-06-16)** | — | Confirmed 28 bytes. Waldorf §3.3 enumerates indices 0–27; §2.22 MULD layout has each IDATA occupy 28 bytes (offsets 39–66, 67–94, …). Recorded in [`sysex-protocol.md` §IDATA](docs/spec/sysex-protocol.md). |
| D-05 | MidiKraft-librarian standalone buildability | **Resolved (2026-06-16)** | M1.1 | Investigated in M0.2. **Not feasible:** repo is archived, requires juce-utils + midikraft-base + nlohmann_json + fmt as deps, and ~9 abstract capability interfaces from midikraft-base to be implemented. **Path forward:** write our own focused SNDR→SNDD state machine in M2.3 (estimated 200–400 lines). See [`references/MidiKraft-librarian.md`](references/MidiKraft-librarian.md). |
| D-06 | Exact filename + completeness of `parameterDescriptions_*.json` in gearmulator | **Resolved (2026-06-16)** | M1.2 | Investigated in M0.3. Filename: `source/xtJucePlugin/parameterDescriptions_xt.json`. Format: JSONC. 229 unique indices covering 0–255; 27 omissions all match Waldorf §3.1 reserved slots. Bonus: the JSON is a unified table covering SDATA + MDATA + IDATA via `page` field. One follow-up open question (Filter 1 Type indices 10–12 reality-check) tracked in [`docs/spec/sysex-protocol.md` §Open items](docs/spec/sysex-protocol.md). |

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
- [x] Per-component classification recorded in [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md)
- [x] If `copy + shim`: shim approach documented (patchdb, MIDI Learn core)
- [x] If `reimplement`: roadmap milestones adjusted (M1.1 effort raised, M2.2 split-on-start, M6.1 scope clarified)
- [x] MidiKraft-librarian: standalone build attempted — **not feasible**; details in [`references/MidiKraft-librarian.md`](references/MidiKraft-librarian.md)
- [x] D-01 and D-05 marked Resolved (2026-06-16)

**Why it's a gate:** the source-strategy collapses if the borrowed components don't detach cleanly. Phase 1 scaffolding cannot start until the cost is known.

**Status:** Completed 2026-06-16. The spike found that the dev plan's borrowing assumptions are stale for the post-RmlUi gearmulator: patch-manager UI and skin are reimplements; MidiKraft-librarian is not standalone-buildable. Net: more in-house work than the dev plan assumed, but the alternatives (forking gearmulator's whole UI stack, vendoring 4+ MidiKraft submodules) are worse. PatchDB and MIDI Learn core remain borrowable cleanly.

#### M0.3 — Confirm `parameterDescriptions` JSON
**Effort:** S · **Depends on:** M0.1 · **Decisions:** D-06

**Scope:** Verify exact filename, location, and completeness of the gearmulator parameter-description JSON for MW2/XT. This is "the most important single file in the project" — confirm it covers every SDATA/MDATA/IDATA/GDATA index with names, ranges, and value-to-text enumerations.

**Exit criteria:**
- [x] Filename confirmed: `source/xtJucePlugin/parameterDescriptions_xt.json` (2227 lines, JSONC format)
- [x] Coverage spot-check: 10 parameters across OSC / WAVE / FILTER / ENV / LFO / MOD / NAME all present with names, ranges, and value-to-text mappings
- [x] No SDATA gaps relative to Waldorf §3.1 (all 27 omissions match Waldorf "reserved" slots)
- [x] One open question logged: Filter 1 Type indices 10–12 in JSON vs 0–9 in Waldorf — verify on real hardware in M1.5
- [x] D-06 marked Resolved 2026-06-16

**Bonus findings:** The JSON is a unified table covering SDATA + MDATA + IDATA via a `page` field (better than dev plan assumed). Top-level structure includes `valuelists` (50 named mappings), `midipackets`, and `controllerMap`. See [`references/gearmulator.md`](references/gearmulator.md) §"M0.3 findings".

**Status:** Completed 2026-06-16.

---

### Phase 1 — Core Editor (MVP)

Goal: a working single-patch editor with reliable hardware communication, full skin system, and every parameter tab functional.

#### M1.1 — Project scaffold
**Effort:** L (was M; raised after M0.2 spike — skin system is now in-scope work, not a borrow) · **Depends on:** M0.2 · **Decisions:** —

**Scope:** Fresh JUCE CMake project. Submodules: JUCE, sqlite_orm. **No CLAP** (D-02 deferred). **No MidiKraft-librarian** (D-05). **No juce-widgets** (M2.2 reimplemented). Create `source/{mw2xtLib,mw2xtEditor,mw2xtUI,mw2xtPlugin,patchManager}/`, `references/`, `source/mw2xtUI/skins/xtDefault/` (scaffold for Xenia-derived skin assets, wired in M1.5). Add AGPL-3.0 `LICENSE`. `ATTRIBUTIONS.md` already drafted (from M0.2). Empty Standalone + VST3 + AU build target on macOS first.

**Exit criteria:**
- [x] Empty plugin builds and produces well-formed Standalone (.app), VST3 (.vst3), AU (.component) bundles (Apple Silicon)
- [x] AU bundle has correct AU metadata: `type=aumi` (MIDI Processor), `subtype=MwXT`, `manufacturer=RPAu`
- [x] All bundles ad-hoc code-signed; hardened runtime enabled
- [x] AU passes Apple's `auval -v aumi MwXT RPAu` validation suite (Cocoa view present, class info OK, host callbacks OK, parameter info OK — full pass)
- [x] CMake builds cleanly on macOS (Xcode generator, CMake 4.3.2, Xcode 26.5, JUCE 8.0.13). Windows/Linux deferred.
- [x] `ATTRIBUTIONS.md` updated with per-file entries for the copied skin assets
- [x] Skin asset directory populated: 35 PNG bitmaps + `Digital.ttf` + `xtDefault.json` (12 MB total) copied from `references/gearmulator/source/xtJucePlugin/skins/xtDefault/` under GPL-3.0 attribution

**Status:** Completed 2026-06-16.

**Verification note:** `auval` is what AU hosts (Logic, MainStage, AU Lab) run before loading a plugin — passing the full suite means the AU is well-formed enough that a host will accept it. The same `mw2xtEditor_SharedCode` static library backs the VST3 and Standalone formats, so the AU pass is a strong proxy for those too. Hosting in an actual DAW remains a useful smoke test but is no longer a strict gate.

#### M1.2 — Protocol layer + unit tests *(gate)*
**Effort:** L · **Depends on:** M1.1 · **Decisions:** D-04

**Scope:** `mw2xtLib`: copy wave/wavetable codec verbatim from gearmulator, copy parameter JSON, write `SoundData` / `MultiData` / `GlobalData` structs with Edisyn-sourced clamping ranges. Confirm IDATA byte count (D-04) before freezing `InstrumentData`. Unit tests cover the correctness-critical paths.

**Exit criteria:**
- [x] `ctest` green for: SNDP encode/decode (including HH=00h/01h split at index 128)
- [x] `ctest` green for: SNDD checksum
- [x] `ctest` green for: wave XOR-flip nibble codec (round-trip on known bytes)
- [x] `ctest` green for: MULP IDM = 21h
- [x] Spot-check: decoded parameter values match Edisyn output for the same SDATA bytes (≥5 patches)
- [x] D-04 marked Resolved

**Status:** Completed 2026-06-19.

**Why it's a gate:** silent wrong-parameter bugs at the protocol layer poison everything above it.

#### M1.3 — HardwareMidiDevice + dual test harness *(gate)*
**Effort:** L · **Depends on:** M1.2 · **Decisions:** D-03

**Scope:** `HardwareMidiDevice`: Universal Device Inquiry autodetect (from mwsd), SNDP send with rate limiter (100ms coalescing), SNDR/SNDD via our own state machine (D-05), DISD parser. Stand up the dual test harness per [Testing strategy: Xenia + real hardware](docs/spec/editor-requirements.md#testing-strategy-xenia--real-hardware) — Xenia-over-IAC for fast iteration, real XT for sign-off; setup details in [`docs/xenia-setup.md`](docs/xenia-setup.md).

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
**Effort:** M · **Depends on:** M1.6 · **Decisions:** —

**Scope:** Copy `jucePluginLib/patchdb/*` from gearmulator (per [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md) D-01 resolution: copy + small shim). Take supporting `baseLib/{binarystream,filesystem,hybridcontainer}.h` and `synthLib/{midiToSysex,midiTypes}.h`. Shim out `dsp56kBase/{logging,threadtools}.h` with std/JUCE equivalents. Strip ROM-loading path. Data-source management, auto-scan, schema migrations.

**Exit criteria:**
- [ ] DB created on first launch; survives plugin restart
- [ ] Manual import of a .syx file appears as patch in DB
- [ ] Auto-scan picks up new files in configured source folders

#### M2.2 — Browser panel
**Effort:** XL — split before starting (was L; raised after M0.2 spike — gearmulator's patch-manager UI is RmlUi-coupled and reimplemented JUCE-native per D-01 resolution) · **Depends on:** M2.1 · **Decisions:** —

**Scope:** Build a JUCE-native browser UI on top of the borrowed PatchDB (M2.1). Grid + list views, full-text search, tag browser, three-state rating (favourite / neutral / hidden). Drag patch to hardware edit buffer. Drag to bank slot. Panel slides in as overlay; main editor remains visible.

**Split when starting:** create M2.2a (browser shell + grid view), M2.2b (list view + search), M2.2c (tag browser + ratings), M2.2d (drag-and-drop integration) once M2.1 lands.

**Exit criteria:**
- [ ] Both views render and switch
- [ ] Search filters in real time
- [ ] Drag-to-edit-buffer sends SNDD to BB=20h NN=00h and patch becomes the active edit
- [ ] Drag-to-bank sends SNDD to a specific bank slot

#### M2.3 — Bulk receive/send
**Effort:** M · **Depends on:** M2.2 · **Decisions:** —

**Scope:** Implement our own sequential SNDR→SNDD state machine (per D-05 resolution — MidiKraft-librarian not feasible). Per-patch timeout/retry, handler stack for incoming interleaved DISD/MODD/RMTP, JUCE-thread-safe progress callbacks. 256-patch dump takes 30+ seconds on real hardware. Reference `references/MidiKraft-librarian/Librarian.cpp` for the original implementation's patterns. File export as .syx and .mid.

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
**Effort:** M · **Depends on:** M1.6 · **Decisions:** —

**Scope:** Copy MIDI Learn **core** from gearmulator (`jucePluginLib/midiLearnMapping.*`, `midiLearnManager.*`, `midiLearnTranslator.*`, `midiLearnPreset.*`) per D-01 resolution: copy clean. **Reimplement the overlay UI** in JUCE-native (the gearmulator overlay is RmlUi-coupled). Right-click any parameter → assign CC. All 44 XT hardware-knob CCs become assignable.

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

→ **M1.3 — HardwareMidiDevice + dual test harness** (gate). M1.2 complete: mw2xtLib builds JUCE-free, ctest green for all M1.2 exit criteria.

## 6. Changelog

| Date | Change |
|---|---|
| 2026-06-15 | Initial roadmap created. Sequenced milestones M0.1 → M6.7 across Phases 0–6. Six open decisions logged (D-01 … D-06). |
| 2026-06-15 | M0.1 completed. References cloned (gearmulator @26cec55, edisyn @49f13d5, mwsd @391d99b). D-06 partially resolved (`parameterDescriptions_xt.json` located; completeness spot-check is M0.3). D-01 updated with skin/patch-manager findings. |
| 2026-06-16 | Added `docs/spec/`: editor requirements (per-milestone acceptance criteria), authoritative SysEx protocol spec (distilled from Waldorf PDFs, cross-checked against Edisyn), and per-spec README. Waldorf PDFs kept locally only (gitignored). D-04 (IDATA byte count) resolved as 28. |
| 2026-06-16 | M0.2 decoupling spike completed. D-01 resolved: patchdb = copy+shim, patch-manager UI = reimplement (both variants mutually intertwined, both require juceRmlUi/RmlUi/Lua stack), skin = reimplement using Xenia PNG assets directly, MIDI Learn core = copy clean, MIDI Learn UI = reimplement. D-05 resolved: MidiKraft-librarian not standalone-buildable (archived, requires juce-utils + midikraft-base + 9 capability interfaces); writing our own state machine in M2.3. ATTRIBUTIONS.md draft created. Milestone scope updates: M1.1 effort raised M→L (skin in-house now), M2.2 raised L→XL (browser UI reimplement, split on start), M1.1 submodule list trimmed (no MidiKraft-librarian, no juce-widgets). |
| 2026-06-16 | D-02 resolved: defer CLAP. M1.1 ships Standalone + VST3 + AU only; CLAP can be added in Phase 6 polish. `clap-juce-extensions` not added as a submodule. |
| 2026-06-16 | Docs additions: `README.md` (project front page), `docs/xenia-setup.md` (IAC Bus + Xenia setup guide), and a new "Testing strategy: Xenia + real hardware" section in `editor-requirements.md` (consolidates the dual-target posture, trust boundary, and milestone-by-milestone Xenia use). M1.3 scope simplified to point at the new section instead of repeating the intent. |
| 2026-06-16 | **M1.1 completed.** JUCE 8.0.13 and sqlite_orm v1.9.1 added as submodules at pinned commits. Source tree created (`source/{mw2xtLib,mw2xtEditor,mw2xtUI,mw2xtPlugin,patchManager}/`). Minimal AudioProcessor + AudioProcessorEditor shell in `mw2xtPlugin/` — empty 960×600 window with "MW2/XT Editor" placeholder. Top-level `CMakeLists.txt` configures the plugin as a MIDI Processor (`IS_MIDI_EFFECT TRUE`, `AU_MAIN_TYPE kAudioUnitType_MIDIProcessor`). All three formats — Standalone (.app), VST3 (.vst3), AU (.component) — build successfully on macOS / Apple Silicon (Xcode 26.5, JUCE 8.0.13). AU bundle metadata verified: `type=aumi`, `subtype=MwXT`, `manufacturer=RPAu`. **`auval -v aumi MwXT RPAu` passes the full validation suite** (Cocoa view, class info, host callbacks, parameter info). 12 MB of Xenia skin assets (35 PNGs + Digital.ttf + xtDefault.json) copied to `source/mw2xtUI/skins/xtDefault/` with per-file ATTRIBUTIONS entry. AGPL-3.0 LICENSE file added at repo root. Bundle id `com.retroproaudio.mw2xtEditor`. **Next: M1.2 protocol layer.** |
| 2026-06-19 | **M1.2 completed.** mw2xtLib static library (no JUCE): SoundData/MultiData/GlobalData structs (static_assert sizes), all Waldorf SysEx message encoders/decoders (SNDP/SNDD/MULP/MULD/GLBD/GLBP/WAVD/WCTD/DISD and all request types), wave XOR-flip nibble codec (adapted from gearmulator). parameterDescriptions_xt.json copied from reference. CTest green: SNDP HH split, SNDD checksum + 5-patch round-trip, MULP IDM=0x21, wave codec round-trip. **Next: M1.3 HardwareMidiDevice.** |
| 2026-06-19 | **D-03 resolved.** Hardware test on real XT (`tools/sndp_rate_test.py`): normal parameters show no drops at 20 ms intervals — blanket rate limiter removed. Wave parameter (SDATA 14) shows visible drops at fast rates — 100 ms throttle-with-trailing-send retained for Wave only. Xenia is sufficient for all normal SNDP correctness work but not for Wave rate validation. NFR-2, trust-boundary table, and protocol spec rate-limiting section updated accordingly. |
| 2026-06-16 | M0.3 completed. D-06 resolved: `parameterDescriptions_xt.json` is 2227-line JSONC (`//` comments — needs tolerant parser), 229 unique indices covering 0–255, all 27 SDATA omissions match Waldorf-reserved slots. Bonus: the JSON is a unified table covering SDATA + MDATA + IDATA via a `page` field, with `valuelists` / `midipackets` / `controllerMap` top-level sections (richer than dev plan assumed). One open question logged in `sysex-protocol.md` (Filter 1 Type 0–12 in JSON vs 0–9 in Waldorf §3.15). **Phase 0 complete; next is M1.1 project scaffold.** |
