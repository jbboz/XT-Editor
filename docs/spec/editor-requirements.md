# Editor Requirements

Per-milestone acceptance criteria — the "definition of done" for each ROADMAP milestone. This document complements [`../../ROADMAP.md`](../../ROADMAP.md) (execution sequencing) and [`sysex-protocol.md`](sysex-protocol.md) (the wire format). When something here disagrees with the dev plan, this document is authoritative.

## How to read

- **Functional requirements** describe behaviour: what must be true of the system from a user's or caller's point of view. Checkboxes are the testable form.
- **Non-functional requirements** describe constraints that don't fit a single feature: performance budgets, reliability, error handling, UX-wide invariants.
- **Open questions** call out items where the requirement is intentionally undefined — to be resolved during the milestone, not after.
- Milestone IDs match `ROADMAP.md`. Effort, dependencies, and gate status live in the roadmap; not duplicated here.
- Later phases (M2–M6) are intentionally less detailed; they'll be expanded as each phase becomes the in-flight one.

## Cross-cutting non-functional requirements

These apply across the whole editor regardless of milestone.

- **NFR-1 (Thread safety):** No SysEx round-trips on the JUCE message thread. All hardware I/O happens off the UI thread; results are posted back via `MessageManager::callAsync` or equivalent.
- **NFR-2 (Rate limiting):** Every outbound SNDP/MULP/GLBP path goes through the per-parameter 100ms coalescing queue (see [Waldorf §2.13 + protocol spec](sysex-protocol.md)). Direct sends bypassing the rate limiter are a defect.
- **NFR-3 (Determinism in tests):** All exploration algorithms (M5.x) accept an explicit RNG seed and produce reproducible output for a given seed.
- **NFR-4 (Undo for every user edit):** Any parameter change initiated through the UI pushes a single undoable action. Hardware-originated changes (incoming CC/SysEx) do not push to the undo stack.
- **NFR-5 (Skin failure mode):** If a skin asset fails to load, the editor falls back to the built-in default skin and surfaces a non-modal error toast. The editor never crashes from a malformed skin.
- **NFR-6 (Hardware absence):** All editor functions that don't require the XT (browsing patches, editing parameters into the model, exploration without audition) work with no MIDI device connected. The "Send" path is the only one that errors when disconnected.
- **NFR-7 (Bidirectional sync invariant):** For any parameter, an edit applied via UI is reflected in the model and reaches hardware; a change received from hardware updates the model and the UI. There is one source of truth per parameter (the model), and both sides converge on it.

---

# Phase 0 — Reference & Decoupling Spike

## M0.1 — Acquire reference material

**Status:** ✅ Completed 2026-06-15.

**Functional requirements:**
- [x] gearmulator, Edisyn, and mwsd cloned into `references/` at pinned commits
- [x] Per-repo notes file at `references/<name>.md` records source URL, commit hash, license, and per-milestone usage map
- [x] `references/README.md` indexes the files-of-interest across the three sources
- [x] `parameterDescriptions_xt.json` path located in gearmulator
- [x] Wave/wavetable nibble codec located in gearmulator
- [x] Edisyn `WaldorfMicrowaveXT.java` and `Synth.java` located

## M0.2 — Decoupling spike *(gate)*

**Status:** ✅ Completed 2026-06-16. Findings in [`../../ATTRIBUTIONS.md`](../../ATTRIBUTIONS.md) and [`../../references/MidiKraft-librarian.md`](../../references/MidiKraft-librarian.md).

**Goal:** know the true cost of borrowing each gearmulator component before scaffolding starts.

**Functional requirements:**
- [x] For each gearmulator component — `jucePluginLib/patchdb/`, `jucePluginEditorLib/patchmanager*/`, the RmlUi-based skin system (`juceRmlUi/`), MIDI Learn (woven into `jucePluginLib`) — produced a classification: copy clean, copy + small shim, or reimplement
- [x] Classification recorded in `ATTRIBUTIONS.md` with include/dependency closure
- [x] Shim approach documented for the copy+shim items (patchdb, MIDI Learn core)
- [x] Roadmap milestones adjusted for the reimplement items (M1.1, M2.2, M6.1)
- [x] MidiKraft-librarian standalone build attempted; result recorded in `references/MidiKraft-librarian.md` (not feasible — repo archived, requires 4+ external libs and ~9 abstract capability interfaces)
- [x] D-01 and D-05 marked Resolved 2026-06-16

**Non-functional requirements:**
- NFR-M0.2-1: The spike did not modify any borrowed source. ✓

**Outcomes that fed back into the requirements / roadmap:**
- The two patch-manager variants (original + RmlUi) are mutually dependent — both require juceRmlUi + RmlUi + Lua + custom renderers. Not separable.
- The Xenia skin's JSON layout file (`xtDefault.json`) is still present alongside the newer RmlUi assets — we may parse it for layout coordinates when building our own skin layer, or just use it as a reference for layout intent.
- MIDI Learn separates cleanly into a pure-logic core and an RmlUi-coupled UI; we take the core and reimplement the UI.

## M0.3 — Confirm `parameterDescriptions` JSON

**Status:** ✅ Completed 2026-06-16. Findings in [`../../references/gearmulator.md`](../../references/gearmulator.md) §"M0.3 findings".

**Functional requirements:**
- [x] Filename `source/xtJucePlugin/parameterDescriptions_xt.json` confirmed (2227 lines). Format is **JSONC** (`//` line comments) — protocol layer must use a tolerant parser or strip comments at build time.
- [x] Coverage spot-check (10 indices across OSC/WAVE/FILTER/ENV/LFO/MOD/NAME): all present with name, range, and value-to-text mapping
- [x] 27 SDATA indices missing from JSON — all 27 match Waldorf §3.1 reserved slots exactly
- [x] D-06 marked Resolved 2026-06-16

**Findings beyond the requirement:**
- The JSON is a **unified parameter table** covering SDATA + MDATA + IDATA, disambiguated by the `page` field. One source of metadata for all three contexts.
- Top-level keys include `parameterdescriptiondefaults`, `parameterdescriptions`, `regions`, `valuelists` (50 named value-to-text mappings), `midipackets`, and `controllerMap` — richer than the dev plan assumed.
- **Open question (deferred to M1.5):** Filter 1 Type in the JSON enumerates 13 values (0–12) vs Waldorf §3.15's 10 (0–9). Indices 10–12 (Notch24, Notch12, BandStop12) might be late firmware additions or Xenia-only extensions — verify on real hardware.

**Implementation note for M1.2:**
- The protocol layer needs a JSONC-tolerant parser. Two reasonable options:
  - Use `nlohmann/json` with `parse(..., callback, allow_exceptions, ignore_comments=true)` — JUCE ecosystem-friendly
  - Strip `//` and `/* */` at build time via CMake `add_custom_command` and parse plain JSON at runtime — simpler, no extra dep

---

# Phase 1 — Core Editor (MVP)

## M1.1 — Project scaffold

**Functional requirements:**
- [ ] Fresh JUCE CMake project builds an empty Standalone, VST3, and AU plugin on macOS (CLAP deferred per D-02)
- [ ] Submodules in place: JUCE, sqlite_orm (CLAP, MidiKraft-librarian, juce-widgets all dropped per D-02 / D-05 / M0.2 spike)
- [ ] Source tree matches `mw2xt_editor_development_plan.md` §7: `source/{mw2xtLib,mw2xtEditor,mw2xtUI,mw2xtPlugin,patchManager}/`, `references/`, default-skin directory placeholder
- [ ] `LICENSE` (AGPL-3.0) and `ATTRIBUTIONS.md` present with provenance from M0.2 spike

**Non-functional requirements:**
- NFR-M1.1-1: Empty plugin opens in at least one DAW (e.g., Reaper or Logic) without crashing or printing errors.
- NFR-M1.1-2: Build instructions in `README.md` are correct enough that a fresh checkout on macOS, with prerequisites installed, builds in one command.

## M1.2 — Protocol layer + unit tests *(gate)*

**Functional requirements — data structures:**
- [ ] `SoundData` is a 256-byte fixed-size struct/array matching Waldorf §3.1 (see [protocol spec §SDATA](sysex-protocol.md))
- [ ] `MultiData` (MDATA) is 32 bytes per Waldorf §3.2
- [ ] `InstrumentData` (IDATA) is 28 bytes per Waldorf §3.3 (resolves D-04)
- [ ] `GlobalData` (GDATA) is 32 bytes per Waldorf §3.6
- [ ] Typed accessors for every named parameter; clamping ranges match Edisyn `WaldorfMicrowaveXT.java`
- [ ] "Reserved" fields are preserved on round-trip (read in, write back unchanged) and zero-initialized on fresh patches

**Functional requirements — message framing:**
- [ ] All message encoders/decoders implemented for: SNDR, SNDD, SNDP, MULR, MULD, MULP, GLBR, GLBD, GLBP, WAVR, WAVD, WCTR, WCTD, DISR, DISD, RMTP, MODR, MODD, Universal Device Inquiry response
- [ ] SNDP HH byte: `00h` for parameter indices 0–127, `01h` for 128–255 (silent-wrong-value bug if reversed)
- [ ] MULP IDM is `21h`, **not** `20h` as the Waldorf doc table says (see [protocol spec §Divergences](sysex-protocol.md))
- [ ] SNDP and GLBP have **no** checksum byte (omitted per Waldorf §2.13, §2.53)
- [ ] All other dump messages include checksum = sum of databytes `& 0x7F`, and decoder accepts `0x7F` as universally-valid checksum

**Functional requirements — codecs:**
- [ ] WDATA nibble codec round-trips: encode(decode(bytes)) == bytes for any valid 128-nibble buffer
- [ ] WDATA respects the XOR-flip signed format: `signed char s = Wave[n] ^ 0x80` (Waldorf §3.4)
- [ ] WCTDATA codec handles 4-nibble per-index encoding (256 nibbles for 64 entries, Waldorf §3.5)

**Unit test acceptance:**
- [ ] All encoder/decoder paths have unit tests; `ctest` is green
- [ ] Round-trip test: ≥5 known SDATA byte arrays decoded into typed values and re-encoded match Edisyn output for the same input
- [ ] Checksum computation matches Waldorf §1 ("sum of databytes truncated to 7 bits"); test includes a corrupted-checksum negative case

**Non-functional requirements:**
- NFR-M1.2-1: No protocol-layer code depends on JUCE. The protocol layer is plain C++17, testable without an audio host.
- NFR-M1.2-2: All struct layouts have static asserts confirming size in bytes (catches accidental drift).

## M1.3 — HardwareMidiDevice + dual test harness *(gate)*

**Functional requirements — autodetect:**
- [ ] `HardwareMidiDevice::autodetect()` sends Universal Device Inquiry `F0 7E 7F 06 01 F7` and parses the response (Waldorf §4)
- [ ] Detected device returns family code, member code (MW2 / MW2-XT-mainboard / Microwave XT / MWPC / expandable / voice-expanded), and device ID
- [ ] If no response within configurable timeout (default 500 ms), autodetect returns "no device found" and does not block longer

**Functional requirements — outbound:**
- [ ] SNDP sends route through the per-parameter coalescing queue with 100 ms window (NFR-2)
- [ ] SNDR/SNDD bulk dumps route through MidiKraft-librarian's request/response state machine with progress callbacks
- [ ] All other message types (MUL*, GLB*, WAV*, WCT*, DIS*, RMTP, MOD*) have direct send methods

**Functional requirements — inbound:**
- [ ] Incoming SysEx is dispatched to per-message-type registered callbacks
- [ ] Incoming CC matching the CC table (Waldorf controls PDF) updates the model
- [ ] DISD parses 80 ASCII chars (LCD upper+lower row) and LED bitmask per Waldorf §2.62; exposes both as observable state

**Functional requirements — dual harness:**
- [ ] Editor can be configured to talk to Xenia (via IAC bus loopback) or real XT hardware via JUCE MIDI device selection
- [ ] All message types verified byte-identical to Edisyn output on the IAC bus (MIDI monitor confirmation)
- [ ] Autodetect verified against real XT (returns correct device family/member)
- [ ] 100 ms SNDP coalescing verified against physical XT firmware (resolves D-03 — record whether Xenia is faithful enough for future timing work)

**Non-functional requirements:**
- NFR-M1.3-1: No MIDI I/O blocks the audio thread; all SysEx callbacks marshal to message thread before touching UI
- NFR-M1.3-2: If hardware is disconnected mid-session, outbound sends fail silently and post a non-modal status; inbound callback registrations survive a reconnect

## M1.4 — PatchModel + EditorController

**Functional requirements — model:**
- [ ] `PatchModel` owns one `SoundData` (current edit), one `MultiData` + 8 `InstrumentData` (current multi), one `GlobalData`
- [ ] Programmatic parameter writes go through typed setters that clamp to Edisyn-verified ranges
- [ ] A separate A/B compare buffer holds an alternative `SoundData`; swap operation is atomic and triggers UI refresh

**Functional requirements — controller:**
- [ ] `EditorController` owns: `juce::UndoManager`, dirty flag, A/B compare state, exploration engine instance (Phase 5), CC receive routing, sound/multi mode
- [ ] Dirty flag clears on store-to-bank and on load-from-DB; sets on any parameter edit
- [ ] CC routing covers all 44 XT hardware-knob CCs per Waldorf controls PDF and dispatches to the corresponding SDATA index

**Acceptance:**
- [ ] Programmatic edit pushes one undo entry, model reflects change, SNDP issued via HardwareMidiDevice
- [ ] Incoming SysEx and incoming CC both update model without creating undo entries (NFR-4)
- [ ] A/B swap toggles between two SoundData values without losing either

## M1.5 — OSC + FILTER tabs (vertical slice) *(gate)*

**Functional requirements — OSC tab:**
- [ ] Controls present for both oscillators: Octave (SDATA 1/12, range 16–112 = -4..+4), Semitone (2/13, 52–76 = -12..+12), Detune (3/14), Pitch Bend Scale (5/17, 0..122 incl. "harmonic", "global"), Keytrack (6/18, 0..76 = -100%..+200%), FM Amount (7, Osc 1 only)
- [ ] Osc 2 Sync (16) and Osc 2 Link (19) toggles present
- [ ] Each control's displayed value matches Waldorf "Value" column (signed where applicable; symbolic names where Waldorf uses them)

**Functional requirements — FILTER tab:**
- [ ] Filter 1 controls: Cutoff (62), Resonance (63), Type dropdown (64, 10 entries per Waldorf §3.15), Keytrack (65), Env Amount (66), Env Velocity (67), Special Param (70)
- [ ] Filter 1 Type selector shows the human-readable names from §3.15 (not numeric indices)
- [ ] Filter 1 Special Param label updates when filter type changes — the label reflects the type's contextual meaning (e.g., "Waveshape Drive" for type 5/6, "FM Source" for type 8, etc.). Authoritative label-per-type mapping is in [protocol spec §Filter Type Context](sysex-protocol.md).
- [ ] Filter 2 controls: Cutoff (73), Type (74 — 2 entries: 6dB LP / 6dB HP), Keytrack (75)

**Functional requirements — skin + lifecycle:**
- [ ] Skin renders both tabs using the Xenia-derived default skin
- [ ] Padlock icons appear next to every knob (visual only — locking logic deferred to Phase 5)
- [ ] Tab switching is instant; no audible/visual glitch when switching during a SysEx round-trip

**Acceptance (gate):**
- [ ] Knob edit in UI → SNDP → audible change on hardware (verified on Xenia and real XT)
- [ ] Hardware knob turn → CC → model + UI update (verified on real XT for the 44 mapped CCs that intersect OSC/FILTER tabs)
- [ ] Undo/redo restores the previous value and updates both the model and the visible knob
- [ ] A/B compare swap is reflected on both tabs

**Non-functional requirements:**
- NFR-M1.5-1: UI knob drag at 60 Hz mouse rate produces ≤10 SNDP messages per second per parameter (rate limiter working)
- NFR-M1.5-2: Filter Type dropdown change does not produce a visible flicker on the Special Param label

## M1.6 — Remaining editor tabs

**To be detailed when this milestone becomes in-flight.** Per-tab subtasks (M1.6a … M1.6i) will get their own acceptance-criteria sections. Tabs to cover: WAVE, AMP, ENV, LFO, ARP, MISC, NAME, GLOBAL, MULTI.

**Cross-tab requirements that already apply:**
- All section parameters visible without page navigation (mirrors hardware section grouping, not the page system)
- Each tab integrates with undo, A/B compare, dirty flag (NFR-4, M1.4)
- Padlock icons placed on all knobs (visual only until M5.3)

---

# Phase 2 — Patch Library

**To be detailed when Phase 2 becomes in-flight.** Currently captured in roadmap exit criteria:
- M2.1 — PatchDB wired (SQLite, data sources, auto-scan)
- M2.2 — Browser panel (grid/list, search, tags, drag-to-bank/edit-buffer)
- M2.3 — Bulk receive/send via MidiKraft-librarian with progress; export to .syx/.mid
- M2.4 — A/B compare via DB load with changed-param highlighting
- M2.5 — Secondary MIDI device routing

**Known cross-cutting requirements for Phase 2:**
- DB schema versioned with migrations from the start (so future schema changes don't lose data)
- Bulk send/receive is cancellable mid-operation without corrupting the DB or hardware state

---

# Phase 3 — Wavetable & Waveform Editor

**To be detailed when Phase 3 becomes in-flight.** Currently captured in roadmap exit criteria for M3.1–M3.6.

**Known cross-cutting requirements for Phase 3:**
- Wave/wavetable visualizers render at ≥30 fps on a 5-year-old Mac without dropping audio
- User-wave upload range 1000–1249 enforced at the UI layer (cannot accidentally overwrite ROM waves)
- User-wavetable upload range 96–128 enforced

---

# Phase 4 — Modulation Matrix

**To be detailed when Phase 4 becomes in-flight.** Currently captured in roadmap exit criteria for M4.1–M4.3.

**Known cross-cutting requirements for Phase 4:**
- 32 sources × 36 destinations per [protocol spec §Modulation Sources / §Modulation Destinations](sysex-protocol.md) (dev plan said 31 × 36; Waldorf §3.12 has 32 entries 0–31, §3.13 has 36 entries 0–35)
- 16 mod slot indices map onto the grid; ordering within a stacked cell is stable and observable

---

# Phase 5 — Exploration Engine

**To be detailed when Phase 5 becomes in-flight.** Currently captured in roadmap exit criteria for M5.1–M5.4.

**Known cross-cutting requirements for Phase 5:**
- All algorithms operate on `SoundData` and never touch MIDI directly (NFR-3 testability)
- Locked parameters are excluded from mutation, merge, nudge, morph, hill-climb, and constriction — never written
- Per-section enable/disable masks at the UI layer translate to per-parameter masks in the engine

---

# Phase 6 — Polish & Release

**To be detailed when Phase 6 becomes in-flight.** Currently captured in roadmap exit criteria for M6.1–M6.7.

**Known cross-cutting requirements for Phase 6:**
- Release artifacts are reproducible: tagging a commit produces byte-identical Standalone/VST3/AU/(CLAP) binaries for the same source state and toolchain
- macOS releases are notarized; Windows releases are code-signed
- AGPL-3.0 source bundle accompanies every binary release
