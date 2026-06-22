# Proposal: bidirectional hardware bridge for Xenia (physical Microwave II/XT)

*Draft for posting as a gearmulator GitHub Discussion / feature issue. Audience: the
gearmulator maintainer(s). Tone: respectful, concrete, homework-done. Edit the bracketed
bits before posting.*

---

**Title:** Xenia ↔ physical Microwave II/XT hardware bridge — would you accept this upstream?

Hi — first, thank you for gearmulator; the XT emulation is superb and the plugin
infrastructure is a pleasure to read.

I own real Microwave XT hardware and want Xenia to act as a **total-recall control/edit
layer for the physical unit**: turn a knob on the XT and Xenia follows; tweak Xenia (or a
DAW automation lane) and the hardware follows; reload a project and the hardware is restored
to the saved state. Today people fake fragments of this with DAW MIDI-learn, which can't
handle the XT's 14-bit NRPN / single-parameter SysEx, multi-mode part routing, or
total-recall-on-load.

I've prototyped against `[HEAD / commit ___]` and it looks like **most of the
infrastructure already exists** — the feature would be an additive module that consumes
existing seams rather than touching the editor or the engine. Before I build it out, I'd
like to know whether you'd welcome it upstream, and if so how you'd want it structured.

## What I'd build

A self-contained bridge (per-synth, starting with Xenia) that:

1. **Outbound (Xenia → hardware):** on parameter changes originating from the UI /
   automation / preset load, emit the corresponding XT-format CC / 14-bit NRPN /
   single-parameter SysEx to a **dedicated hardware MIDI output**.
2. **Inbound (hardware → Xenia):** parse incoming NRPN/SysEx from the hardware and apply to
   the focused part.
3. **Total recall:** on state load, transmit a full edit-buffer (and, for Multi, the
   multi-config + per-part singles) to the unit.
4. **Throttling:** millisecond-paced output queue with parameter-thinning, because the
   MC68331 chokes on dense SysEx (I have real-hardware timing data — e.g. Wave-table
   parameters need ~100 ms pacing where ordinary params are fine far faster).

All gated behind a settings toggle and off by default; zero impact when unused.

## Why I think it's a small, clean addition

From reading `source/jucePluginLib/`, the seams already seem to be there:

- **Dedicated external MIDI I/O, off the audio thread** — `MidiPorts` (ring-buffered sender
  thread, user-facing port picker in `jucePluginEditorLib/midiPorts.cpp`).
- **Source-aware routing** — the MIDI routing matrix already gates events by source in
  `Processor::processBlock`.
- **Param ↔ XT wire format** — `Controller::sendParameterChange` /
  `createMidiDataFromPacket` / `parseSysexMessage`, with the packet shapes in
  `parameterDescriptions_xt.json`.
- **Loop-feedback prevention** — `Parameter::Origin {Ui, Midi, HostAutomation, …}` already
  tags provenance, so outbound can skip anything that arrived as `Midi`.
- **Observability** — `Parameter::onValueChanged`.
- **Bidirectional controller feedback** — you already do this for MIDI Learn
  (`m_hostFeedbackQueue`).
- **Multi-mode routing** — `getCurrentPart` / `getPartsForMidiChannel` /
  `onCurrentPartChanged`.
- **Total-recall hook** — `Controller::onStateLoaded`.

So the net-new code is mostly: a bridge component that subscribes to those events, an
XT-aware throttle/pacing queue, and the total-recall walk. I don't believe it requires any
change to the editor, the skin, or the DSP.

## Questions for you

1. Is a physical-hardware bridge something you'd want **upstream**, or would you prefer it
   live out-of-tree? (Either is fine on my end — I'm going to build it regardless because I
   need it; I'd just much rather contribute than carry a fork.)
2. If upstream: where should it live — a shared `…Lib` module reused across synths, or
   per-synth under `xtJucePlugin/`? Any existing pattern I should follow?
3. Any landmines you already know about (threading constraints around `MidiPorts`, state
   chunk layout for Multi recall, the routing matrix) that I should design around?
4. Would you want me to start with a **minimal slice** for review — single-parameter
   outbound mirroring to a selected hardware port, behind a setting — before the full
   bidirectional/total-recall scope?

I'll follow your conventions (I see the `CLAUDE.md`, `.clang-format`, and the
YouTrack/Jenkins notes). Happy to discuss design here first and keep PRs small and
reviewable.

Thanks for considering it.

— [Jake]
