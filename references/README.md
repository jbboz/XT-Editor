# References

Read-only third-party source we consult while building the editor. **Nothing in this directory is compiled or linked.** Build dependencies live elsewhere (added as git submodules during M1.1).

Each upstream repo is cloned into `references/<name>/` and gitignored. Provenance (source URL, pinned commit hash, license, file locations, observations) lives in a sibling `references/<name>.md` file that **is** committed.

To set up a fresh checkout:

```sh
git clone https://github.com/dsp56300/gearmulator.git references/gearmulator
git clone https://github.com/eclab/edisyn.git              references/edisyn
git clone https://github.com/jeanette-c/mwsd.git           references/mwsd
```

Then `git checkout` the pinned commit hash recorded in each `<name>.md` to match the state this project was developed against.

## Why this layout

- **Cloned trees gitignored** so the project repo stays small and upstream history doesn't pollute it.
- **Notes files committed** so anyone can reproduce the reference state.
- **Per-repo notes** call out the specific files that matter for each milestone, saving the next person from re-doing the spelunking.

## Project files of interest by reference

| File / area | Source | Used by |
|---|---|---|
| `parameterDescriptions_xt.json` | [gearmulator](gearmulator.md) | M1.2 protocol layer; "the most important single file in the project" |
| Wave/wavetable codec (XOR-flip) | [gearmulator](gearmulator.md) | M1.2 |
| PatchDB + Patch Manager | [gearmulator](gearmulator.md) | M2.1, M2.2 (subject to D-01) |
| Skin system | [gearmulator](gearmulator.md) | M1.1 scaffold, M1.5 wiring (subject to D-01) |
| MIDI Learn overlay | [gearmulator](gearmulator.md) | M6.1 (subject to D-01) |
| `WaldorfMicrowaveXT.java` | [edisyn](edisyn.md) | M1.2 (clamping), M5.1 (mutation weights), M5.2 (algorithms) |
| `Synth.java` (exploration algorithms) | [edisyn](edisyn.md) | M5.2 |
| Universal Device Inquiry autodetect | [mwsd](mwsd.md) | M1.3 |
| DISD parser (LCD + LED) | [mwsd](mwsd.md) | M1.3, M3.6 |
