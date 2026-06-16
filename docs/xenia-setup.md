# Xenia + IAC Bus Setup

This guide walks through wiring up **Xenia** (gearmulator's Microwave II/XT software emulation) as a MIDI loopback target for the editor.

**Two audiences:**

- **Developers and testers (now):** Xenia is the primary dev-loop target throughout Phase 1+ — see [Testing strategy: Xenia + real hardware](spec/editor-requirements.md#testing-strategy-xenia--real-hardware) for context on when Xenia is sufficient vs. when real XT is required.
- **End users (eventually, M6.6):** This is the same setup that lets someone use the editor without owning Microwave hardware. The M6.6 milestone wires an in-app help link to this guide.

Both audiences use the same plumbing. The instructions are identical.

---

## Prerequisites

- macOS (these instructions are macOS-specific; Linux/Windows notes at the end)
- A DAW or AU/VST host (Reaper, AU Lab, Logic Pro, Bitwig — anything that loads VST3 or AU plugins)
- The editor built and runnable (M1.1 minimum)
- An ability to build gearmulator's Xenia plugin from source, **or** a pre-built Xenia binary

## Step 1 — Enable the macOS IAC Bus

macOS ships with a virtual MIDI driver called the **IAC Bus**, disabled by default.

1. Open **Audio MIDI Setup** (in `/Applications/Utilities/` or via Spotlight)
2. Menu: **Window → Show MIDI Studio** (or ⌘2)
3. Double-click the **IAC Driver** icon
4. Tick **Device is online**
5. Confirm at least one port exists (default name is "Bus 1")
6. Optional: rename it to something memorable like "XT-Editor Bus" — the editor's device picker shows this name

Verification: the IAC Driver icon goes from greyed-out to solid. Other applications now see the port as both a MIDI input and a MIDI output.

## Step 2 — Build (or obtain) the Xenia plugin

Xenia is one of several synth emulations in the gearmulator repo. To build it from `references/gearmulator/`:

```sh
cd references/gearmulator
mkdir -p temp/cmake_mac && cd temp/cmake_mac
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release \
  -Dgearmulator_SYNTH_XENIA=ON \
  -Dgearmulator_BUILD_JUCEPLUGIN=ON
cmake --build . --target xenia_VST3 xenia_AU xenia_Standalone -j
```

This produces:
- Standalone: `xenia.app` somewhere under `temp/cmake_mac/bin/` (path varies by build config)
- AU: `xenia.component` — install to `~/Library/Audio/Plug-Ins/Components/`
- VST3: `xenia.vst3` — install to `~/Library/Audio/Plug-Ins/VST3/`

> Xenia needs a Waldorf firmware ROM file to actually produce sound. The gearmulator README explains the firmware-loading path. Without a ROM, Xenia loads as a plugin but the audio engine is inert — that's still useful for SysEx protocol testing (the message handling doesn't depend on audio), just not for auditioning sound.

## Step 3 — Load Xenia in a host

Open your DAW or AU Lab and instantiate Xenia on a MIDI track.

For unattended/headless dev work, **AU Lab** (free, downloadable from Apple Developer) is the lightest option:

1. AU Lab → File → New → empty document
2. Add a Music Effect or Instrument track
3. Insert the Xenia AU
4. Audio output → Built-in Output (or your interface)

## Step 4 — Route MIDI between editor and Xenia

In the host that's running Xenia:

- **Xenia's MIDI Input** → set to **IAC Bus 1** (or whatever you named your IAC port)
- **Xenia's MIDI Output** → also set to **IAC Bus 1** (same port — IAC is bidirectional)

In the editor (once M1.1 lands, this is the device picker in settings):

- **MIDI Out** → **IAC Bus 1**
- **MIDI In** → **IAC Bus 1**

That's it. The editor and Xenia are now talking over the IAC Bus as if connected by a real MIDI cable.

## Step 5 — Verify the loopback works

A simple sanity check:

1. In the editor's MIDI device picker, click **Detect device** (M1.3+ feature)
2. Editor sends Universal Device Inquiry: `F0 7E 7F 06 01 F7`
3. Xenia replies: `F0 7E 06 02 3E 0E 00 03 00 ...` (where `03 00` identifies as Microwave XT)
4. Editor reports the detected device name and firmware version

If autodetect fails:
- Confirm both endpoints are pointed at the same IAC port (not e.g. Bus 1 vs Bus 2)
- Confirm Xenia's host is actually receiving MIDI from IAC (most hosts show a MIDI activity indicator)
- Confirm the IAC Driver is online (Audio MIDI Setup → MIDI Studio)
- A general-purpose MIDI monitor (MIDI Monitor.app, free) routed to the same IAC port will show whether bytes are flowing — useful for narrowing down which side is misbehaving

## Step 6 — Use it

Day-to-day dev: leave Xenia open in its host. Rebuild the editor; reattach. The IAC connection survives editor rebuilds and Xenia stays loaded — that's the whole point of this setup.

For the trust boundary (what's safe to test on Xenia alone vs requires real XT), see the [trust boundary table](spec/editor-requirements.md#trust-boundary--whats-safe-to-gate-on-xenia-alone-vs-requires-real-xt) in the editor requirements doc.

---

## Troubleshooting

**Xenia receives messages but doesn't respond.** Probably no firmware ROM loaded. Check the Xenia plugin's settings panel for a ROM-loading affordance. SysEx protocol testing (M1.2 / M1.3 message round-trips) does NOT require a ROM — only auditioning sound does.

**Universal Device Inquiry response is missing.** Some IAC configurations buffer/coalesce SysEx. Confirm the IAC port has SysEx enabled (it usually does by default) and try increasing the autodetect timeout from 500 ms to 1500 ms while debugging.

**Latency feels worse than real hardware.** Expected. IAC + plugin host adds a few ms of overhead that USB-MIDI doesn't. This rarely matters for parameter editing; for note timing during auditioning, real hardware is more representative.

**Editor sees IAC port but no MIDI flows.** Check that the IAC port is enabled in **both** the editor and the host (Xenia's host). macOS shows the port either way; ports must be explicitly selected for I/O.

**Xenia's audio sounds wrong.** Out of scope for the editor — we're testing the SysEx protocol, not the emulation's audio fidelity. If audio quality matters for what you're testing, gate that test on real XT (per the trust boundary).

---

## Non-macOS platforms

The same dual-target posture applies; only the virtual-MIDI mechanism differs.

- **Linux:** ALSA's virtual MIDI ports (`snd-virmidi` module) or JACK MIDI provide equivalents to IAC. The editor's device picker shows them via JUCE's MIDI device enumeration.
- **Windows:** No built-in virtual MIDI driver. **loopMIDI** (Tobias Erichsen, free) is the standard option; install, create a port, route both sides to it.

Xenia builds on Linux (gearmulator's `build_linux.sh`) and Windows (`build_win64.bat`). Same plugin formats; same routing model.
