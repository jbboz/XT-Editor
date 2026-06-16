# gearmulator (dsp56300)

**Source:** https://github.com/dsp56300/gearmulator
**Pinned commit:** `26cec5578dc0d674e8f7b4aa6a6b325cc81c0d56` (2026-05-26 — "remove reference to obsolete json")
**License:** GPL-3.0 (LICENSE.md in repo root)

Low-level emulator collection covering multiple classic synths. The Microwave II/XT emulation in this project is named **Xenia**.

## What we use

| Need | Path in gearmulator | Notes |
|---|---|---|
| Parameter descriptions JSON | `source/xtJucePlugin/parameterDescriptions_xt.json` | **The most important single file.** 2227 lines, JSONC format (`//` comments). 451 entries covering 229 unique indices in 0–255 + 50 named value-to-text mappings (`valuelists`). Resolves D-06. See M0.3 findings below. |
| Wave nibble codec (XOR-flip 0x80) — primary | `source/xtLib/xtState.cpp` (around lines 1050–1112) | Spec comment `"signed char s = Wave[n] ^ 0x80"` lives here. |
| Wave nibble codec — ROM-side reference | `source/xtLib/xtRomWaves.cpp` | Same XOR-flip pattern applied to ROM wave data. |
| Xenia skin assets | `source/xtJucePlugin/skins/xtDefault/` | XT orange/black aesthetic. Format has moved to RmlUi (see Notable changes). |
| Xenia plugin entry | `source/xtJucePlugin/PluginProcessor.{cpp,h}`, `PluginEditorState.{cpp,h}` | Reference for how Xenia wires the device library to the plugin shell. |
| PatchDB | `source/jucePluginLib/patchdb/` | Lives in shared lib, not in xtJucePlugin. D-01 must trace its dep closure. |
| Patch Manager (core) | `source/jucePluginEditorLib/patchmanager/` | Original implementation. |
| Patch Manager (RmlUi) | `source/jucePluginEditorLib/patchmanagerUiRml/` | Newer RmlUi-based UI; coexists with the original. |
| Device library | `source/xtLib/` | xtDevice/xtMidi/xtState etc. Heavy; we don't link this, we only consult it. |

## M0.3 findings — parameterDescriptions_xt.json

- **Format:** JSONC (JSON-with-comments). Use a tolerant parser (e.g. `nlohmann/json` with the right options, or strip `//` and `/* */` before parsing) at build time / load time. The file is 2227 lines.
- **Top-level keys:** `parameterdescriptiondefaults`, `parameterdescriptions`, `regions`, `valuelists`, `midipackets`, `controllerMap`. Much richer than the dev plan assumed — covers parameter metadata, named value lists, MIDI packet shapes, and the CC mapping.
- **Unified parameter table:** `parameterdescriptions` is a flat array of 451 entries covering 229 unique indices spanning 0–255. The same numeric index appears multiple times with different `page` values — this disambiguates SDATA (sound) vs MDATA (multi) vs IDATA (instrument) usage of the same index byte. Names follow a naming convention: `O1Octave` (sound), `MControlZ` (multi), `MI0Pan` (multi instrument 0). This is **better than the dev plan assumed** — one JSON drives metadata for all three data contexts.
- **Coverage:** 27 SDATA indices are deliberately omitted; all 27 match Waldorf §3.1 "reserved" slots exactly. No real gaps.
- **`valuelists`:** 50 named value-to-text mappings. Frequently used: `octaves`, `signed`, `offOn`, `pbrange`, `ascii`, `waveType`, `filter1Type`, `modSource`, `modDest`, `keytrack77`, `keytrack128`, `pan`, `panMod`, `midiNote`.

### Notable divergence: Filter 1 Type has 13 values, not 10

Waldorf §3.15 enumerates filter types 0–9 (10 types). The JSON's `filter1Type` value list has 13:

| Idx | JSON label | In Waldorf §3.15? |
|---|---|---|
| 0–9 | matches Waldorf exactly (24 dB LP through 12 dB LP S&H) | ✓ |
| 10 | "24 dB Notch" | ✗ — JSON only |
| 11 | "12 dB Notch" | ✗ — JSON only |
| 12 | "12 dB Band Stop" | ✗ — JSON only |

Two possibilities, both worth confirming during M1.5 on real hardware:
- Late firmware additions (post-2.16 spec) — real XT supports them
- Xenia-emulator extensions — Xenia accepts them; real XT clamps or behaves undefined

If they're emulator-only, our editor should clamp `F1Type` to 0–9 for real hardware and 0–12 for Xenia. Decision deferred to M1.5 hardware testing.

## Notable changes since the dev plan was written

The dev plan (drafted earlier) assumed certain layouts and a JSON-based skin system. The current gearmulator HEAD differs in two material ways — **both feed into D-01 (the decoupling spike):**

1. **Skin system migrated to RmlUi.** The "JSON-driven skin architecture (control positions, bitmap knob filmstrips, panel backgrounds)" the dev plan describes has been replaced by RML/RCSS (HTML/CSS-like) rendered via the in-tree `source/juceRmlUi/` framework. Xenia's `skins/xtDefault/` is RML now. **Implication:** copying the skin system means taking `juceRmlUi` too, or porting back to a JSON model, or reimplementing. M0.2 must classify this.

2. **Patch Manager has two implementations.** `patchmanager/` (original) and `patchmanagerUiRml/` (newer). Both depend on `jucePluginLib/patchdb/`. **Implication:** picking one is itself a sub-decision inside D-01.

3. **No standalone "MIDI Learn overlay" directory.** The dev plan references `source/jucePlugin/` for the MIDI Learn overlay. In current HEAD it's woven into `jucePluginLib`/`jucePluginEditorLib` rather than a discrete unit. Confirm during M0.2 whether it remains separable.

## License obligations

GPL-3.0. AGPL-3.0 final project incorporating GPL-3.0 source is permitted (GPLv3 §13). Per-file attribution required in `ATTRIBUTIONS.md`.
