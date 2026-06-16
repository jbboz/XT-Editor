# gearmulator (dsp56300)

**Source:** https://github.com/dsp56300/gearmulator
**Pinned commit:** `26cec5578dc0d674e8f7b4aa6a6b325cc81c0d56` (2026-05-26 — "remove reference to obsolete json")
**License:** GPL-3.0 (LICENSE.md in repo root)

Low-level emulator collection covering multiple classic synths. The Microwave II/XT emulation in this project is named **Xenia**.

## What we use

| Need | Path in gearmulator | Notes |
|---|---|---|
| Parameter descriptions JSON | `source/xtJucePlugin/parameterDescriptions_xt.json` | **The most important single file.** Resolves D-06. |
| Wave nibble codec (XOR-flip 0x80) — primary | `source/xtLib/xtState.cpp` (around lines 1050–1112) | Spec comment `"signed char s = Wave[n] ^ 0x80"` lives here. |
| Wave nibble codec — ROM-side reference | `source/xtLib/xtRomWaves.cpp` | Same XOR-flip pattern applied to ROM wave data. |
| Xenia skin assets | `source/xtJucePlugin/skins/xtDefault/` | XT orange/black aesthetic. Format has moved to RmlUi (see Notable changes). |
| Xenia plugin entry | `source/xtJucePlugin/PluginProcessor.{cpp,h}`, `PluginEditorState.{cpp,h}` | Reference for how Xenia wires the device library to the plugin shell. |
| PatchDB | `source/jucePluginLib/patchdb/` | Lives in shared lib, not in xtJucePlugin. D-01 must trace its dep closure. |
| Patch Manager (core) | `source/jucePluginEditorLib/patchmanager/` | Original implementation. |
| Patch Manager (RmlUi) | `source/jucePluginEditorLib/patchmanagerUiRml/` | Newer RmlUi-based UI; coexists with the original. |
| Device library | `source/xtLib/` | xtDevice/xtMidi/xtState etc. Heavy; we don't link this, we only consult it. |

## Notable changes since the dev plan was written

The dev plan (drafted earlier) assumed certain layouts and a JSON-based skin system. The current gearmulator HEAD differs in two material ways — **both feed into D-01 (the decoupling spike):**

1. **Skin system migrated to RmlUi.** The "JSON-driven skin architecture (control positions, bitmap knob filmstrips, panel backgrounds)" the dev plan describes has been replaced by RML/RCSS (HTML/CSS-like) rendered via the in-tree `source/juceRmlUi/` framework. Xenia's `skins/xtDefault/` is RML now. **Implication:** copying the skin system means taking `juceRmlUi` too, or porting back to a JSON model, or reimplementing. M0.2 must classify this.

2. **Patch Manager has two implementations.** `patchmanager/` (original) and `patchmanagerUiRml/` (newer). Both depend on `jucePluginLib/patchdb/`. **Implication:** picking one is itself a sub-decision inside D-01.

3. **No standalone "MIDI Learn overlay" directory.** The dev plan references `source/jucePlugin/` for the MIDI Learn overlay. In current HEAD it's woven into `jucePluginLib`/`jucePluginEditorLib` rather than a discrete unit. Confirm during M0.2 whether it remains separable.

## License obligations

GPL-3.0. AGPL-3.0 final project incorporating GPL-3.0 source is permitted (GPLv3 §13). Per-file attribution required in `ATTRIBUTIONS.md`.
