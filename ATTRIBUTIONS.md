# Attributions

Per-file provenance for code ported or copied into this project. This file is the running record of license obligations and source-strategy decisions.

This document is a **draft** as of 2026-06-16 — populated with the M0.2 decoupling-spike classifications. It will be expanded with per-file attribution headers as actual code is copied or ported (M1.1 onwards).

---

## License of the combined work

**AGPL-3.0.** The combined work satisfies the obligations of all upstream licenses simultaneously:

| Upstream | License | Compatible with AGPL-3.0? |
|---|---|---|
| JUCE | AGPLv3 option | ✓ (we use the AGPLv3 option) |
| gearmulator (dsp56300) | GPL-3.0 | ✓ (GPLv3 §13 explicitly permits combination with AGPLv3) |
| MidiKraft-librarian (christofmuc) | AGPL-3.0 | ✓ (same license) |
| Edisyn (eclab) | Apache-2.0 | ✓ (one-way compatible into (A)GPLv3) |
| mwsd (jeanette-c) | GPL-3.0 | ✓ |

Per-file attribution headers required:
- **Apache-2.0** ported logic — every C++ file containing logic derived from Edisyn must carry the Apache-2.0 attribution header (original copyright, NOTICE-style mention).
- **GPL-3.0** and **AGPL-3.0** copied/ported code — per-file note in this document, plus a comment in the file recording the upstream source path and commit hash.

---

## Source-strategy decisions (D-01 + D-05 from ROADMAP)

The M0.2 decoupling spike traced each borrowed component's dependency closure. Results below revise the dev plan §2's assumptions.

### gearmulator components

| Component | Classification | Rationale |
|---|---|---|
| `xtJucePlugin/parameterDescriptions_xt.json` | **Copy clean** | Pure data file; ~400 lines JSON. No transitive deps. |
| Wave/wavetable codec (`xtLib/xtState.cpp` snippets) | **Copy clean** | Self-contained codec functions (XOR-flip + nibble pack/unpack); ~100 lines. |
| `jucePluginLib/patchdb/` (SQLite layer) | **Copy + small shim** | Pulls in `baseLib/{binarystream,filesystem,hybridcontainer}.h`, `dsp56kBase/{logging,threadtools}.h`, `synthLib/{midiToSysex,midiTypes}.h`. Shim out `dsp56kBase` (logging/threading via std/JUCE equivalents). Take `baseLib` and `synthLib` subsets. |
| `jucePluginEditorLib/patchmanager/` + `patchmanagerUiRml/` | **Reimplement** | The two are mutually dependent (original `patchmanager.cpp` `#include`s `patchmanagerUiRml/patchmanagerUiRml.h`, and vice versa). Both require `juceRmlUi` + the upstream `RmlUi` library (with Lua scripting + custom Metal/OpenGL renderers) + `juceUiLib` + the full `jucePluginEditorLib`/`jucePluginLib`. Total closure is essentially "fork most of gearmulator." Build our own JUCE-native browser UI on top of the borrowed `patchdb` core. |
| Skin system (`juceRmlUi/` + `xtJucePlugin/skins/xtDefault/`) | **Reimplement (use Xenia PNG assets directly)** | The RmlUi-based skin engine requires RmlUi (~40 files) + Lua VM + custom Metal/OpenGL renderers + sse2neon for ARM macOS. Cross-platform but heavy. The Xenia skin's PNG assets (background, knob filmstrips, page icons, filter-type glyphs) are reusable directly under GPL-3.0. Bind them via native JUCE LookAndFeel + custom `Component` subclasses. The `xtDefault.json` layout file is small and structured enough to parse for layout coordinates if desired. |
| MIDI Learn core (`jucePluginLib/midiLearnMapping.*`, `midiLearnManager.*`, `midiLearnTranslator.*`, `midiLearnPreset.*`) | **Copy clean** | Pure logic; depends on `baseLib/binarystream`, jucePluginLib's `controller`/`controllermap`/`parameter`, `synthLib/midiTypes`, and JUCE core/data_structures. Most of these are taken anyway. |
| MIDI Learn UI (`jucePluginEditorLib/settingsMidiLearn.*`, `parameterOverlay*.*`) | **Reimplement** | Tightly RmlUi-coupled (`juceRmlUi/rmlElemButton`, `rmlElemComboBox`, `rmlEventListener`, `rmlHelper`, `rmlInplaceEditor`, plus `juceRmlPlugin/rmlPlugin.h`, `RmlUi/Core/*`). Build our own JUCE-native overlay using the borrowed MIDI Learn core. |
| MIDI Learn overlay alternative (per dev plan §2.1, "from `source/jucePlugin/`") | **N/A** | The dev plan referenced `source/jucePlugin/` for the MIDI Learn overlay. That directory no longer exists in current HEAD — the functionality migrated into `jucePluginLib` (core) + `jucePluginEditorLib` (RmlUi-coupled UI). See classifications above. |

### MidiKraft-librarian (resolves D-05)

**Verdict: not feasible as a standalone module.**

The repo is archived (merged into the consolidated MidiKraft tree at https://github.com/christofmuc/MidiKraft). Even before archiving it required `juce-utils`, `midikraft-base`, `nlohmann_json`, `fmt` as link dependencies and ~9 abstract capability interfaces from `midikraft-base` to be implemented against our hardware.

**Path forward:** write our own focused SNDR→SNDD state machine for M2.3. ~200–400 lines of JUCE-aware C++. Reference `Librarian.cpp` for patterns (timeout/retry, handler stack, partial-message handling) without linking. See [`references/MidiKraft-librarian.md`](references/MidiKraft-librarian.md) for the full analysis.

### Other upstream sources (unchanged from dev plan)

| Source | Status | Notes |
|---|---|---|
| Edisyn (`WaldorfMicrowaveXT.java`, `Synth.java`) | Port logic to C++ | Apache-2.0 attribution headers per derived file (M1.2 clamping, M5.1 mutation weights, M5.2 algorithms) |
| mwsd (`synth_info.cpp`, `curses_mw_ui.cpp`) | Port logic to C++ | GPL-3.0; M1.3 autodetect + DISD parser; M3.6 LCD mirror UI |
| KnobKraft `juce-widgets` | Selectively copy widgets | M2.2 patch name label, color tag system, search bar — *now in question pending whether to keep KnobKraft deps at all given D-05; revisit in M2.2 planning* |

---

## Roadmap impacts

The spike's findings shift several milestone scopes (recorded in ROADMAP):

| Milestone | Was | Now |
|---|---|---|
| **M1.1** (Project scaffold) | Skin system "copied from gearmulator" | Our own JUCE-native skin layer that consumes Xenia PNG assets. Effort M→L. |
| **M2.1** (PatchDB wired) | "Copied from gearmulator" | Copy patchdb/* + shim baseLib/dsp56kBase utilities. Effort stays M but with explicit shim work. |
| **M2.2** (Browser panel) | "from gearmulator patch-manager" | Reimplement JUCE-native browser UI on borrowed patchdb. Effort L→XL; split into sub-milestones. |
| **M2.3** (Bulk receive/send) | "via MidiKraft-librarian" | Our own SNDR→SNDD state machine. Effort stays M (state machine is bounded). |
| **M6.1** (MIDI Learn overlay) | "Port from gearmulator 2.2.2" | Copy MIDI Learn *core* from current gearmulator; reimplement the *UI overlay* in JUCE-native. Effort stays M. |

---

## Per-file attribution log

Format for each entry:

```
### <our path>
- Source: <upstream repo>:<commit>:<upstream path>
- License: <license>
- Modifications: <summary of edits>
```

### source/mw2xtUI/skins/xtDefault/*.png, Digital.ttf, xtDefault.json
- **Source:** dsp56300/gearmulator @ `26cec55` : `source/xtJucePlugin/skins/xtDefault/`
- **License:** GPL-3.0 (Xenia skin assets, "XT orange-and-black" aesthetic)
- **Modifications:** None — files copied verbatim. The upstream `xtDefault.rml`, `xtDefault.rcss`, `xt-waveeditor-defaults.rcss`, and `assets.cmake` are intentionally **not** copied — those are gearmulator's RmlUi-specific bindings; we reimplement the skin engine natively in JUCE per D-01 (see [`ROADMAP.md`](ROADMAP.md)).
- **Files included (36 total, ~12 MB):**
  - `xtDefaultBG.png` — main panel background (6.6 MB)
  - `xt_encoder_ranged.png`, `xt_encoder_ranged_red.png`, `_page0.png` variants — knob filmstrips
  - `xtknob.png`, `xtknob_small.png`, `xtknob_minus.png`, `xtknob_plus.png` — additional knob bitmaps
  - `alphadial.png` — alpha-channel encoder bitmap
  - `xtPageOsc.png`, `xtPageWave.png`, `xtPageMod.png`, `xtPageArp.png`, `xtPageMulti.png`, `xtPagePatchManager.png` — page-button icons
  - `f1Type6.png`, `f1Type7.png`, `f1Type8.png`, `f1Type9.png`, `f1Type12.png`, `f1TypeUnused.png` — filter-type glyphs
  - `fxAmpMod.png`, `fxAutoWahLP.png`, `fxChorus.png`, `fxDelay.png`, `fxFlanger2.png`, `fxModDelay.png`, `fxOff.png`, `fxOverdrive.png` — effect-type glyphs
  - `led.png`, `btWaveEditor.png` — UI element bitmaps
  - `Digital.ttf` — LCD/display font
  - `xtDefault.json` — gearmulator's JSON layout file, retained as a reference for layout intent
