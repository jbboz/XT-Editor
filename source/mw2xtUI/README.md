# mw2xtUI — UI layer

JUCE `Component` subclasses, custom `LookAndFeel`, skin asset bindings.

**Filled in at:** M1.5 (OSC + FILTER tabs, gate; first skin wiring) and M1.6 (remaining tabs).

**Contents (planned):**
- Per-tab `Component` for OSC, WAVE, FILTER, AMP, ENV (with graphical ADSR + Wave Env), LFO (waveform icons), ARP (16-step grid), MISC, NAME, GLOBAL, MULTI
- 2D modulation matrix grid (M4)
- Wavetable editor / waveform editor / 3D waterfall visualizer (M3)
- LCD mirror panel (M3)
- Exploration panel with four sub-panels (M5)
- Custom `LookAndFeel` that consumes the Xenia PNG assets in `skins/xtDefault/`

See [D-01](../../ROADMAP.md) and [`../../ATTRIBUTIONS.md`](../../ATTRIBUTIONS.md) — we reimplement the skin engine native to JUCE rather than borrowing gearmulator's RmlUi-based engine.
