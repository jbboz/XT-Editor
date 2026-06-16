# Xenia default skin assets

PNG bitmaps + TTF font + JSON layout file copied from gearmulator's `xtJucePlugin/skins/xtDefault/` (pinned commit recorded in [`references/gearmulator.md`](../../../../references/gearmulator.md)).

**License:** GPL-3.0 (compatible with our AGPL-3.0 combined work per GPLv3 §13). Asset-level attribution recorded in [`../../../../ATTRIBUTIONS.md`](../../../../ATTRIBUTIONS.md).

**Wired in at:** M1.5 (skin renders the OSC + FILTER tabs as the vertical-slice gate).

The `.rml` / `.rcss` / `assets.cmake` files from gearmulator's directory are **not copied** — those are gearmulator's RmlUi-specific bindings. We reimplement the skin engine native to JUCE (per D-01) and bind these PNG/TTF assets ourselves via `LookAndFeel` and custom `Component` subclasses.

The `xtDefault.json` layout file IS copied (when assets land) as a reference for layout intent — we may parse it for coordinate hints or use it as documentation of where each control sat in gearmulator's UI.
