# Specifications

This directory holds the project's specification documents and local Waldorf reference materials.

## What's tracked here

| File | Purpose |
|---|---|
| `editor-requirements.md` | Per-milestone acceptance criteria — the "definition of done" for each ROADMAP milestone. |
| `sysex-protocol.md` | Our own SysEx protocol reference, distilled from Waldorf's documentation and cross-checked against Edisyn and gearmulator. **Authoritative for this project.** |

## Reference PDFs (not tracked)

Three Waldorf documents are used as source material. They are **kept locally only** (gitignored) because the repo is public and the PDFs are © Waldorf Music. Anyone reproducing the project should download them directly from Waldorf:

**Source:** https://downloads.waldorfmusic.com/cloud/index.php/s/2BLZXbKrMw3x26N/download/Documentation%20EN.zip

After downloading and extracting the ZIP, place these files in `docs/spec/`:

| File | Pages | Content |
|---|---|---|
| `mw2_XT_sysex.pdf` | 26 | The authoritative SysEx specification (software release 2.16). Source for `sysex-protocol.md`. |
| `mw2_XT_controls.pdf` | 3 | MIDI CC number assignment table (software release 2.09). |
| `mw2_XT_XTk_eng.pdf` | 126 | The full operations manual ("MWII/XT/XTk Manual 2.3 E"). |

`pdftotext`-extracted `.txt` siblings are also gitignored if present.

## Convention

When `sysex-protocol.md` and the Waldorf PDF disagree, **`sysex-protocol.md` is authoritative.** The Waldorf doc has a number of typos and inconsistencies (recorded in `sysex-protocol.md` §Divergences), and Edisyn's hardware-tested implementation has resolved several spec ambiguities. Cite the Waldorf section number when relevant (e.g., "Waldorf §3.1 SDATA index 70") but treat our spec as the implementation target.
