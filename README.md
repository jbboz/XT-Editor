# Waldorf Microwave II/XT Editor

A modern cross-platform parameter editor and patch librarian for the Waldorf **Microwave II**, **Microwave XT**, and **Microwave XTk** wavetable synthesizers. Built in C++/JUCE, distributed as a standalone application and as VST3/AU plugins.

## Status

🚧 **Early development — no usable build yet.**

The project is currently in Phase 1 scaffolding. The specification, architectural reference, and execution roadmap are complete; protocol-layer code begins at milestone M1.1.

- **Phase 0 (reference acquisition, decoupling spike, parameter-JSON validation): ✅ complete**
- **Phase 1 (core editor MVP): 🔨 in progress**
- Phases 2–6 planned

See [`ROADMAP.md`](ROADMAP.md) for the full plan and progress.

## What this is

The Microwave II/XT/XTk are wavetable synthesizers Waldorf shipped between 1997 and 2001. They have a small LCD, a few encoders, and a deep, modulation-rich voice architecture that is genuinely difficult to program from the front panel. Existing editor software — XplorerMac, mwaveXL, monstrumWaveXT — works, but each is incomplete or aged out.

This project replaces them with a **single editor that respects the hardware's section grouping but eliminates its page system**, plus a SQLite-backed patch librarian and a suite of patch-exploration tools that have no equivalent in any other Microwave editor.

The editor talks to real hardware via MIDI/SysEx. It does **not** embed an emulator — but it can drive [Xenia](https://github.com/dsp56300/gearmulator) (gearmulator's Microwave emulation) over a MIDI loopback, so contributors and users without hardware can still use it. See [`docs/xenia-setup.md`](docs/xenia-setup.md).

## Why another editor

The hardware's biggest usability problems are LCD-driven: one parameter at a time, page navigation everywhere, no overview of the 16-slot modulation matrix, no visualization of the wavetable. This project's design lens is:

| Hardware limitation | Editor solution |
|---|---|
| LCD shows one parameter at a time | All section parameters visible simultaneously |
| Modulation matrix requires slot-by-slot navigation | 2D source × destination grid with full overview |
| Wavetable position is a number, no visual | Inline waveform preview + 3D waterfall in wavetable editor |
| Arpeggiator pattern buried in sub-pages | 16-step click grid directly on the ARP tab |
| Name editor: scroll through ASCII characters | Direct text input |
| Filter type shown as numbers | Named selector (24dB LP, Waveshaper, etc.) |
| Shift+button combos require physical access | RMTP simulates any button press from the editor |
| No randomization or exploration | Full exploration engine: mutation, merge, morph, hill-climbing |
| Envelope shapes not visible | Graphical ADSR display per envelope, live updating |
| LFO shapes shown as numbers | Waveform-icon selectors |

The visual identity is the XT itself — dark chassis, orange accents, hardware-faithful section grouping.

## Planned features

### Phase 1 — Core editor (MVP)
- Full parameter editor: OSC, WAVE, FILTER, AMP, ENV (3 envelopes + 8-stage Wave Env), LFO, ARP (with 16-step click grid), MISC, NAME, GLOBAL, MULTI
- Bidirectional MIDI/SysEx sync (UI ↔ hardware)
- Undo/redo on every parameter change
- A/B compare
- Universal Device Inquiry autodetect (no manual device-ID config)
- SNDP rate limiter (the firmware-protection coalescing queue)

### Phase 2 — Patch library
- SQLite-backed library with full-text search, tags, three-state rating
- Drag-to-edit-buffer, drag-to-bank-slot
- Bulk receive/send via custom SNDR→SNDD state machine with progress bar
- Export to `.syx` / `.mid`
- A/B compare with changed-parameter highlighting

### Phase 3 — Wavetable & waveform editor
- 64-sample waveform canvas (pencil + line tools + harmonic editor + audio import)
- 64-entry wavetable editor with spectral interpolation preview
- 3D wavetable waterfall visualizer
- User-wave (1000–1249) and user-wavetable (96–128) upload
- Real-time LCD mirror panel via DISD

### Phase 4 — Modulation matrix
- 32 sources × 36 destinations grid view, with all 16 mod slots mapped onto cells (stacked when multiple slots share a route)
- 4 modifier slots with 16 operations each
- Click-to-add / drag-to-set-amount

### Phase 5 — Patch exploration engine
- **Mutate** — weighted random perturbation with per-section enable/disable and per-parameter lock buttons
- **Merge / Nudge** — recombine current patch with library targets
- **Morph** — 2D XY pad over four corner patches; real-time SNDP, MIDI CC assignable to axes
- **Hill-climb** — interactive evolutionary algorithm; user grades candidates, engine converges
- **Constriction** — progressive narrowing of mutation range for fine-tuning

### Phase 6 — Polish & release
- MIDI Learn overlay (right-click any control → assign CC)
- RMTP front-panel button simulation (Store / Compare / Recall without physical access)
- DAW automation exposure (VST3/AU; CLAP deferred)
- Community skin support
- Xenia compatibility mode for hardware-free use
- Signed/notarized release pipeline (macOS, Windows, Linux)

## Architecture

Layered, mostly platform-independent:

```
┌────────────────────────────────────────────────────┐
│              JUCE plugin shell                     │
│  AudioProcessor · AudioProcessorEditor · MIDI I/O  │
└──────────────────────┬─────────────────────────────┘
                       │
       ┌───────────────▼────────────────┐
       │       EditorController         │
       │  patch state · undo · CC routing│
       └─┬─────────────┬─────────────────┘
         │             │
┌────────▼─────┐  ┌────▼────────────────────────────┐
│  PatchModel  │  │       HardwareMidiDevice         │
│  SDATA[256]  │  │  autodetect · SNDP rate limiter  │
│  MDATA + 8×  │  │  bulk SNDR→SNDD state machine    │
│  IDATA[28]   │  │  DISD parser · all message types │
│  GDATA[32]   │  └──────────────┬───────────────────┘
└──────┬───────┘                 │
       │                  juce::MidiOutput / Input
       ▼
┌────────────────────────────────────┐
│       PatchDB (SQLite)             │
│  import · export · search · tags   │
└────────────────────────────────────┘
       │
┌──────▼──────────────────────────────────────────────┐
│                 EditorUI (JUCE)                      │
│  Tabbed pages · 2D mod matrix · wavetable editor     │
│  ADSR displays · exploration panel · LCD mirror      │
└──────────────────────────────────────────────────────┘
```

The protocol layer (`mw2xtLib`) has no JUCE dependencies and is unit-testable in plain C++17.

See [`docs/spec/editor-requirements.md`](docs/spec/editor-requirements.md) for the per-milestone acceptance criteria and [`docs/spec/sysex-protocol.md`](docs/spec/sysex-protocol.md) for our authoritative SysEx reference (distilled from Waldorf's docs, with every typo and inconsistency catalogued).

## Built on / inspired by

This project stands on a lot of upstream work. Several components are copied or ported with attribution; others are read-only references that informed the design.

| Project | Used as | License |
|---|---|---|
| [JUCE](https://juce.com/) | Application framework, audio plugin formats, MIDI | AGPLv3 option |
| [gearmulator](https://github.com/dsp56300/gearmulator) (Xenia) | Parameter description JSON, wave/wavetable codec, PatchDB SQLite layer, MIDI Learn core, skin assets, dev-time MIDI loopback target | GPL-3.0 |
| [Edisyn](https://github.com/eclab/edisyn) | Hardware-tested reference for SysEx spec edge cases; mutation weight table; six exploration algorithms (mutate / merge / nudge / morph / hill-climb / constrict) | Apache-2.0 |
| [mwsd](https://github.com/jeanette-c/mwsd) | Universal Device Inquiry autodetect; DISD (LCD) parser | GPL-3.0 |
| [MidiKraft-librarian](https://github.com/christofmuc/MidiKraft-librarian) | Reference patterns for the SNDR→SNDD state machine (not linked — see [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md)) | AGPL-3.0 |
| [sqlite_orm](https://github.com/fnc12/sqlite_orm) | SQLite C++ ORM for the patch library | AGPL-3.0 / commercial |
| Waldorf Music | The Microwave II SysEx specification (consulted; not redistributed in this repo) | © Waldorf Music |

Per-file attribution lives in [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md).

## Documentation

- [`ROADMAP.md`](ROADMAP.md) — milestone-by-milestone execution plan with effort tags, gates, and the open-decisions log
- [`docs/spec/editor-requirements.md`](docs/spec/editor-requirements.md) — per-milestone acceptance criteria + cross-cutting non-functional requirements + the [testing strategy](docs/spec/editor-requirements.md#testing-strategy-xenia--real-hardware)
- [`docs/spec/sysex-protocol.md`](docs/spec/sysex-protocol.md) — our authoritative SysEx protocol reference
- [`docs/spec/README.md`](docs/spec/README.md) — how to obtain the Waldorf source PDFs (not vendored in this repo)
- [`docs/xenia-setup.md`](docs/xenia-setup.md) — dev/test harness setup via Xenia + macOS IAC Bus
- [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md) — per-component licensing and source provenance
- [`mw2xt_editor_development_plan.md`](mw2xt_editor_development_plan.md) — original architectural reference (the dev plan)
- [`references/`](references/) — per-upstream notes with pinned commit hashes and per-milestone usage maps

## License

The combined work is **AGPL-3.0**. See [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md) for the per-upstream license table and how the combination is permitted.

## Hardware compatibility

Targeted hardware:

- Waldorf **Microwave II** (rackmount)
- Waldorf **Microwave XT** (tabletop)
- Waldorf **Microwave XTk** (keyboard)

Optional alternative:

- **Xenia** (gearmulator's Microwave emulation) — drive the editor over a MIDI loopback. Useful for development, testing, and hardware-free experimentation. Setup: [`docs/xenia-setup.md`](docs/xenia-setup.md).

## Contributing

Project is in early planning. The codebase doesn't exist yet — Phase 1 scaffolding begins shortly. Once the protocol layer (M1.2) is in place, PRs will be welcome.

In the meantime, the best ways to help are:

- File issues with bug reports about existing Microwave editor software that you'd like this project to address
- Annotate [`docs/spec/sysex-protocol.md`](docs/spec/sysex-protocol.md) §Open items if you have hardware-verified answers (especially the Filter 1 Type 10–12 question)
- Report on real-XT firmware quirks not captured in the Waldorf docs

## Acknowledgements

This project would not exist without the prior work documented in [`ATTRIBUTIONS.md`](ATTRIBUTIONS.md). Particular thanks to:

- **Sean Bolton** and the Edisyn maintainers for hardware-verified MW II/XT support and the exploration algorithms
- **dsp56300** for Xenia and the gearmulator infrastructure that makes hardware-free testing possible
- **Christof Ruch** for KnobKraft/MidiKraft, whose patterns inform the bulk-transfer design even though we don't link the library
- **Jeanette C.** for mwsd — the autodetect and DISD parsing logic
- **Waldorf Music** for the Microwave II SysEx specification (consulted as the source-of-truth for protocol shape)
