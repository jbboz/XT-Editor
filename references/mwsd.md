# mwsd (jeanette-c)

**Source:** https://github.com/jeanette-c/mwsd
**Pinned commit:** `391d99b59cdcb421e3e0a46e52f6f2e272f1cf18` (2021-10-15 — "Minor changes to fix a compilation error with g++ 11.1")
**License:** GPL-3.0 (COPYING in repo root)

Cross-platform terminal utility (C++ + ncurses) that displays the Microwave II/XT LCD on a computer. Originally written to assist blind users. Small codebase, two pieces matter for us.

## What we use

| Need | Path in mwsd | Notes |
|---|---|---|
| Universal Device Inquiry autodetect | `synth_info.cpp` / `synth_info.hpp` | Broadcasts `F0 7E 7F 06 01 F7`, parses the device family member response to identify the XT and its device ID. Ported into our `HardwareMidiDevice` in M1.3. |
| DISD parser (80-char LCD + LED bitmask) | `curses_mw_ui.cpp` / `curses_mw_ui.hpp` | Decodes the `15h` DISD SysEx message into displayable text + LED state. Used in M1.3 (basic) and M3.6 (LCD mirror panel). |
| SysEx miner / extractor | `curses_mw_miner.cpp` / `curses_mw_miner.hpp` | Auxiliary — useful for understanding bank dump handling, not directly ported. |

## How it's used per milestone

- **M1.3** — port autodetect (`synth_info.cpp`) and DISD parser (`curses_mw_ui.cpp`) into `HardwareMidiDevice`. Apache-style attribution headers per file are unnecessary since this is GPL-3.0 → AGPL-3.0 (one-way compatible into AGPLv3), but record per-file provenance in `ATTRIBUTIONS.md`.
- **M3.6** — DISD parser drives the real-time LCD mirror panel UI.

## License obligations

GPL-3.0. AGPL-3.0 final project incorporating GPL-3.0 source is permitted. Per-file attribution in `ATTRIBUTIONS.md`.
