# Waldorf Microwave II / XT / XTk — SysEx Protocol

Our distillation of the Microwave II SysEx protocol as we will implement it. Source material:

- **Waldorf §x.y** = section reference into `docs/spec/mw2_XT_sysex.pdf` (release 2.16). Kept locally only; see [`README.md`](README.md).
- **Waldorf CC §** = `docs/spec/mw2_XT_controls.pdf` (release 2.09).
- **Edisyn** = the hardware-tested Java implementation at `references/edisyn/edisyn/synth/waldorfmicrowavext/WaldorfMicrowaveXT.java`.

When this document and the Waldorf PDF disagree, **this document is authoritative**. The PDF has multiple typos (catalogued in §Divergences) and Edisyn has corrected several of them through hardware testing.

---

## Contents

1. [Conventions](#conventions)
2. [Frame structure](#frame-structure)
3. [Message ID matrix](#message-id-matrix)
4. [Message reference](#message-reference)
5. [Data layouts](#data-layouts) — SDATA, MDATA, IDATA, GDATA
6. [Wave codecs](#wave-codecs) — WDATA, WCTDATA
7. [Lookup tables](#lookup-tables) — modulation sources/destinations, modifiers, filter types, play parameters
8. [Device Inquiry](#device-inquiry)
9. [MIDI Control Change map](#midi-control-change-map)
10. [Divergences from the Waldorf doc](#divergences-from-the-waldorf-doc)
11. [Implementation notes](#implementation-notes)

---

## Conventions

- **Hex** values use a trailing `h` (Waldorf style) or `0x` prefix interchangeably.
- **Bit numbering:** bit 0 = least significant. The SysEx data-byte invariant is bit 7 = 0 (range `00h`–`7Fh`) except for `F0h`/`F7h` framing bytes.
- **Range notation `0-127`** in tables is inclusive; **"reserved"** means the byte exists in the layout but currently has no meaning. Reserved bytes must be sent as `0` and must be preserved on round-trip (read in, write back unchanged) so we don't corrupt future firmware extensions.
- **"MW2"** here is the Microwave II (rackmount); **"XT"** is the Microwave XT (knob-laden tabletop); **"XTk"** is the keyboard variant. They share firmware and the protocol applies identically except where flagged "XT only" or "MW2 only".
- **Parameter index `H.PP`** notation (e.g., `0.62` for filter cutoff) is shorthand for the SNDP HH/PP pair: high byte then low byte.

## Frame structure

Every SysEx message follows:

```
F0  3E  0E  DEV  IDM  [data ...]  [XSUM]  F7
```

| Pos | Field | Value | Meaning |
|---|---|---|---|
| 0 | `EXC` | `F0h` | SysEx start |
| 1 | `IDW` | `3Eh` | Waldorf Music manufacturer ID |
| 2 | `IDE` | `0Eh` | Equipment ID for Microwave II family |
| 3 | `DEV` | `00h`–`7Eh`, `7Fh`=broadcast | Device ID (configurable in §GDATA index 6) |
| 4 | `IDM` | see [Message ID matrix](#message-id-matrix) | Message type |
| 5… | data | `00h`–`7Fh` per byte | Message-specific payload |
| n-1 | `XSUM` | `Σ(data) & 7Fh` | Checksum — sum of databytes in 8-bit, masked to 7 bits |
| n | `EOX` | `F7h` | SysEx end |

**Checksum rules:**
- Sum is over **data bytes only**. `IDW`, `IDE`, `DEV`, `IDM`, `F0h`, `F7h` are not summed.
- 8-bit addition, then `& 0x7F` (NOT modular arithmetic on 7-bit values during accumulation).
- `XSUM = 7Fh` is accepted as universally valid (a software bypass — useful when manually crafting test messages).
- If a message has zero data bytes (e.g., a simple request like GLBR), `XSUM = 00h`.
- **SNDP, MULP, GLBP omit the checksum entirely.** This is intentional; the messages have a fixed short length and the parser knows to expect `F7h` directly after the value byte. (See Waldorf §2.13, §2.23, §2.53.)

**Length ordering across messages** (useful for parser dispatch by length once IDM is known):

| Message | Total length (bytes incl. F0/F7) |
|---|---|
| SNDR / MULR / WAVR / WCTR / DISR | 9 |
| GLBR / MODR | 7 |
| SNDP | 10 (no checksum) |
| MULP / GLBP | 8 (no checksum) |
| RMTP | 9 |
| DISP | 9 |
| DISL | 7 |
| DISD | 88 |
| GLBD | 39 |
| MODD | 7 |
| SNDD (single) | 265 |
| MULD (single) | 265 |
| WAVD | 137 |
| WCTD | 265 |
| SNDD (All Sounds) | 65545 |

## Message ID matrix

Waldorf encodes IDM as `<dump-type><data-type>`:

| Data type (low nibble) | Description |
|---|---|
| `0` | SND — Sound |
| `1` | MUL — Multi |
| `2` | WAV — Wave |
| `3` | WCT — Wave control table |
| `4` | GLB — Global parameters |
| `5` | DIS — Display |
| `6` | RMT — Remote control |
| `7` | MOD — Sound/Multi mode |
| `8` | INF — Information (MWPC variants only; we don't use) |

| Dump type (high nibble) | Description |
|---|---|
| `0x` | Request |
| `1x` | Dump |
| `2x` | Parameter change |
| `3x` | Store |
| `4x` | Recall |
| `5x` | Compare |

**Valid IDMs the editor uses:**

| IDM | Label | Description | Direction (editor) |
|---|---|---|---|
| `00h` | SNDR | Sound Request | TX |
| `10h` | SNDD | Sound Dump | TX + RX |
| `20h` | SNDP | Sound Parameter Change | TX + RX |
| `01h` | MULR | Multi Request | TX |
| `11h` | MULD | Multi Dump | TX + RX |
| `21h` | **MULP** (see [Divergences](#divergences-from-the-waldorf-doc)) | Multi Parameter Change | TX + RX |
| `02h` | WAVR | Wave Request | TX |
| `12h` | WAVD | Wave Dump | TX + RX |
| `03h` | WCTR | Wave Control Table Request | TX |
| `13h` | WCTD | Wave Control Table Dump | TX + RX |
| `04h` | **GLBR** (see Divergences) | Global Parameter Request | TX |
| `14h` | GLBD | Global Parameter Dump | TX + RX |
| `24h` | GLBP | Global Parameter Change | TX + RX |
| `05h` | DISR | Display Request | TX |
| `15h` | DISD | Display Dump | RX |
| `25h` | DISP | Display Parameter Change | (unused) |
| `45h` | DISL | Display Recall | (unused) |
| `26h` | RMTP | Remote Control | TX |
| `07h` | MODR | Mode Request | TX |
| `17h` | MODD | Mode Dump | RX |

---

## Message reference

### SNDR — Sound Request *(IDM `00h`)*

Requests one or more sound dumps from the XT. Location given in two bytes:

| BB NN | Location |
|---|---|
| `00 00`–`00 7F` | Locations A001–A128 |
| `01 00`–`01 7F` | Locations B001–B128 |
| `10 00` | All Sounds (entire library — 256 patches; expect a 65 KB SNDD reply) |
| `20 00` | Sound Mode edit buffer |
| `30 00`–`30 07` | Multi Mode instrument 1–8 sound buffer |

Frame: `F0 3E 0E DEV 00 BB NN ((BB+NN)&7F) F7` — total 9 bytes.

### SNDD — Sound Dump *(IDM `10h`)*

Carries a full SDATA payload. Same location encoding as SNDR.

- **Single sound:** 256 SDATA bytes between BB/NN and XSUM. Frame length 265 bytes.
- **All Sounds** (BB NN = `10 00`): 256 × 256 SDATA bytes, one XSUM at end. Frame length 65,545 bytes. Expect this transfer to take **30+ seconds** over real hardware; use MidiKraft-librarian with progress reporting.

### SNDP — Sound Parameter Change *(IDM `20h`)*

Real-time per-parameter edit. **No checksum.**

Frame: `F0 3E 0E DEV 20 LL HH PP XX F7` — total 10 bytes.

| Field | Range | Meaning |
|---|---|---|
| `LL` | `00h` or `00h–07h` | `00h` = Sound Mode edit buffer; `01h`–`07h` = Multi Mode instrument 1–8 buffer (a `01h` LL targets instrument 2, but a `00h` LL is ambiguous — see [Implementation notes](#implementation-notes)) |
| `HH` | `00h` or `01h` | High byte of parameter index — `00h` for SDATA 0–127, `01h` for SDATA 128–255 |
| `PP` | `00h–7Fh` | Low byte of parameter index |
| `XX` | per parameter | Value, see [SDATA layout](#sdata--sound-data-256-bytes) |

The HH split at index 128 is a silent-wrong-parameter bug surface: writing to index 130 with HH=00h actually writes to index 2 (Osc 1 Semitone). Test coverage required.

### MULR — Multi Request *(IDM `01h`)*

Like SNDR but for Multi patches. Locations: `00 00`–`00 7F` = Multi 001–128; `10 00` = All Multis; `20 00` = edit buffer.

Frame: `F0 3E 0E DEV 01 BB NN ((BB+NN)&7F) F7` — total 9 bytes.

### MULD — Multi Dump *(IDM `11h`)*

Full Multi payload: 32 MDATA bytes + 8 × 28 IDATA bytes = 256 data bytes between BB/NN and XSUM. Frame length 265 bytes.

### MULP — Multi Parameter Change *(IDM `21h` — note typo in Waldorf §2.23 title)*

In Sound Mode all MULP messages are ignored by hardware.

Frame: `F0 3E 0E DEV 21 LL PP XX F7` — total 9 bytes. **No checksum.**

| Field | Range | Meaning |
|---|---|---|
| `LL` | `20h` or `01h–07h` | `20h` = Multi edit buffer; `01h–07h` = Multi Mode instrument 1–7 buffer (instrument 8 = `00h`? The Waldorf doc says `01h–07h` for "instrument 1–8"; ambiguous. Edisyn cross-check required.) |
| `PP` | `00h–1Fh` | Parameter index into MDATA (§3.2) or IDATA (§3.3) depending on `LL` |
| `XX` | per parameter | Value |

### WAVR — Wave Request *(IDM `02h`)*

Locations:

| HH LL | Wave |
|---|---|
| `00 00`–`00 7F` | ROM Waves 000–127 |
| `01 00`–`01 7F` | ROM Waves 128–255 |
| `02 00`–`02 2B` | ROM Waves 256–299 |
| `07 68`–`07 7F` | User Waves 1000–1023 |
| `08 00`–`08 7F` | User Waves 1024–1151 |
| `09 00`–`09 61` | User Waves 1152–1249 |

Frame: `F0 3E 0E DEV 02 HH LL ((HH+LL)&7F) F7` — total 9 bytes.

### WAVD — Wave Dump *(IDM `12h`)*

128 WDATA nibble bytes (= 64 samples; see [Wave codecs](#wave-codecs)). Frame length 137 bytes.

### WCTR — Wave Control Table Request *(IDM `03h`)*

Locations: `00 00`–`00 7F` = wavetables 001–128. Some wavetables are algorithmic and have no control table; the XT silently fails the request in that case.

Frame: `F0 3E 0E DEV 03 HH LL ((HH+LL)&7F) F7` — total 9 bytes.

### WCTD — Wave Control Table Dump *(IDM `13h`)*

256 WCTDATA nibbles (= 64 wave-index entries; see [Wave codecs](#wave-codecs)). User-writable range is wavetables 96–128 only; writing 1–95 silently fails. Frame length 265 bytes.

### GLBR — Global Parameter Request *(IDM `04h`)*

No location. Frame: `F0 3E 0E DEV 04 00 F7` — total 7 bytes (`XSUM = 0` per the empty-data convention).

### GLBD — Global Parameter Dump *(IDM `14h`)*

32 GDATA bytes. Frame length 39 bytes.

### GLBP — Global Parameter Change *(IDM `24h`)*

Frame: `F0 3E 0E DEV 24 PP XX F7` — total 8 bytes. **No checksum.**

| Field | Range | Meaning |
|---|---|---|
| `PP` | `00h–1Fh` | Parameter index into GDATA (§3.6) |
| `XX` | per parameter | Value |

### DISR — Display Request *(IDM `05h`)*

No location. Frame: `F0 3E 0E DEV 05 00 F7` — total 7 bytes.

### DISD — Display Dump *(IDM `15h`)*

80 ASCII bytes (upper + lower LCD row, 40 each) + 1 LED bitmask byte. Frame length 88 bytes.

LED bitmask bits:

| Bit | Light |
|---|---|
| `01` | MIDI |
| `02` | Column #1 |
| `04` | Column #2 |
| `08` | Column #3 |
| `10` | Column #4 |
| `20` | Column #5 |
| `40` | Play |

(Bit `80h` is reserved — the bitmask remains a 7-bit data byte, so the MSB must be 0.)

### RMTP — Remote Control Parameter Change *(IDM `26h`)*

Simulates encoder turns and button presses. The dev plan calls out RMTP as the key capability that lets us implement Store/Compare/Recall without physical button presses.

Frame: `F0 3E 0E DEV 26 UU MM ((UU+MM)&7F) F7` — total 9 bytes.

| `UU` | Element |
|---|---|
| `00` | Encoder #1 (left) |
| `01` | Encoder #2 |
| `02` | Encoder #3 |
| `03` | Encoder #4 |
| `04` | Encoder #5 (big red one) |
| `05` | Play / Shift button |
| `06` | Soundpar #1 / Store button |
| `07` | Soundpar #2 / Recall button |
| `08` | Soundpar #3 / Compare button |
| `09` | Multipar / Undo button |
| `0A` | Global / Utility button |
| `0B` | Power button |

`MM` (movement / button state):

| `MM` | Encoder action | Button action |
|---|---|---|
| `00` | turn left by 64 | released |
| `01` | turn left by 63 | pressed |
| `02`–`3F` | turn left by `MM` | pressed |
| `40` | (no encoder move) | pressed |
| `41` | turn right by 1 | pressed |
| `42`–`7F` | turn right by `MM` | pressed |

Waldorf §2.71 cautions that RMTP "might still introduce bugs." Validate each RMTP path against hardware before committing to the M6.2 RMTP remote-control milestone.

### MODR — Mode Request *(IDM `07h`)*

No data. Frame: `F0 3E 0E DEV 07 F7` — total 7 bytes (Waldorf doc omits XSUM here; consistent with the empty-data zero-checksum convention).

### MODD — Mode Dump *(IDM `17h`)*

Frame: `F0 3E 0E DEV 17 MM F7` — total 7 bytes. `MM = 0` for Sound mode, `MM = 1` for Multi mode.

---

## Data layouts

### SDATA — Sound Data (256 bytes)

| Idx | Range | Display | Parameter |
|---|---|---|---|
| 0 | `0–1` | `1` | Sound Format Version (currently 1; format 0 is unpublished) |
| 1 | `16–112` | `-4..+4` | Osc 1 Octave (in steps of 12) |
| 2 | `52–76` | `-12..+12` | Osc 1 Semitone |
| 3 | `0–127` | `-64..+64` | Osc 1 Detune |
| 4 | reserved | | |
| 5 | `0–122` | `0–120, harmonic, global` | Osc 1 Pitch Bend Range |
| 6 | `0–76` | `-100%..+200%` | Osc 1 Keytrack |
| 7 | `0–127` | | Osc 1 FM Amount |
| 8–11 | reserved | | |
| 12 | `16–112` | `-4..+4` | Osc 2 Octave |
| 13 | `52–76` | `-12..+12` | Osc 2 Semitone |
| 14 | `0–127` | `-64..+64` | Osc 2 Detune |
| 15 | reserved | | |
| 16 | `0–1` | off / on | Osc 2 Sync |
| 17 | `0–122` | `0–120, harmonic, global` | Osc 2 Pitch Bend Range |
| 18 | `0–76` | `-100%..+200%` | Osc 2 Keytrack |
| 19 | `0–1` | off / on | Osc 2 Link |
| 20–24 | reserved | | |
| 25 | `0–127` | wavetable 001–128 | Wavetable |
| 26 | `0–63` | `0..60, tri, sqr, saw` | Wave 1 Startwave |
| 27 | `0–127` | `free, 3–357 deg` | Wave 1 Start Phase |
| 28 | `0–127` | `-64..+64` | Wave 1 Envelope Amount |
| 29 | `0–127` | `-64..+64` | Wave 1 Envelope Velocity Amount |
| 30 | `0–127` | `-200%..+197%` | Wave 1 Keytrack |
| 31 | `0–1` | off / on | Wave 1 Limit |
| 32–35 | reserved | | |
| 36 | `0–63` | `0..60, tri, sqr, saw` | Wave 2 Startwave |
| 37 | `0–127` | `free, 3–357 deg` | Wave 2 Start Phase |
| 38 | `0–127` | `-64..+64` | Wave 2 Envelope Amount |
| 39 | `0–127` | `-64..+64` | Wave 2 Envelope Velocity Amount |
| 40 | `0–127` | `-200%..+197%` | Wave 2 Keytrack |
| 41 | `0–1` | off / on | Wave 2 Limit |
| 42 | `0–1` | off / on | Wave 2 Link |
| 43–46 | reserved | | |
| 47 | `0–127` | `0..127` | Mix Wave 1 |
| 48 | `0–127` | `0..127` | Mix Wave 2 |
| 49 | `0–127` | `0..127` | Mix Ringmod |
| 50 | `0–127` | `0..127` | Mix Noise |
| 51 | `0–127` | `0..127` | Mix External **(XT only)** |
| 52 | reserved | | |
| 53 | `0–5` | off, 1–5 | Aliasing |
| 54 | `0–5` | off, 1–5 | Time Quantization |
| 55 | `0–1` | saturate / overflow | Clipping |
| 56 | reserved | | |
| 57 | `0–1` | off / on | Accuracy |
| 58 | `0–82` | [Play params §3.11](#play-parameters-3-11) | Play Parameter #1 |
| 59 | `0–82` | [Play params §3.11](#play-parameters-3-11) | Play Parameter #2 |
| 60 | `0–82` | [Play params §3.11](#play-parameters-3-11) | Play Parameter #3 |
| 61 | `0–82` | [Play params §3.11](#play-parameters-3-11) | Play Parameter #4 |
| 62 | `0–127` | `0..127` | Filter 1 Cutoff |
| 63 | `0–127` | `0..127` | Filter 1 Resonance |
| 64 | `0–9` | [Filter types](#filter-1-types-3-15) | Filter 1 Type |
| 65 | `0–127` | `-200%..+197%` | Filter 1 Keytrack |
| 66 | `0–127` | `-64..+63` | Filter 1 Envelope Amount |
| 67 | `0–127` | `-64..+63` | Filter 1 Envelope Velocity Amount |
| 68–69 | reserved | | |
| 70 | `0–127` | context-sensitive | Filter 1 Special Parameter (label depends on Filter 1 Type — see [Filter Type Context](#filter-type-context)) |
| 71–72 | reserved | | |
| 73 | `0–127` | `0..127` | Filter 2 Cutoff |
| 74 | `0–1` | 6dB LP / 6dB HP | Filter 2 Type |
| 75 | `0–127` | `-200%..+197%` | Filter 2 Keytrack |
| 76 | `0–7` MW2 / `0–35` XT | | Effect Type *(subject to firmware change)* |
| 77 | `0–127` | `0..127` | Amplifier Volume |
| 78 | reserved | | |
| 79 | `0–127` | `-64..+63` | Amplifier Envelope Velocity Amount |
| 80 | `0–127` | `-200%..+197%` | Amplifier Keytrack |
| 81 | `0–127` | | Effect Parameter #1 |
| 82 | `0–1` | off / on | Chorus |
| 83 | `0–127` | | Effect Parameter #2 |
| 84 | `0–127` | left 64–center–right 63 | Panning |
| 85 | `0–127` | `-200%..+197%` | Panning Keytrack |
| 86 | `0–127` | | Effect Parameter #3 |
| 87 | `0–1` | off / on | Glide Active |
| 88 | `0–3` | porta, gliss, finger-porta, finger-gliss | Glide Type |
| 89 | `0–1` | exp / linear | Glide Mode |
| 90 | `0–127` | `0..127` | Glide Time |
| 91 | reserved | | |
| 92 | `0–2` | off, on, hold | Arpeggiator Active |
| 93 | `1–127` | extern, 50–300 BPM | Arpeggiator Tempo |
| 94 | `0–15` | `1/1..1/32` | Arpeggiator Clock |
| 95 | `1–10` | `1..10` | Arpeggiator Range |
| 96 | `0–16` | off, user, 1–15 | Arpeggiator Pattern |
| 97 | `0–3` | up, down, alt, random | Arpeggiator Direction |
| 98 | `0–3` | note, note-rev, played, played-rev | Arpeggiator Note Order |
| 99 | `0–1` | root note / last note | Arpeggiator Velocity |
| 100 | `0–1` | off / on | Arpeggiator Reset on Pattern Start |
| 101 | `0–15` | `1..16` | Arpeggiator User Pattern Length |
| 102 | `0–15` | 4-step bitmask | Arpeggiator User Pattern positions 1–4 |
| 103 | `0–15` | 4-step bitmask | Arpeggiator User Pattern positions 5–8 |
| 104 | `0–15` | 4-step bitmask | Arpeggiator User Pattern positions 9–12 |
| 105 | `0–15` | 4-step bitmask | Arpeggiator User Pattern positions 13–16 |
| 106–107 | reserved | | |
| 108 | `0–1` | Poly / Mono | Allocation Mode |
| 109 | `0–2` | normal / dual / unisono | Assignment |
| 110 | `0–127` | `0..127` | Detune |
| 111 | reserved | | |
| 112 | `0–127` | | De-Pan |
| 113 | `0–127` | `0..127` | Filter Env Attack |
| 114 | `0–127` | `0..127` | Filter Env Decay |
| 115 | `0–127` | `0..127` | Filter Env Sustain |
| 116 | `0–127` | `0..127` | Filter Env Release |
| 117 | `0–2` | normal, single, retrigger | Filter Env Trigger |
| 118 | reserved | | |
| 119 | `0–127` | `0..127` | Amp Env Attack |
| 120 | `0–127` | `0..127` | Amp Env Decay |
| 121 | `0–127` | `0..127` | Amp Env Sustain |
| 122 | `0–127` | `0..127` | Amp Env Release |
| 123 | `0–2` | normal, single, retrigger | Amp Env Trigger |
| 124 | reserved | | |
| 125–140 | `0–127` | times `0..127`, levels `0..127` | Wave Env 8 stages (Time/Level pairs) |
| 141 | `0–2` | normal, single, retrigger | Wave Env Trigger |
| 142 | `0–1` | off / on | Wave Key-On Loop |
| 143 | `0–7` | `1..8` | Wave Key-On Loop Start |
| 144 | `0–7` | `1..8` | Wave Key-On Loop End |
| 145 | `0–1` | off / on | Wave Key-Off Loop |
| 146 | `0–7` | `1..8` | Wave Key-Off Loop Start |
| 147 | `0–7` | `1..8` | Wave Key-Off Loop End |
| 148 | reserved | | |
| 149–156 | `0–127` | times `0..127`, levels `-64..+63` | Free Env (4 stages with separate Release Time/Level) |
| 157 | `0–2` | normal, single, retrigger | Free Env Trigger |
| 158 | reserved | | |
| 159 | `0–127` | `0..127` or notation | LFO 1 Rate |
| 160 | `0–5` | sin, tri, sqr, saw, rnd, S&H | LFO 1 Shape |
| 161 | `0–127` | `0..127` | LFO 1 Delay |
| 162 | `0–3` | off, on, on, clock | LFO 1 Sync |
| 163 | `0–127` | `-64..+63` | LFO 1 Symmetry |
| 164 | `0–127` | `0..127` | LFO 1 Humanize |
| 165 | reserved | | |
| 166 | `0–127` | `0..127` or notation | LFO 2 Rate |
| 167 | `0–5` | sin, tri, sqr, saw, rnd, S&H | LFO 2 Shape |
| 168 | `0–127` | `0..127` | LFO 2 Delay |
| 169 | `0–3` | off, on, on, clock | LFO 2 Sync |
| 170 | `0–127` | `-64..+63` | LFO 2 Symmetry |
| 171 | `0–127` | `0..127` | LFO 2 Humanize |
| 172 | `0–127` | free, 3–357 deg | LFO 2 Phase |
| 173 | reserved | | |
| 174 | `0–31` | [mod sources](#modulation-sources-3-12) | Modifier Delay Source |
| 175 | `0–127` | `0..127` | Modifier Delay Time |
| 176–179 | | | **Modifier 1**: Source 1 (176), Source 2 (177), Type (178, [modifiers](#modifiers-3-14)), Parameter (179) |
| 180–183 | | | **Modifier 2** |
| 184–187 | | | **Modifier 3** |
| 188–191 | | | **Modifier 4** (Waldorf §3.1 mislabels these as "Modifier 3" — see [Divergences](#divergences-from-the-waldorf-doc)) |
| 192–239 | | | **Modulation slots 1–16**: each slot is 3 bytes — Source (`0–31`), Amount (`0–127`, displayed `-64..+63`), Destination (`0–35`, [mod destinations](#modulation-destinations-3-13); Waldorf §3.1 incorrectly caps at `0–33`). Slot N starts at index `192 + (N-1)*3`. |
| 240–255 | `32–127` ASCII | | Patch Name (16 characters) |

### MDATA — Multi Data (32 bytes)

| Idx | Range | Display | Parameter |
|---|---|---|---|
| 0 | `0–127` | `0..127` | Multi Volume |
| 1 | `0–121` | `0..120, global` | Control W |
| 2 | `0–121` | `0..120, global` | Control X |
| 3 | `0–121` | `0..120, global` | Control Y |
| 4 | `0–121` | `0..120, global` | Control Z |
| 5 | `1–127` | extern, 50–300 BPM | Arpeggiator Tempo |
| 6–15 | reserved | | |
| 16–31 | `32–127` ASCII | | Multi Name (16 characters) |

### IDATA — Instrument Data (28 bytes per instrument; 8 instruments per Multi)

| Idx | Range | Display | Parameter |
|---|---|---|---|
| 0 | `0–1` | A / B | Sound Bank |
| 1 | `0–127` | `1..128` | Sound Number |
| 2 | `0–17` | global, omni, 1–16 | MIDI Channel |
| 3 | `0–127` | `0..127` | Volume |
| 4 | `16–112` | `-48..+48` | Transpose |
| 5 | `0–127` | `-64..+63` | Detune |
| 6 | `0–1` | Main Out / Sub Out | Output |
| 7 | `0–1` | off / on | Status |
| 8 | `0–127` | left 64–center–right 63 | Panning |
| 9 | `0–2` | off / on / inverse | Pan Mod |
| 10–11 | reserved | | |
| 12 | `1–127` | `1..127` | Lowest Velocity |
| 13 | `1–127` | `1..127` | Highest Velocity |
| 14 | `0–127` | `0..127` | Lowest Key |
| 15 | `0–127` | `0..127` | Highest Key |
| 16 | `0–2` | off / on / hold / Sound Arp | Arpeggiator Active |
| 17 | `0–15` | `1/1..1/32` | Arpeggiator Clock |
| 18 | `1–10` | `1..10` | Arpeggiator Range |
| 19 | `0–16` | off, user, 1–15 | Arpeggiator Pattern |
| 20 | `0–3` | up, down, alt, random | Arpeggiator Direction |
| 21 | `0–3` | note, note-rev, played, played-rev | Arpeggiator Note Order |
| 22 | `0–1` | root note / last note | Arpeggiator Velocity |
| 23 | `0–1` | off / on | Arpeggiator Reset on Pattern Start |
| 24 | `0–18` | off / Ch 1–16 / Inst / global | Arpeggiator Notes Out |
| 25–27 | reserved | | |

**Total IDATA size: 28 bytes per instrument. ×8 instruments = 224 bytes. Combined with 32 MDATA = 256 bytes per MULD payload — matches Waldorf §2.22.** (This resolves D-04: IDATA byte count is confirmed as 28.)

### GDATA — Global Parameters (32 bytes)

The Waldorf doc warns "Global Parameters are very unordered" and that earlier doc revisions had wrong indices. The 2.16 indices below are confirmed.

| Idx | Range | Display | Parameter |
|---|---|---|---|
| 0 | reserved | | |
| 1 | `1` | `1` | GDATA format version |
| 2 | `0–2` | A / B / Multi | Startup Soundbank (2 = Multi mode) |
| 3 | `0–127` | `1..128` | Startup Sound Number |
| 4 | `1–17` | omni, 1–16 | MIDI Channel |
| 5 | `0–2` | sound / multi / combined | Program Change Mode |
| 6 | `0–126` | `0..126` | Device ID (the DEV byte) |
| 7 | `0–121` | `0..120, harmonic` | Bend Range |
| 8 | `0–120` | `0..120` | Controller W |
| 9 | `0–120` | `0..120` | Controller X |
| 10 | `0–120` | `0..120` | Controller Y |
| 11 | `0–120` | `0..120` | Controller Z |
| 12 | `0–127` | `0..127` | Main Volume |
| 13–14 | reserved | | |
| 15 | `52–76` | `-12..+12` | Transpose |
| 16 | `54–74` | 430 Hz–450 Hz | Master Tune |
| 17 | `0–127` | `0..127` | Display Timeout |
| 18 | `0–127` | `0..127` | LCD Contrast |
| 19–22 | reserved | | |
| 23 | `0–127` | `1..128` | Startup Multi Number |
| 24 | `0–16` | off / Ch 1–16 | Arpeggiator Note-Out Channel |
| 25 | `0–1` | off / on | MIDI Clock Output |
| 26 | `0–3` | off / Ctl / SysEx / Ctl+SysEx | Parameter Send |
| 27 | `0–1` | off / on | Parameter Receive |
| 28 | `0–3` | `1..4` | Input Gain **(XT only)** |
| 29–31 | reserved | | |

---

## Wave codecs

### WDATA — Wave Data (Waldorf §3.4)

One wave is **64 samples of 8-bit data**, encoded as **128 nibbles (one nibble per byte)**:

```
Byte 2n     = sample n, most significant nibble (in upper 4 bits: 0x00..0xF0)
Byte 2n+1   = sample n, least significant nibble (in lower 4 bits: 0x00..0x0F)
```

To reconstruct sample `n`:

```
uint8_t encoded = (byte[2n] & 0xF0) | (byte[2n+1] & 0x0F);
int8_t  sample  = (int8_t)(encoded ^ 0x80);   // XOR-flip to signed
```

The XT stores only the first 64 samples; the back half is mirrored:

```
Wave[64+n] = -Wave[63-n]   for n = 0..63
```

This is **not two's-complement** — the XOR with `0x80` is mandatory. Skipping it produces an audible DC offset and wrong wave shape (off-by-one octave isn't this; this is just wrong).

### WCTDATA — Wave Control Table Data (Waldorf §3.5)

A wavetable is **64 entries**, each entry being a 12-bit wave-index value (range 0–200 for ROM waves, 1000–1249 for user waves). Each entry is encoded as **4 nibbles** (one nibble per byte → 256 bytes total):

```
Byte 4n     = entry n, MSN of upper half
Byte 4n+1   = entry n, LSN of upper half
Byte 4n+2   = entry n, MSN of lower half
Byte 4n+3   = entry n, LSN of lower half
```

To reconstruct entry `n`:

```
uint16_t entry = ((b[4n]   & 0xF0) << 8)
               | ((b[4n+1] & 0x0F) << 8)
               | ((b[4n+2] & 0xF0))
               | ((b[4n+3] & 0x0F));
```

Constraints (Waldorf §3.5):
- Indices not in `{0..200} ∪ {1000..1249}` cause spectral interpolation between neighbouring valid entries
- The **last three entries are always triangle, square, sawtooth** — hardwired
- The **first entry must be valid** (cannot be interpolated)
- Only **wavetables 96–128 are user-writable**; uploading WCTD to 1–95 silently fails

---

## Lookup tables

### Play Parameters (§3.11)

The four Play Parameter slots (SDATA 58–61) hold an index into this table. The XT uses them for its four assignable front-panel encoders.

| Value | SDATA idx | Parameter |
|---|---|---|
| 0–4 | 1, 2, 3, 5, 6 | Osc 1: Octave / Semi / Detune / PB / Keytrack |
| 5–9 | 12, 13, 14, 17, 18 | Osc 2: Octave / Semi / Detune / PB / Keytrack |
| 10 | 25 | Wavetable |
| 11–15 | 26, 27, 28, 29, 30 | Wave 1: Startwave / Phase / Env / Velo / Keytrack |
| 16–20 | 36, 37, 38, 39, 40 | Wave 2: Startwave / Phase / Env / Velo / Keytrack |
| 21–24 | 47, 48, 49, 50 | Mix: Wave1 / Wave2 / RingMod / Noise |
| 25–27 | 53, 54, 55 | Aliasing / Quantize / Clipping |
| 28–33 | 62–67 | Filter 1: Cutoff / Reso / Type / Keytrack / Env / Velo |
| 34–36 | 73, 74, 75 | Filter 2: Cutoff / Type / Keytrack |
| 37 | 77 | Sound Volume |
| 38–39 | 79, 80 | Amp Env Velo / Amp Keytrack |
| 40 | 81 | Chorus |
| 41–42 | 84, 85 | Panning / Pan Keytrack |
| 43–44 | 87, 88 | Glide on/off / Type |
| 45–52 | 92–99 | Arpeggiator: Active / Tempo / Clock / Range / Pattern / Direction / Note Order / Velocity |
| 53–54 | 108, 109 | Allocation / Assignment |
| 55–58 | 113–116 | Filter Env: A / D / S / R |
| 59–62 | 119–122 | Amp Env: A / D / S / R |
| 63–68 | 159–164 | LFO 1: Rate / Shape / Delay / Sync / Symmetry / Humanize |
| 69–75 | 166–172 | LFO 2: Rate / Shape / Delay / Sync / Symmetry / Humanize / Phase |
| 76 | 7 | Osc 1 FM Amount |
| 77 | 70 | Filter 1 Special |
| 78 | 90 | Glide Time |
| 79 | — | Control W |
| 80 | — | Control X |
| 81 | — | Control Y |
| 82 | — | Control Z |

### Modulation Sources (§3.12)

| Idx | Source |
|---|---|
| 0 | off |
| 1 | LFO 1 |
| 2 | LFO 1 × Modwheel |
| 3 | LFO 1 × Aftertouch |
| 4 | LFO 2 |
| 5 | Filter Envelope |
| 6 | Amplifier Envelope |
| 7 | Wave Envelope |
| 8 | Free Envelope |
| 9 | Key Follow |
| 10 | Keytrack |
| 11 | Velocity |
| 12 | Release Velocity |
| 13 | Aftertouch |
| 14 | Poly Pressure |
| 15 | Pitch Bend |
| 16 | Modwheel |
| 17 | Sustain Control |
| 18 | Foot Control |
| 19 | Breath Control |
| 20 | Control W |
| 21 | Control X |
| 22 | Control Y |
| 23 | Control Z |
| 24 | Control Delay |
| 25 | Modifier #1 |
| 26 | Modifier #2 |
| 27 | Modifier #3 |
| 28 | Modifier #4 |
| 29 | MIDI Clock |
| 30 | Minimum |
| 31 | Maximum |

**Total: 32 sources** (0–31). The original dev plan said 31 sources — the count is 32 with index 0 = "off". Code grids should size for 32.

### Modulation Destinations (§3.13)

| Idx | Destination |
|---|---|
| 0 | Pitch |
| 1 | Osc 1 Pitch |
| 2 | Osc 2 Pitch |
| 3 | Wave 1 Pos |
| 4 | Wave 2 Pos |
| 5 | Mix Wave 1 |
| 6 | Mix Wave 2 |
| 7 | Mix Ringmod |
| 8 | Mix Noise |
| 9 | Filter 1 Cutoff |
| 10 | Filter 1 Resonance |
| 11 | Filter 2 Cutoff |
| 12 | Volume |
| 13 | Panning |
| 14 | Filter Env Attack |
| 15 | Filter Env Decay |
| 16 | Filter Env Sustain |
| 17 | Filter Env Release |
| 18 | Amp Env Attack |
| 19 | Amp Env Decay |
| 20 | Amp Env Sustain |
| 21 | Amp Env Release |
| 22 | Wave Envelope Times |
| 23 | Wave Envelope Levels |
| 24 | Free Envelope Times |
| 25 | Free Envelope Levels |
| 26 | LFO 1 Rate |
| 27 | LFO 1 Level |
| 28 | LFO 2 Rate |
| 29 | LFO 2 Level |
| 30 | Mod #1 Amount |
| 31 | Mod #2 Amount |
| 32 | Mod #3 Amount |
| 33 | Mod #4 Amount |
| 34 | FM Amount |
| 35 | F1 Extra (Wave / BP offset / Osc 2 FM / S&H rate, depending on Filter 1 Type) |

**Total: 36 destinations** (0–35). Note that Waldorf §3.1 SDATA layout lists `0–33` for the per-slot destination byte while §3.13 itself enumerates 36 entries — see [Divergences](#divergences-from-the-waldorf-doc). The valid input range is `0–35`; the SDATA layout in this document records the per-slot destination byte range as `0–35` accordingly.

### Modifiers (§3.14)

| Idx | Operation |
|---|---|
| 0 | + (Addition) |
| 1 | − (Subtraction) |
| 2 | × (Multiplication) |
| 3 | ÷ (Division) |
| 4 | XOR |
| 5 | OR |
| 6 | AND |
| 7 | S&H (Sample & Hold) |
| 8 | Ramp |
| 9 | Switch |
| 10 | Abs value |
| 11 | Min value |
| 12 | Max value |
| 13 | Lag processor |
| 14 | Control filter |
| 15 | Differentiator |

**Total: 16 operations.**

### Filter 1 Types (§3.15)

| Idx | Type | Special-Param meaning |
|---|---|---|
| 0 | 24 dB Lowpass | (unused / fixed) |
| 1 | 12 dB Lowpass | (unused / fixed) |
| 2 | 24 dB Bandpass | (unused / fixed) |
| 3 | 12 dB Bandpass | BP offset |
| 4 | 12 dB Highpass | (unused / fixed) |
| 5 | Sine Waveshaper → 12 dB LP | Waveshape drive |
| 6 | 12 dB LP → Waveshaper | Waveshape drive |
| 7 | Dual 12 dB Low/Bandpass parallel | Wave / channel balance |
| 8 | 12 dB Lowpass FM-Filter | Osc 2 FM amount |
| 9 | 12 dB Lowpass with Sample & Hold | S&H Rate |
| 10 | 24 dB Notch — *gearmulator-only?* | TBD |
| 11 | 12 dB Notch — *gearmulator-only?* | TBD |
| 12 | 12 dB Band Stop — *gearmulator-only?* | TBD |

**Filter type extension (M0.3 finding):** Indices 10–12 appear in gearmulator's `parameterDescriptions_xt.json` but are *not* documented in Waldorf §3.15 (release 2.16). Either they're late firmware additions or Xenia emulator-only extensions. Verify on real XT during M1.5 — clamp `F1Type` to 0–9 if hardware doesn't recognize them.

#### Filter Type Context

The Filter 1 Special Parameter (SDATA index 70) label depends on the selected Filter 1 Type (SDATA index 64). The mapping is **not in the Waldorf doc** and must be confirmed against the hardware LCD during M1.5. The labels above (right column) are the working list, taken from Waldorf §3.13 modulation destination 35 hint ("F1 Extra (Wave/BP offset/Osc2 FM/S&H Rate)") and Edisyn cross-check. **Open: confirm against XT LCD what types 0–4 display as the Special Param label.**

---

## Device Inquiry

The XT responds to standard Universal Device Inquiry:

```
TX: F0 7E <channel> 06 01 F7
```

Where `<channel>` is either the XT's configured Device ID (GDATA index 6) or `7Fh` for broadcast. Reply:

```
RX: F0 7E 06 02      Universal Device Inquiry response header
    3E               Waldorf manufacturer ID
    0E 00            Device family code (low, high) — Microwave II
    XX YY            Device family member code (see below)
    V1 V2 V3 V4      Software revision in ASCII (e.g. "2", ".", "0", "9" for v2.09)
    F7               EOX
```

**Device family member codes** (since firmware 2.16, encoded as bitmasks so combinations exist):

| XX | YY | Hardware |
|---|---|---|
| `00` | `00` | Microwave 2 (base model) |
| `01` | `00` | Microwave 2 with XT mainboard (delay effects available) |
| `03` | `00` | Microwave XT |
| `05` | `00` | Microwave PC on Terratec EWS Frontmodule |
| `09` | `00` | MW2/XT with expandable mainboard, 10 voices |
| `19` | `00` | Expanded MW2/XT, 30 voices |

Bitmask meanings (combine via OR):

| Bit | Meaning |
|---|---|
| `01` | Mainboard 2.0 |
| `02` | XT Frontboard |
| `04` | MWPC (Microwave PC) |
| `08` | Expandable mainboard |
| `10` | Voice expansion (additional voices fitted) |

So `09h = 01 OR 08` = mainboard 2.0 + expandable. `19h = 01 OR 08 OR 10` = mainboard 2.0 + expandable + voice expansion.

---

## MIDI Control Change map

The XT transmits and receives MIDI CCs for the parameters listed below. CCs are the primary mechanism for the 44 hardware-knob controls. Numbers are 7-bit MIDI CCs (`B0 nn vv`); values map per the "Value Range" column.

Source: Waldorf controls PDF (release 2.09).

| CC | Range | Parameter | Value semantics |
|---|---|---|---|
| 1 | 0–127 | Modulation Wheel MSB | |
| 2 | 0–127 | Breath Control MSB | |
| 4 | 0–127 | Foot Controller MSB | |
| 5 | 0–127 | Glide Time | |
| 7 | 0–127 | Channel Volume | |
| 10 | 0–127 | Panning | 64=center; 0=left, 127=right |
| 12 | 0–1 | Chorus | 0=off, 1=on |
| 14 | 0–127 | Filter Env Attack | |
| 15 | 0–127 | Filter Env Decay | |
| 16 | 0–127 | Filter Env Sustain | |
| 17 | 0–127 | Filter Env Release | |
| 18 | 0–127 | Amp Env Attack | |
| 19 | 0–127 | Amp Env Decay | |
| 20 | 0–127 | Amp Env Sustain | |
| 21 | 0–127 | Amp Env Release | |
| 22 | 0–3 | Glide Type | 0=porta 1=finger-porta 2=gliss 3=finger-gliss |
| 23 | 0–1 | Glide Mode | 0=exp 1=linear |
| 24 | 0–127 | LFO 1 Rate | |
| 25 | 0–5 | LFO 1 Shape | 0=sin 1=tri 2=sqr 3=saw 4=rnd 5=S&H |
| 26 | 0–127 | LFO 2 Rate | |
| 27 | 0–127 | LFO 2 Delay | 0=off, 1=retrigger, 2–127=1–126 |
| 28 | 0–5 | LFO 2 Shape | same as LFO 1 |
| 29 | 0–2 | Filter Env Trigger | 0=normal 1=single 2=retrigger |
| 30 | 0–127 | LFO 1 Delay | 0=off, 1=retrigger, 2–127=1–126 |
| 31 | 0–2 | Amp Env Trigger | 0=normal 1=single 2=retrigger |
| 32 | 0–1 | Bank Select LSB | 0=Bank A 1=Bank B |
| 33 | 16–112 | Osc 1 Octave | −4 to +4, steps of 12 |
| 34 | 56–76 | Osc 1 Semitone | −12 to +12 |
| 35 | 0–127 | Osc 1 Detune | −64 to +63 |
| 36 | 0–121 | Osc 1 Pitchbend Scale | 0–120=semitones, 121=harmonic, 122=global |
| 37 | 0–127 | Osc 1 Keytrack | −100% to +200% |
| 38 | 16–112 | Osc 2 Octave | |
| 39 | 56–76 | Osc 2 Semitone | |
| 40 | 0–127 | Osc 2 Detune | |
| 41 | 0–1 | Osc 2 Sync | |
| 42 | 0–121 | Osc 2 Pitchbend Scale | |
| 43 | 0–127 | Osc 2 Keytrack | |
| 44 | 0–1 | Osc 2 Link | |
| 45 | 0–127 | Wave 1 Level | |
| 46 | 0–127 | Wave 2 Level | |
| 47 | 0–127 | RingMod Level | |
| 48 | 0–127 | Noise Level | |
| 50 | 0–127 | Filter 1 Cutoff | |
| 51 | 0–127 | Filter 1 Keytrack | −200% to +197% |
| 52 | 0–127 | Filter 1 Env Amount | −64 to +63 |
| 53 | 0–127 | Filter 1 Env Velocity | −64 to +63 |
| 54 | 0–9 | Filter 1 Type | per §3.15 |
| 55 | 0–127 | Amp Keytrack | |
| 56 | 0–127 | Filter 1 Resonance | |
| 57 | 0–127 | Amp Volume | |
| 58 | 0–127 | Amp Env Velocity | |
| 60 | 0–127 | Filter 2 Cutoff | |
| 61 | 0–1 | Filter 2 Type | 0=6dB LP 1=6dB HP |
| 62 | 0–127 | Filter 2 Keytrack | |
| 64 | 0–127 | Sustain | |
| 65 | 0–127 | Portamento Switch | |
| 66 | 0–127 | Sostenuto | |
| 70 | 0–127 | Wavetable | wavetable 001–128 |
| 71 | 0–63 | Wave 1 Startwave | |
| 72 | 0–127 | Wave 1 Phase | |
| 73 | 0–127 | Wave 1 Env Amount | |
| 74 | 0–127 | Wave 1 Env Velocity | |
| 75 | 0–127 | Wave 1 Keytrack | |
| 76 | 0–1 | Wave 1 Limit | |
| 77 | 0–63 | Wave 2 Startwave | |
| 78 | 0–127 | Wave 2 Phase | |
| 79 | 0–127 | Wave 2 Env Amount | |
| 80 | 0–127 | Wave 2 Env Velocity | |
| 81 | 0–127 | Wave 2 Keytrack | |
| 82 | 0–1 | Wave 2 Limit | |
| 83 | 0–1 | Wave 2 Link | |
| 85 | 0–127 | Free Env Time 1 | |
| 86 | 0–127 | Free Env Level 1 | |
| 87 | 0–127 | Free Env Time 2 | |
| 88 | 0–127 | Free Env Level 2 | |
| 89 | 0–127 | Free Env Time 3 | |
| 90 | 0–127 | Free Env Level 3 | |
| 91 | 0–127 | Free Env Release Time | |
| 92 | 0–127 | Free Env Release Level | |
| 93 | 0–2 | Free Env Trigger | |
| 102 | 0–2 | Arp Active | |
| 103 | 0–9 | Arp Range | 1–10 octaves |
| 104 | 0–15 | Arp Clock | |
| 105 | 0–127 | Arp Tempo | 0=external, 1–127=50–300 BPM |
| 106 | 0–3 | Arp Direction | |
| 107 | 0–16 | Arp Pattern | |
| 108 | 0–3 | Arp Note Order | |
| 109 | 0–1 | Arp Velocity | |
| 110 | 0–1 | Arp Reset | |
| 111 | 0–15 | Arp Pattern Length | |
| 112 | 0–1 | LFO 1 Sync | |
| 113 | 0–127 | LFO 1 Symmetry | |
| 114 | 0–127 | LFO 1 Humanize | |
| 115 | 0–1 | LFO 2 Sync | |
| 116 | 0–127 | LFO 2 Symmetry | |
| 117 | 0–127 | LFO 2 Humanize | |
| 118 | 0–127 | LFO 2 Phase | |
| 120 | 0 | All Sound Off | standard MIDI Channel Mode |
| 121 | 0 | Reset All Controllers | standard MIDI Channel Mode |
| 123 | 0 | All Notes Off | standard MIDI Channel Mode |

In **SysEx send mode** (GDATA 26 = SysEx or Ctl+SysEx) the same parameter changes additionally arrive as SNDP messages. Editor receive-routing must dedupe when both modes are active.

---

## Divergences from the Waldorf doc

These are catalogued because they will bite anyone reading the Waldorf PDF directly. Every divergence below should round-trip correctly through our protocol layer with the corrected value.

| Where in Waldorf doc | What it says | What's correct | Evidence |
|---|---|---|---|
| §1.1 IDM list | `GLBR 14h` and `GLBD 14h` (both 14h) | `GLBR = 04h`, `GLBD = 14h` | §1.1 matrix shows `04` and `14` separately; §2.51 frame uses `04h`; §2.52 frame uses `14h`. Edisyn confirms. |
| §2.21 title | `MULR 11h` | `MULR = 01h` | §1.1 matrix; §2.21 frame body uses `01h`. |
| §2.22 title | `MULD 21h` | `MULD = 11h` | §1.1 matrix; §2.22 frame body uses `11h`. |
| §2.23 title | `MULP 20h` | `MULP = 21h` | §2.23 frame body says `21h`; §1.1 matrix omits MULP entirely. **Edisyn confirms 21h** (and the dev plan flags this as the canonical fix). |
| §1.1 IDM matrix | MULx row shows only `01 11` | MULP exists at `21h` (Parameter Change column) | §2.23 documents MULP; matrix is incomplete. |
| §2.42 title | `WAVD 13h Wave ControlDump` | `WCTD = 13h` | Section is in fact about wave control table dump; title typo. |
| §2.51 title | `WCTR 04h Global Parameter Request` | `GLBR = 04h` | Section is in fact about global parameter request; title typo. |
| §2.62 title | `DISR 15h Display Dump` | `DISD = 15h` | Section is in fact about display dump; title typo. |
| §2.31 location table | `User Waves 1024..10151` | `User Waves 1024..1151` | "10151" is an obvious typo; second character `1` should not be there. Same typo in §2.32. |
| §3.1 SDATA indices 188–191 | Labelled "Modifier 3 Source 1", "Source 2", "Type", "Parameter" (duplicating Modifier 3) | These are **Modifier 4** | Indices 176–179 = Mod 1; 180–183 = Mod 2; 184–187 = Mod 3; 188–191 = Mod 4. |
| §3.1 SDATA index 76 | `0-7[MW2] 0-35[XT]` | Range differs by hardware; XT has 36 effect types, MW2 has 8 | The hardware comment is correct; we must dispatch by device family code on parameter reads. |
| §3.1 SDATA index 51 | `Mix External [XT only]` | XT-only — MW2 ignores this index | Test against MW2 hardware before assuming write-through. |
| §3.6 GDATA preamble | "All indices were wrong in previous documentations" | 2.16 indices listed are now correct | Don't trust pre-2.16 docs. |
| §1.1 IDM matrix says "MULx x1 Multi" with no Parameter Change column | MULP exists at IDM `21h` | The matrix omits it. | See above; same divergence. |
| §3.1 SDATA per-slot destination byte | Range `0–33` | Range `0–35` | §3.13 enumerates 36 destinations (indices 0–35). §3.1's `0–33` cap predates the addition of destinations 34 (FM Amount) and 35 (F1 Extra). |

---

## Implementation notes

### SNDP/MULP/GLBP rate limiting

Per the dev plan and gearmulator's BUG-10134 fix: the XT firmware will be overwhelmed by uncoalesced rapid parameter changes (UI knob drag, DAW automation). Implementation:

- One coalescing queue **per parameter index**, not one global queue
- Within a 100 ms window per parameter, keep only the most recent value; send when the window closes
- If a different parameter changes during the window, both are sent independently — only intra-parameter spam is coalesced
- Bypassing the rate limiter is a defect (NFR-2 in [editor-requirements.md](editor-requirements.md))

### Parameter index HH byte

SNDP `HH` is `00h` for SDATA 0–127 and `01h` for 128–255. Writing to index 200 with `HH = 00h` writes to index 72 (Filter 2 Cutoff) instead of 200 (Mod 3 Source) — both are valid indices, so the bug is silent. Cover with unit tests and a static assert that the encoder branches on `index >= 128`.

### MULP location ambiguity

Waldorf §2.23 says `LL = 20h` targets the Multi edit buffer and `LL = 01h..07h` targets instrument 1–8 buffers. That's 7 values for 8 instruments. Edisyn cross-check needed to know what `LL = 00h` means (probably instrument 8 to make it 0–7; verify before coding).

### Reserved field preservation

SDATA, MDATA, IDATA, GDATA all have "reserved" slots. The spec says set to 0 for future compatibility — but if a future firmware uses them, we must not zero them out on round-trip. Read-write paths through the editor preserve the bytes; only **fresh-patch** initialization writes zeros.

### MW2 vs XT differences (parameter level)

The editor must know which hardware family it's talking to (via Device Inquiry) and adjust:

- SDATA 51 (Mix External): write-through XT only; ignored on MW2
- SDATA 76 (Effect Type): 0–7 on MW2, 0–35 on XT — clamp accordingly
- GDATA 28 (Input Gain): XT only
- Member codes with bit `02` (XT Frontboard) indicate the unit has the 44 knobs and full CC behaviour

### Universal `7Fh` checksum

Decoders should accept `XSUM = 7Fh` as valid regardless of actual data. Useful for hand-written test messages and as a sentinel during debugging. Encoders should never emit `7Fh` unless the computed sum genuinely equals `7Fh`.

### DISP / DISL — display poke messages

`DISP` (write a single character to the LCD) and `DISL` (recall stock LCD) are documented in Waldorf §2.63 / §2.64. The editor does not use either. Reception is harmless; the LCD mirror panel can ignore them or treat them as cues to re-request via DISR.

### INFR / INFD

Waldorf §2.82 / §2.83 describe Information request/dump messages used only by the Microwave PC on Terratec EWS hardware. The editor does not target MWPC; these messages are ignored.

---

## Open items

These are items that need confirmation against real hardware during the M1.2 / M1.3 milestones. They're tracked here, not in ROADMAP, because they affect protocol correctness rather than scheduling.

- [ ] Filter 1 Type indices 10–12 (24 dB Notch / 12 dB Notch / 12 dB Band Stop): real firmware feature or Xenia emulator-only? Cross-check `F1Type` writes to the real XT and observe LCD readout.
- [ ] MULP `LL = 00h` semantics — instrument 8, error, or ignored?
- [ ] Filter 1 Special Parameter label-per-type for filter types 0–4 (currently marked "fixed/unused" in §Filter Type Context but not confirmed against the XT LCD)
- [ ] LFO Sync (SDATA 162/169) values 1 and 2 both display as "on" per Waldorf doc — verify they're not actually subtly different on hardware
- [ ] GDATA index 16 Master Tune range `54–74` → 430–450 Hz: the centre point is 64 = 440 Hz, but verify on hardware that mapping is linear in Hz vs cents
- [ ] Whether Xenia's SysEx timing is faithful enough to count as a rate-limiter test bed (resolves [ROADMAP D-03](../../ROADMAP.md))
