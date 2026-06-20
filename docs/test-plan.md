# MW2/XT Editor — M1.3 Verification Plan

Manual sign-off checklist for **M1.3 — HardwareMidiDevice + dual test harness**. This plan covers the two remaining exit criteria that require live hardware or IAC-bus observation:

1. Every message type frames byte-identical to Edisyn on the IAC bus (MIDI monitor verified)
2. Autodetect returns correct family/device ID against real XT

See [`ROADMAP.md`](../ROADMAP.md) §M1.3 for gate status and [`docs/spec/sysex-protocol.md`](spec/sysex-protocol.md) for wire-format reference.

---

## Prerequisites

| Item | Notes |
|---|---|
| **MIDI Monitor.app** | Free, macOS. Watch IAC Bus 1 input. |
| **IAC Bus 1 enabled** | Audio MIDI Setup → MIDI Studio → IAC Driver → Device 1 → check "Device is online" |
| **Xenia** in AU Lab or Reaper | MIDI In = IAC Bus 1, MIDI Out = IAC Bus 1 |
| **Edisyn** with WaldorfMicrowaveXT editor | MIDI Out = IAC Bus 1, Device ID = 0 |
| **Editor Standalone (Debug build)** | MIDI Out = IAC Bus 1, MIDI In = IAC Bus 1 |
| **Real XT** (Part C only) | USB-MIDI connected, default Device ID = 0 |

IAC Bus routing: Editor MIDI Out → IAC Bus 1 → Xenia MIDI In; Xenia MIDI Out → IAC Bus 1 → Editor MIDI In. Both Edisyn and the editor connect to the same bus, so MIDI Monitor sees everything.

---

## Part A — Debug trigger

The editor has no parameter UI yet (M1.4). Add a temporary debug trigger to fire test messages so they appear in MIDI Monitor. Remove after verification passes.

In `source/mw2xtPlugin/PluginEditor.h`, declare (inside `#ifdef JUCE_DEBUG`):

```cpp
#ifdef JUCE_DEBUG
    void runMidiVerification();
#endif
```

In `source/mw2xtPlugin/PluginEditor.cpp`, implement `keyPressed` and `runMidiVerification`:

```cpp
bool EditorPluginEditor::keyPressed(const juce::KeyPress& k)
{
#ifdef JUCE_DEBUG
    if (k == juce::KeyPress('t', juce::ModifierKeys::commandModifier, 0)) {
        runMidiVerification();
        return true;
    }
#endif
    return false;
}

#ifdef JUCE_DEBUG
void EditorPluginEditor::runMidiVerification()
{
    using namespace mw2xt;
    // Open a fresh device directly from the editor for testing.
    // Replace "IAC Driver Bus 1" with the exact port name shown in MIDI Monitor.
    HardwareMidiDevice dev;
    if (!dev.open("IAC Driver Bus 1", "IAC Driver Bus 1")) {
        DBG("runMidiVerification: could not open IAC port");
        return;
    }
    dev.setDeviceId(0x00);

    auto pause = []{ juce::Thread::sleep(50); };

    // ── SNDP ──────────────────────────────────────────────────────────────
    dev.sendSndp(0x00,   0, 64);   pause();  // idx=0,   HH=00 PP=00 XX=40
    dev.sendSndp(0x00, 127, 127);  pause();  // idx=127, HH=00 PP=7F XX=7F
    dev.sendSndp(0x00, 128,   0);  pause();  // idx=128, HH=01 PP=00 XX=00  ← HH split
    dev.sendSndp(0x00, 255, 100);  pause();  // idx=255, HH=01 PP=7F XX=64

    // ── SNDR ──────────────────────────────────────────────────────────────
    dev.sendSndr(0x20, 0x00);  pause();  // edit buffer
    dev.sendSndr(0x00, 0x00);  pause();  // Bank A slot 0
    dev.sendSndr(0x10, 0x00);  pause();  // All Sounds

    // ── MULP / MULR ───────────────────────────────────────────────────────
    dev.sendMulp(0x20, 0, 64);   pause();  // IDM must be 21h
    dev.sendMulr(0x20, 0x00);    pause();

    // ── GLBx ──────────────────────────────────────────────────────────────
    dev.sendGlbp(0, 64);  pause();  // 8 bytes, no checksum
    dev.sendGlbr();       pause();

    // ── WAVx / WCTx ───────────────────────────────────────────────────────
    dev.sendWavr(0x07, 0x68);  pause();  // user wave 1000
    dev.sendWctr(0x00, 0x60);  pause();

    // ── DISR / RMTP / MODR ───────────────────────────────────────────────
    dev.sendDisr();             pause();
    dev.sendRmtp(0x06, 0x01);  pause();  // Store button press
    dev.sendModr();             pause();

    // ── SNDD (TX) ─────────────────────────────────────────────────────────
    SoundData sd;
    sd[62] = 64; sd[63] = 32;   // Filter 1 Cutoff=64, Resonance=32
    dev.sendSndd(0x20, 0x00, sd);  pause();  // edit buffer SNDD — 265 bytes
}
#endif
```

Trigger: **Cmd-T** in the Standalone window. All frames appear in MIDI Monitor in sequence.

---

## Part B — Expected bytes (Device ID = 0x00)

Run the debug trigger and verify each frame in MIDI Monitor. The "Expected" column is authoritative — it is derived directly from `Protocol.h` with the test inputs above and can be checked without Edisyn.

### SNDP — Sound Parameter Change (10 bytes, no checksum)

`F0 3E 0E DEV 20 LL HH PP XX F7`

| Call | Expected | Key invariant |
|---|---|---|
| `sendSndp(0x00, 0, 64)` | `F0 3E 0E 00 20 00 00 00 40 F7` | HH=00 for idx 0–127 |
| `sendSndp(0x00, 127, 127)` | `F0 3E 0E 00 20 00 00 7F 7F F7` | |
| `sendSndp(0x00, 128, 0)` | `F0 3E 0E 00 20 00 01 00 00 F7` | HH=01 for idx 128–255 |
| `sendSndp(0x00, 255, 100)` | `F0 3E 0E 00 20 00 01 7F 64 F7` | PP = idx & 0x7F |

**Critical:** if idx=128 produces HH=00, the HH-split bug is present. This is the test from `sysex-protocol.md` §SNDP ("silent-wrong-parameter bug").

### SNDR — Sound Request (9 bytes)

`F0 3E 0E DEV 00 BB NN ((BB+NN)&7F) F7`

| Call | Expected |
|---|---|
| `sendSndr(0x20, 0x00)` | `F0 3E 0E 00 00 20 00 20 F7` |
| `sendSndr(0x00, 0x00)` | `F0 3E 0E 00 00 00 00 00 F7` |
| `sendSndr(0x10, 0x00)` | `F0 3E 0E 00 00 10 00 10 F7` |

### MULP — Multi Parameter Change (9 bytes, no checksum)

`F0 3E 0E DEV 21 LL PP XX F7`

| Call | Expected | Key invariant |
|---|---|---|
| `sendMulp(0x20, 0, 64)` | `F0 3E 0E 00 21 20 00 40 F7` | IDM = **21h** not 20h |

### MULR — Multi Request (9 bytes)

| Call | Expected |
|---|---|
| `sendMulr(0x20, 0x00)` | `F0 3E 0E 00 01 20 00 20 F7` |

### GLBP — Global Parameter Change (8 bytes, no checksum)

`F0 3E 0E DEV 24 PP XX F7`

| Call | Expected | Key invariant |
|---|---|---|
| `sendGlbp(0, 64)` | `F0 3E 0E 00 24 00 40 F7` | 8 bytes — no XSUM before F7 |

### GLBR — Global Request (7 bytes)

| Call | Expected |
|---|---|
| `sendGlbr()` | `F0 3E 0E 00 04 00 F7` |

### WAVR — Wave Request (9 bytes)

`F0 3E 0E DEV 02 HH LL ((HH+LL)&7F) F7`

| Call | Expected |
|---|---|
| `sendWavr(0x07, 0x68)` | `F0 3E 0E 00 02 07 68 6F F7` (XSUM = (07h+68h)&7Fh = 6Fh) |

### WCTR — Wave Control Table Request (9 bytes)

| Call | Expected |
|---|---|
| `sendWctr(0x00, 0x60)` | `F0 3E 0E 00 03 00 60 60 F7` (XSUM = 60h) |

### DISR — Display Request (7 bytes)

| Call | Expected |
|---|---|
| `sendDisr()` | `F0 3E 0E 00 05 00 F7` |

### RMTP — Remote Control (9 bytes)

`F0 3E 0E DEV 26 UU MM ((UU+MM)&7F) F7`

| Call | Expected |
|---|---|
| `sendRmtp(0x06, 0x01)` | `F0 3E 0E 00 26 06 01 07 F7` (XSUM = (06h+01h)&7Fh = 07h) |

### MODR — Mode Request (7 bytes)

| Call | Expected |
|---|---|
| `sendModr()` | `F0 3E 0E 00 07 00 F7` |

### UDI Broadcast — autodetect TX (6 bytes)

| Call | Expected |
|---|---|
| `autodetect()` TX | `F0 7E 7F 06 01 F7` |

### SNDD — Sound Dump TX (265 bytes)

Too long for a full hex dump. Verify structure only:

| Position | Expected |
|---|---|
| bytes[0..4] | `F0 3E 0E 00 10` |
| bytes[5] | `20` (BB = edit buffer) |
| bytes[6] | `00` (NN) |
| bytes[7] | `00` (SDATA[0] = sound format, zero-init) |
| bytes[69] | `40` (SDATA[62] = Filter 1 Cutoff = 64, which we set) |
| bytes[70] | `20` (SDATA[63] = Filter 1 Resonance = 32) |
| bytes[263] | XSUM = `(Σ bytes[7..262]) & 0x7F` — compute and verify |
| bytes[264] | `F7` |

Total length in MIDI Monitor: **265 bytes**.

---

## Part C — Edisyn cross-check (spot checks only)

Run Edisyn (WaldorfMicrowaveXT editor, Device ID = 0, MIDI Out = IAC Bus 1). Trigger the operations below and compare MIDI Monitor output to the expected bytes in Part B. Edisyn is the hardware-tested reference — any disagreement between Edisyn and our output is a bug in our encoder.

| Edisyn operation | Edisyn UI gesture | Our equivalent | Bytes to compare |
|---|---|---|---|
| SNDP idx=62 (Filter Cutoff) value=64 | Drag Filter 1 Cutoff knob to 64 | `sendSndp(0x00, 62, 64)` → `F0 3E 0E 00 20 00 00 3E 40 F7` | Full 10 bytes |
| SNDP idx=128 (first hi-byte param) | Edit a parameter in the 128+ range | `sendSndp(0x00, 128, 0)` | Verify HH=01 in both |
| MULP IDM check | In Multi mode, edit any Multi parameter | `sendMulp(0x20, 0, 64)` | Confirm IDM byte = **21h** in both |
| GLBP no-checksum check | Edit a Global parameter | `sendGlbp(0, 64)` | Confirm 8 bytes, no XSUM |
| SNDR edit buffer | Edisyn → Request / fetch current patch | `sendSndr(0x20, 0x00)` | Full 9 bytes |

If Edisyn is unavailable for a row, fall back to the independently-computed expected bytes in Part B.

---

## Part D — Real XT autodetect sign-off

Switch the editor's MIDI port selection from IAC Bus 1 to the XT's USB-MIDI port. Trigger autodetect via the debug build or via a direct call in a test session.

**Expected TX** (visible in MIDI Monitor on the XT output):
```
F0 7E 7F 06 01 F7
```

**Expected RX** (XT reply — log the exact bytes):
```
F0 7E <devId> 06 02 3E 0E 00 <memberLow> <memberHigh> <V1> <V2> <V3> <V4> F7
```

| Field | Expected value | Notes |
|---|---|---|
| `devId` | XT's configured Device ID (GDATA 6; factory default = 0) | |
| `memberLow` | `03` for a standard Microwave XT | `01`=mainboard 2.0, `02`=XT frontboard; XT has both → `03` |
| `memberHigh` | `00` | With expandable/voice-expanded boards, `memberLow` will be higher (see sysex-protocol.md §Device Inquiry bitmask) |
| Firmware | e.g., `"2"`, `"."`, `"1"`, `"6"` | ASCII bytes V1–V4 |
| `DeviceInfo.valid` | `true` | |

Record the exact bytes returned and note them in the ROADMAP.md changelog. If `memberLow` differs from `03h`, cross-check against the bitmask table in `sysex-protocol.md` §Device Inquiry to confirm the correct hardware variant.

---

## Pass criteria

All items must be checked before marking M1.3 complete in ROADMAP.md.

- [x] SNDP HH=`00` for idx 0–127; HH=`01` for idx 128–255 — MIDI Monitor confirmed 2026-06-20
- [x] SNDP idx=128 byte sequence matches expected (`F0 3E 0E 00 20 00 01 00 00 F7`) — MIDI Monitor confirmed 2026-06-20 (Edisyn unavailable; independently-computed bytes used per Part C fallback)
- [x] MULP IDM = `21h` (not `20h`) — MIDI Monitor confirmed 2026-06-20
- [x] GLBP is 8 bytes with no checksum byte before `F7` — MIDI Monitor confirmed 2026-06-20
- [x] All other message types in Part B match expected bytes exactly — MIDI Monitor confirmed 2026-06-20
- [x] SNDD TX is 265 bytes; Filter param bytes at correct offsets (bytes[69]=`40`, bytes[70]=`20`) — MIDI Monitor confirmed 2026-06-20
- [x] UDI TX = `F0 7E 7F 06 01 F7` on IAC bus — MIDI Monitor confirmed 2026-06-20 (labeled "Universal Non-Real Time 6 bytes")
- [x] Real XT autodetect returns `valid=true` with correct family/member code — confirmed 2026-06-20; port=Unitor8/AMT8 Port 13, deviceId=0x00, familyMemberLow=0x03, familyMemberHigh=0x00, firmware="2.33" (14-byte XT firmware quirk: omits devId byte; parser handles both 14- and 15-byte formats)
- [x] Returned firmware version string logged in ROADMAP.md changelog — confirmed 2026-06-20

**Observations from 2026-06-20 run:**
- Key repeat fired ~58 repetitions; `keyPressed` should guard with `k.isKeyCurrentlyDown()` check or a flag if repeat is a problem in practice.
- `juce::Thread::sleep()` called from the message thread is a no-op; 50ms pauses did not take effect. Frames arrived out of send-order but all bytes were correct. Pauses are cosmetic for human readability in MIDI Monitor only.

**M1.3 closed 2026-06-20.** All 9 pass criteria confirmed. Exit criteria marked `[x]` in `ROADMAP.md`. "Currently in flight" updated to M1.4. Debug triggers (`runMidiVerification`, `runAutodetectScan`) removed from `PluginEditor`.
