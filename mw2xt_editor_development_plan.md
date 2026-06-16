# Waldorf Microwave II/XT Editor — Development Plan

## C++/JUCE · AGPL-3.0 · Standalone \+ Plugin

---

## 1\. Project Orientation

### What you are building

A full-featured MIDI/SysEx parameter editor and patch librarian for the Waldorf Microwave II/XT/XTk. It targets real hardware over MIDI, ships as a cross-platform JUCE application in both standalone and VST3/AU/CLAP plugin formats, and is released under AGPL-3.0.

The reference product is **XplorerMac** (and its successors monstrumWaveXT / mwaveXL), but with a modern technology base, robust library management, and features those tools entirely lack: a visual modulation matrix, wavetable editor, and a full suite of patch exploration tools (randomization, mutation, merge, morph, hill-climbing).

The visual identity is the XT itself — dark chassis, orange accent colour, hardware-faithful section grouping — while the interaction model is modernised wherever the hardware's physical constraints created usability problems.

### What this is NOT

This editor does not embed the Xenia DSP emulation engine. The hardware synth (or Xenia over a MIDI loopback) is the sound source. The editor sends and receives SysEx/CC over a normal MIDI port. You do not need the ROM, the emulation cores, or the DSP56300 emulator at runtime — you borrow source-code infrastructure only.

### License

AGPL-3.0 for the combined work. All source published on GitHub satisfies the obligations of all component licences simultaneously:

- gearmulator components: GPL-3.0 ✓  
- KnobKraft/MidiKraft components: AGPL-3.0 ✓  
- Edisyn (ported logic): Apache 2.0 ✓ (compatible; attribution required in source comments)  
- mwsd (ported logic): GPL-3.0 ✓  
- JUCE: AGPLv3 option ✓

---

## 2\. Source Strategy

This project is a **fresh JUCE CMake project** — not a fork of any upstream repo. It pulls in well-defined components from four sources as submodules or copied files. This avoids inheriting gearmulator's emulation-layer build complexity and keeps the codebase coherent and purposeful.

### 2.1 From gearmulator — copy specific files, do not fork

**`parameterDescriptions_*.json`** — the complete MW2/XT parameter definition file mapping every SNDP/MULP/GLBP index to its name, range, and value-to-text enumeration. Tested resolution of spec ambiguities. Copy directly into `source/mw2xtLib/data/`. The most important single file in the project.

**Wave and wavetable nibble codec** — encode/decode for WDATA (§3.4) and WCTDATA (§3.5). The XOR-flip signed format (`signed char s = Wave[n] ^ 0x80`) is correctness-critical and already correct in gearmulator. Copy verbatim from `source/mw2xt/` or `source/xeniaJucePlugin/`.

**PatchDB \+ Patch Manager UI** — `source/synthLib/patchDB/` and `source/patchManager/`. SQLite-backed library with full-text search, tags, favourites, grid/list views, drag-and-drop, .syx/.mid import/export. The most effort-intensive subsystem to build from scratch. Strip the ROM-loading path; everything else is device-agnostic.

**Skin system** — gearmulator's JSON-driven skin architecture (control positions, bitmap knob filmstrips, panel backgrounds) is cleanly separable from the parameter binding model. Take this system. Xenia's existing skin assets — the orange-and-black XT aesthetic — can be used directly as your default skin under GPL-3.0. The community already building Xenia skins becomes your skin community.

**MIDI Learn overlay** — `source/jucePlugin/`: right-click parameter-to-CC assignment overlay added in gearmulator 2.2.2. Essential because the XT's 44 hardware knobs transmit CC.

**SNDP rate limiter pattern** — gearmulator's BUG-10134 fix: per-parameter 100ms coalescing queue. Without this, fast DAW automation or UI interaction will overwhelm the XT firmware.

### 2.2 From Edisyn — the most important reference in the ecosystem

Edisyn (`github.com/eclab/edisyn`, Apache 2.0) has an existing, working, hardware-tested MW2/XT patch editor written by an author who personally owns the XT. It is Java and cannot be linked against, but it is the single most reliable external reference — more reliable than the spec document in edge cases.

**`WaldorfMicrowaveXT.java`** is a verified executable version of the sysex spec. Use it to verify every message frame before hardware testing, port the per-parameter mutation weight table (weeks of hardware testing already done), and resolve spec ambiguities. Edisyn's hardware-tested value clamping is authoritative over the raw spec.

**`Synth.java`** contains the best open-source patch exploration algorithms available. Port all of them into `ExplorationEngine` (Apache 2.0, credit in source comments):

- **Mutation** — weighted random perturbation within locked/unlocked constraints  
- **Merge** — two patches recombined, each parameter independently drawn from A or B  
- **Nudge** — current patch steered toward one of four target patches  
- **Morph** — real-time 2D XY crossfade between four patches, sends SNDP continuously  
- **Hill-climbing** — interactive evolutionary algorithm; user grades candidates, engine converges. Academically validated. No equivalent in any C++ synth editor.  
- **Constriction** — progressive narrowing of mutation range around a converged result

**Keep Edisyn source in a `references/` directory** — clearly marked as not compiled, one directory away when debugging hardware communication.

### 2.3 From KnobKraft/MidiKraft — use as submodule

**`MidiKraft-librarian`** (AGPL-3.0) — the correct solution for the SNDR → SNDD handshake problem. Provides: request/response state machine with configurable timeouts, retry logic, sequential bank-by-bank bulk download (the 256-patch All Sounds dump can take 30+ seconds on real hardware), interleaved message handling, and progress callbacks for JUCE progress bars. Do not write a custom handshake state machine.

**`juce-widgets`** — selectively copy the patch name display label, colour tag system, and search bar.

### 2.4 From mwsd — port specific logic

**Universal Device Inquiry autodetect** — broadcasts `F0 7E 7F 06 01 F7`, parses the device family member code response to identify the XT and its device ID automatically. Users never configure a device ID manually.

**DISD parser** — parses 80-character LCD content and LED bitmask from DISD (§2.62). Enables a real-time LCD mirror panel.

---

## 3\. UI Design

### Philosophy

The XT's physical front panel has a deliberate functional logic that users already know: OSC section, WAVE section, FILTER section, AMP section, ENV/LFO in pages, ARP in pages, modulation matrix across 16 numbered slots. The editor respects that mental model while eliminating every limitation imposed by the hardware's physical constraints.

**Mirror the hardware's section grouping. Do not mirror the hardware's page system.** The LCD can only show one parameter at a time, forcing page navigation. The editor's purpose is to show everything in a section simultaneously, eliminating the page-flipping that makes the XT time-consuming to program.

### Visual Identity

The XT's aesthetic language is the default skin: near-black background, orange accent colour for active elements and value indicators, dark grey knobs with orange indicator lines, monospaced display font for the LCD mirror and parameter values. This is directly achievable using Xenia's skin assets under GPL-3.0.

The default window is sized to show the main editor without scrolling. The browser and exploration panels open as resizable side panels or secondary windows, keeping the core editor uncluttered.

### Layout Structure

```
┌─────────────────────────────────────────────────────────────────┐
│  PATCH NAME  [A001]  [RECV] [SEND] [COMPARE] [STORE]  [DEVICE] │  ← toolbar
├────────────────────────────────────────────────────────────────┤
│  OSC │ WAVE │ FILTER │ AMP │ ENV │ LFO │ ARP │ MOD │ MULTI │ GLOBAL │  ← tabs
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                    [ main editor panel ]                        │  ← tab content
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│  [BROWSER ▸]   [EXPLORE ▸]   [LCD ▸]      undo · redo · dirty  │  ← status bar
└─────────────────────────────────────────────────────────────────┘
```

BROWSER, EXPLORE, and LCD panels slide in from the bottom or right, overlaying nothing — the main editor remains visible and interactive when they are open.

### Tab-by-Tab Design

**OSC tab** Both oscillators side by side, matching the hardware's physical left-right layout. Octave, semitone, detune, keytrack, pitchbend scale, FM amount for each. Osc 2 sync and link toggles. No page navigation required — every OSC parameter visible at once.

**WAVE tab** Wavetable selector with waveform preview strip (shows the current wave shape inline — impossible on hardware). Wave 1 and Wave 2 controls (startwave, phase, env amount, velo amount, keytrack, limit, link) in two columns. Mix level section: Wave1, Wave2, RingMod, Noise, External — shown as both sliders and a proportional visual mixer bar. The wave preview updates as startwave changes, giving visual feedback the hardware cannot provide.

**FILTER tab** Filter 1 full (cutoff, resonance, type dropdown, keytrack, env amount, env velocity, special param — the special param label updates based on selected filter type, matching what the hardware display shows). Filter 2 below (cutoff, type, keytrack). Filter type selector shows the actual filter names (24dB LP, 12dB BP, Waveshaper, etc.) rather than numbers.

**AMP tab** Volume, keytrack, env velocity, panning with keytrack. Chorus on/off. Glide section: on/off, type (portamento/glissando/fingered variants as labelled options), mode (exp/linear), time. All on one screen.

**ENV tab** Three envelopes displayed simultaneously — the hardware shows one at a time through paging. Filter Env (ADSR \+ trigger mode), Amp Env (ADSR \+ trigger mode), Free Env (4-stage \+ release, trigger mode) each with a graphical ADSR display that updates as values change. Wave Env sub-section: 8-stage envelope with key-on and key-off loop controls. Loop range shown visually on the envelope display.

**LFO tab** Both LFOs side by side. Rate, shape (waveform icon for each shape option), delay, sync, symmetry, humanize. LFO2 phase control. Shape selector uses waveform icons rather than text labels.

**ARP tab** All arpeggiator parameters on one screen. The 16-step user pattern — buried in sub-pages on hardware — shown as a click-to-toggle 16-step grid, immediately readable and editable. Tempo with external/BPM display. Clock division as a readable fraction (1/4, 1/8, etc.).

**MOD tab** The single biggest usability improvement over the hardware. The XT's 16 modulation slots require navigating slot-by-slot with no overview. This tab presents a **2D grid**: 31 modulation sources (rows, from §3.12) × 36 destinations (columns, from §3.13). Occupied cells show their amount value. Empty cells are click-to-add. The current modulation routing is comprehensible at a glance for the first time. Modifier slots (§3.14) in a compact sub-panel below the grid: source1, source2, operation, parameter for each of the 4 modifiers. Modifier delay source and time alongside.

**MULTI tab** 8-instrument strip layout, one row per instrument. Bank/sound selector, channel, volume, pan, transpose, detune, output, status (on/off), key range, velocity range. Arpeggiator per-instrument settings in a collapsible section. MDATA (multi volume, name, W/X/Y/Z controllers) in a header row above the strips.

**GLOBAL tab** All GDATA parameters: MIDI channel, device ID (with re-scan button), bend range, controllers W/X/Y/Z, master volume, transpose, master tune, startup mode, parameter send/receive mode, arp output channel, MIDI clock output.

### Improvements Over the Hardware

| Hardware limitation | Editor solution |
| :---- | :---- |
| LCD shows one parameter at a time | All section parameters visible simultaneously |
| Modulation matrix requires slot-by-slot navigation, no overview | 2D source×destination grid with instant comprehension |
| Wavetable position shows a number, no visual | Inline waveform preview on WAVE tab; 3D waterfall in wavetable editor |
| Arp pattern buried in sub-pages | 16-step click grid directly on ARP tab |
| Name editor: scroll through ASCII characters | Direct text input field |
| Filter type shows a number | Named type selector (24dB LP, Waveshaper, etc.) |
| Shift+button features require physical access | RMTP sysex simulates any button press from the editor |
| No randomization or exploration | Full exploration engine: mutation, merge, morph, hill-climbing |
| Envelope shapes not visible | Graphical ADSR display per envelope, live updating |
| LFO shapes shown as numbers | Waveform icon selectors |

### Exploration Panel

Opens as a slide-in overlay. Contains four sub-panels selectable by tab:

**Mutate** — mutation amount slider, per-section enable/disable checkboxes, per-parameter lock buttons (visible as padlock icons on the main editor knobs), Randomize and Mutate buttons.

**Merge/Nudge** — patch A (current) and patch B (loaded from library) side by side with a weighted blend slider. Nudge target slots (up to 4 patches from library).

**Morph** — 2D XY pad with four corner patches assigned from the library. Moving the crosshair interpolates and sends SNDP in real time. MIDI CC assignable to X and Y axes.

**Hill-Climber** — candidate patch grid (3×3 or 4×4). Each cell plays the candidate when clicked and shows a grade button. Controls for mutation rate and recombination method. Brief inline guide: "1. Load a starting patch. 2\. Grade each candidate. 3\. Click Evolve. Repeat." The panel remains open while the main editor shows whichever candidate was last selected.

### Skin System

Built on gearmulator's JSON skin architecture. The default skin ships with the project and uses Xenia's existing XT-aesthetic assets. A skin consists of:

- `skin.json` — control positions, sizes, colours  
- Knob filmstrip PNG — 100 frames, dark grey with orange indicator  
- Panel background PNG — near-black with subtle texture  
- Font specification

Community skins drop into a `skins/` folder alongside the binary. The skin selector is in the settings screen.

---

## 4\. Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                      JUCE Plugin Shell                        │
│     AudioProcessor · AudioProcessorEditor · MIDI I/O         │
└──────────────────────────┬───────────────────────────────────┘
                           │
          ┌────────────────▼──────────────────┐
          │          EditorController          │
          │  patch state · undo · CC routing  │
          └───┬──────────┬────────────────────┘
              │          │
   ┌──────────▼───┐  ┌───▼──────────────────────────────────┐
   │  PatchModel  │  │         HardwareMidiDevice            │
   │  SDATA[256]  │  │  MidiKraft-librarian handshake layer  │
   │  MDATA[32]   │  │  sendSNDP() · sendSNDD()             │
   │  8xIDATA[28] │  │  requestSNDR() · requestGLBR()       │
   │  GDATA[32]   │  │  autodetect() · parseDISD()          │
   └──────────────┘  │  SNDP rate limiter (100ms/param)     │
          │          └───────────────────────────────────────┘
          │                        │ juce::MidiOutput/Input
   ┌──────▼────────────────────────▼──────────┐
   │            PatchDB (SQLite)               │  from gearmulator
   │    import · export · search · tags        │
   └──────────────────────────────────────────┘
          │
   ┌──────▼────────────────────────────────────────────────────┐
   │                    EditorUI (JUCE)                         │
   │   Tabbed pages (skin system from gearmulator/Xenia)       │
   │   Mod matrix grid · Wavetable editor · ADSR displays      │
   │   Exploration panel (morph · hill-climb · merge · mutate) │
   │   Browser panel · LCD mirror panel                        │
   └────────────────────────────────────────────────────────────┘
```

### PatchModel

- `SoundData`: `std::array<uint8_t, 256>` matching SDATA §3.1  
- `MultiData`: `std::array<uint8_t, 32>` (MDATA §3.2) \+ `std::array<InstrumentData, 8>` (IDATA §3.3)  
- `GlobalData`: `std::array<uint8_t, 32>` (GDATA §3.6)

Typed accessors mirror SNDP HH/PP indices. JSON parameter descriptions from gearmulator provide display metadata. Value clamping follows Edisyn's hardware-tested ranges.

### HardwareMidiDevice

Wraps `juce::MidiOutput` / `juce::MidiInput` and MidiKraft-librarian:

- Universal Device Inquiry autodetect (from mwsd)  
- All message types: SNDP, SNDD, SNDR, MULP, MULD, MULR, GLBP, GLBD, GLBR, WAVD, WAVR, WCTD, WCTR, RMTP, MODR  
- Per-parameter SNDP rate limiting (100ms coalescing queue)  
- Incoming SysEx routing to registered callbacks  
- DISD parser for LCD mirror

### ExplorationEngine

Self-contained, MIDI-decoupled. Takes `SoundData` input(s), returns `SoundData` output. Contains mutation weights (from Edisyn `WaldorfMicrowaveXT.java`), parameter lock mask, and all six exploration algorithms from Edisyn `Synth.java`.

### EditorController

MVC layer owning: `juce::UndoManager`, dirty-state, A/B compare buffers, `ExplorationEngine` instance, CC receive routing, sound/multi mode context.

---

## 5\. Feature Plan by Phase

### Phase 1 — Core Editor (MVP)

**Goal:** Working single-patch editor communicating reliably with real hardware, with hardware-faithful UI and the skin system in place from the start.

**Setup:**

- Fresh JUCE CMake project. Submodules: JUCE, sqlite\_orm, MidiKraft-librarian, juce-widgets. Copied files: gearmulator PatchDB \+ patch manager UI \+ wave codec \+ parameterDescriptions JSON \+ skin system. References directory: Edisyn sources, mwsd sources.  
- Builds on macOS, Windows, Linux as Standalone \+ VST3 \+ AU \+ CLAP.  
- AGPL-3.0 LICENSE, ATTRIBUTIONS.md.  
- Default skin: XT orange-and-black using Xenia skin assets.

**Hardware communication:**

- `HardwareMidiDevice` with Universal Device Inquiry autodetect.  
- SNDP send with rate limiter. SNDR → SNDD via MidiKraft-librarian.  
- Incoming CC from hardware knobs updates model and UI in real time.

**Parameter editor — all tabs:**

- OSC, WAVE (with inline waveform preview), FILTER (named type selector), AMP, ENV (graphical ADSR displays, all three envelopes), LFO (waveform icon selectors), ARP (16-step grid), MISC, NAME (direct text input), GLOBAL, MULTI.  
- All controls send SNDP immediately. All controls update on incoming CC/SysEx from hardware.  
- Undo/redo on every parameter change.  
- Padlock icons on all knobs (inactive until Exploration panel is built in Phase 5).

### Phase 2 — Patch Library

**Goal:** Database-backed patch library with hardware bulk transfer.

- PatchDB from gearmulator: SQLite, data-source management, auto-scan.  
- Browser panel: grid \+ list views, full-text search, tag browser, three-state rating.  
- Drag patch to hardware edit buffer. Drag to bank slot.  
- Bulk receive (all 256 patches) via MidiKraft-librarian with progress bar.  
- Bulk send. File export as .syx / .mid.  
- A/B compare: load from DB into compare buffer, highlight changed parameters.  
- Secondary MIDI device routing.

### Phase 3 — Wavetable & Waveform Editor

**Goal:** Draw, import, and upload custom waves and wavetables.

- **Waveform editor**: 64-sample canvas, pencil and line tools, harmonic editor, audio file import.  
- **Wavetable editor**: 64-entry control table, drag wave indices to positions, spectral interpolation preview.  
- **Wave visualizer**: 3D waterfall or stacked 2D display, updates as startwave knob moves.  
- **ROM wave browser**: 300 ROM waves read-only.  
- WAVR/WAVD user wave upload/download (indices 1000–1249).  
- WCTR/WCTD user wavetable upload/download (indices 96–128).  
- **LCD mirror panel**: real-time XT display via DISD (from mwsd parser).

### Phase 4 — Modulation Matrix

**Goal:** A visual grid replacing the hardware's slot-by-slot matrix.

- 2D grid: 31 sources (§3.12) × 36 destinations (§3.13). Occupied cells show amount.  
- 16 mod slots mapped onto grid. Multiple slots per cell shown stacked.  
- Click to add/remove. Drag to set amount. Double-click to inspect stacked slots.  
- Modifier slots panel: source1, source2, operation (16 types), parameter. Modifier delay section.  
- All changes send SNDP immediately.

### Phase 5 — Patch Exploration Engine

**Goal:** The feature that defines this editor against all existing MW2/XT tools.

All operations: push to undo → compute → send SNDP → update UI. User hears result on hardware immediately.

**Mutate**: Edisyn mutation weights, per-section checkboxes, per-parameter padlock buttons (activated from Phase 1 UI), 0–100% mutation amount, random mod matrix option.

**Merge**: current \+ library patch, parameter-by-parameter weighted blend.

**Nudge**: current patch steered toward up to four library targets.

**Morph**: 2D XY pad, four corner patches from library, real-time SNDP, MIDI CC assignable to axes.

**Hill-Climbing**: candidate grid, better/worse grading, evolutionary convergence. Inline guide ("1. Start patch. 2\. Grade. 3\. Evolve. Repeat."). Uses mutation weights and parameter locks.

**Constriction**: progressive narrowing of mutation range for fine-tuning after hill-climbing.

### Phase 6 — Polish & Extended Features

- **MIDI Learn overlay**: right-click → assign CC. All 44 XT hardware knobs become live editor controls.  
- **Parameter automation**: expose SDATA params as VST/AU/CLAP automation parameters through the rate limiter.  
- **Program change receive**: browser highlights and optionally loads matching patch.  
- **RMTP remote control**: simulate Store, Compare, Recall from the editor without physical button presses — the key capability Xenia lacks.  
- **Copy/paste**: patch to clipboard as JSON or raw bytes.  
- **Community skin support**: skin selector in settings, skins folder alongside binary.  
- **Xenia compatibility**: optional SysEx routing to Xenia via MIDI loopback.

---

## 6\. SysEx Implementation Reference

| Message | Direction | Use | Notes |
| :---- | :---- | :---- | :---- |
| SNDP (20h) | Send | Real-time parameter edit | No checksum. HH=00h params 0–127, HH=01h params 128–255 |
| SNDR (00h) | Send | Request patch | BB/NN location |
| SNDD (10h) | Receive | Patch dump | 256 bytes SDATA \+ checksum |
| SNDD (10h) | Send | Send to edit buffer | BB=20h NN=00h |
| MULR (01h) | Send | Request multi |  |
| MULD (11h) | Receive/Send | Multi data | MDATA \+ 8x IDATA |
| MULP (21h) | Send | Multi parameter change | IDM is 21h — spec header says 20h, which is wrong |
| GLBR (04h) | Send | Request global params |  |
| GLBD (14h) | Receive/Send | Global parameter block |  |
| GLBP (24h) | Send | Global parameter change | No checksum |
| WAVR (02h) | Send | Request user wave | HH/LL location |
| WAVD (12h) | Receive/Send | Wave data | 128 nibbles, XOR-flip signed format |
| WCTR (03h) | Send | Request wavetable table |  |
| WCTD (13h) | Receive/Send | Wavetable control table | 256 nibbles, 4-nibble index encoding |
| DISR (05h) | Send | Request LCD | For LCD mirror panel |
| DISD (15h) | Receive | LCD \+ LED state | 80 chars \+ LED bitmask |
| RMTP (26h) | Send | Simulate front panel | Enables shift-button features |
| MODR (07h) | Send | Request mode |  |
| MODD (17h) | Receive | Mode status |  |

**Critical notes:**

- SNDP and GLBP omit checksum. Do not add one.  
- MULP IDM is 21h, not 20h. Edisyn confirms this.  
- SNDP HH byte: 00h for indices 0–127, 01h for 128–255. Silent wrong-parameter bugs if incorrect.  
- RMTP enables Store/Compare/Recall without physical button presses.  
- Incoming CC from hardware knobs maps per the controls PDF; also possible as SysEx in SysEx send mode.  
- Device ID: 00h–7Eh plus broadcast 7Fh. Default to autodetected value.

---

## 7\. Repository Structure

```
mw2xt-editor/
│
├── source/
│   ├── mw2xtLib/           ← Protocol layer: SDATA/MDATA/IDATA/GDATA structs,
│   │   │                      typed accessors, sysex framing, checksum, CC
│   │   │                      mapping. Wave/wavetable codec from gearmulator.
│   │   └── data/
│   │       └── parameterDescriptions_mw2xt.json   ← from gearmulator
│   │
│   ├── mw2xtEditor/        ← PatchModel, HardwareMidiDevice (autodetect +
│   │                          DISD parser from mwsd), EditorController,
│   │                          ExplorationEngine (from Edisyn), undo stack,
│   │                          SNDP rate limiter.
│   │
│   ├── mw2xtUI/            ← JUCE Components: tabbed editor pages with skin
│   │                          system (from gearmulator), ADSR displays, mod
│   │                          matrix grid, wavetable editor, waveform editor,
│   │                          morph XY pad, hill-climber panel, LCD mirror
│   │                          panel, MIDI Learn overlay (from gearmulator),
│   │                          patch name editor.
│   │   └── skins/
│   │       └── default/    ← XT orange-and-black skin (Xenia assets, GPL-3.0)
│   │           ├── skin.json
│   │           ├── knob_filmstrip.png
│   │           └── panel_bg.png
│   │
│   ├── mw2xtPlugin/        ← JUCE AudioProcessor/AudioProcessorEditor shell,
│   │                          CMakeLists, plugin metadata, automation bridge.
│   │
│   └── patchManager/       ← Copied from gearmulator: PatchDB SQLite layer,
│                              BinaryStream, patch manager UI components.
│
├── submodules/
│   ├── JUCE/
│   ├── sqlite_orm/
│   ├── MidiKraft-librarian/    ← christofmuc; AGPL-3.0
│   └── juce-widgets/           ← christofmuc; selective use
│
├── references/                 ← NOT compiled; verified reference source
│   ├── edisyn-waldorf/         ← WaldorfMicrowaveXT.java + Synth.java
│   │   └── README.md           ← Apache 2.0; spec reference + algorithm source
│   └── mwsd/                   ← autodetect + DISD parser reference
│       └── README.md           ← GPL-3.0; autodetect and DISD logic ported
│
├── CMakeLists.txt
├── LICENSE                     ← AGPL-3.0
└── ATTRIBUTIONS.md             ← Per-file attribution for all ported code
```

---

## 8\. Key Risks and Mitigations

**SysEx timing on real hardware.** Use MidiKraft-librarian — do not write a custom state machine. Never block the UI thread on SysEx round trips.

**SNDP rate limiting.** Wire the coalescing queue into `HardwareMidiDevice` from the start. Fast UI or DAW automation without it will overwhelm the XT firmware.

**Parameter index HH byte.** Get this right before hardware testing. HH=01h for SDATA indices 128–255. Silent wrong-parameter bugs otherwise.

**Wave nibble encoding.** Use the gearmulator codec verbatim. Do not rewrite. The XOR-flip signed format is a common source of correctness errors.

**MULP IDM discrepancy.** Use 21h, not 20h. Edisyn confirms. Cross-check before hardware testing.

**Mutation weight accuracy.** Edisyn's weight table encodes hardware-tested knowledge. Do not substitute generic ranges — some parameters that appear wide in the spec will produce silence or reset the patch if randomized. Use Edisyn's weights as baseline; refine only from hardware testing.

**Skin asset reuse.** Xenia skin assets are GPL-3.0. Using them in an AGPL-3.0 project is compatible, but document the asset origin in ATTRIBUTIONS.md and include a note that the skin assets originated from the gearmulator project.

**Hill-climbing UX.** The algorithm is only productive when users understand it. The inline guide (3–4 steps in the panel) and the parameter lock system (restricting exploration to one section) are essential to useful results. Reference Edisyn's documentation.

**AGPL-3.0 compliance.** Source on GitHub satisfies all obligations. Include Apache 2.0 attribution comments in every file containing Edisyn-derived logic.

---

## 9\. Development Sequence

1. **Project scaffold.** Fresh JUCE CMake project. Add submodules (JUCE, sqlite\_orm, MidiKraft-librarian). Add `references/` directory. Write `ATTRIBUTIONS.md`. Set up skin system from gearmulator with default skin directory.  
     
2. **Protocol layer.** Create `mw2xtLib/`: copy wave/wavetable codec from gearmulator, copy parameterDescriptions JSON, write `SoundData` / `MultiData` / `GlobalData` structs with Edisyn-sourced clamping. Unit tests for SNDP encode/decode, SNDD checksum, wave nibble codec. **Do not proceed until tests pass.**  
     
3. **Hardware device.** `HardwareMidiDevice`: Universal Device Inquiry autodetect, SNDP send with rate limiter, SNDR/SNDD via MidiKraft-librarian, DISD parser. **Test against real XT with a MIDI monitor before building any UI.** Validate every message type.  
     
4. **PatchModel \+ EditorController.** Wire SDATA struct to HardwareMidiDevice. Add undo stack, A/B compare buffers, CC routing.  
     
5. **OSC and FILTER tabs.** First two editor pages with full skin system wired in. Validate all parameter changes on hardware in real time. End-to-end proof of concept.  
     
6. **Remaining editor tabs.** WAVE (with waveform preview), AMP, ENV (with ADSR displays), LFO (waveform icons), ARP (16-step grid), MISC, NAME (text input), GLOBAL, MULTI. Add padlock icons to all knobs (inactive until Phase 5).  
     
7. **Patch library.** Wire PatchDB from gearmulator. Import .syx, load patch, send to hardware. Bulk receive via MidiKraft-librarian with progress bar.  
     
8. **Wavetable and waveform editor.** Waveform canvas, harmonic editor, WAVD/WAVR. 3D waterfall visualizer. LCD mirror panel.  
     
9. **Modulation matrix.** 2D grid view, modifier panel, full sysex wiring.  
     
10. **Exploration engine.** Port mutation weights and all six algorithms from Edisyn. Wire to SNDP. Activate padlock buttons. Build Exploration panel with four sub-panels.  
      
11. **MIDI Learn overlay.** Port from gearmulator 2.2.2.  
      
12. **RMTP remote control.** Wire Store/Compare/Recall to RMTP button simulation.  
      
13. **Plugin automation.** Expose SDATA params as VST/AU/CLAP automation.  
      
14. **Polish and release.** Community skin support, Xenia routing option, in-UI help text, release build pipeline.

