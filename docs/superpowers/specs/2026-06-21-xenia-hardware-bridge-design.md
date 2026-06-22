# Xenia Hardware Bridge — strategic pivot + design

**Status:** Design / decision record, 2026-06-21. **Supersedes** the standalone-editor
product direction for Phase 1+ (see "Supersession" below). Direction approved by Jake
2026-06-21 ("I want to build this regardless of whether the gearmulator team pulls it in").
**Depends on:** the spike findings recorded below (read of gearmulator HEAD in
`resources/gearmulator-main/`).
**Related:** [`Xenia-HW-Controller-PRD.md`](../../../Xenia-HW-Controller-PRD.md) (the
originating vision — instinct correct, architecture framing corrected here),
[`ROADMAP.md`](../../../ROADMAP.md), [`references/gearmulator.md`](../../../references/gearmulator.md).

---

## 1. Decision in one paragraph

Stop building a standalone Microwave II/XT editor. **Xenia (gearmulator's MW II/XT
emulation) already ships the editor, the librarian, randomize, and a bit-accurate engine.**
The one thing nobody else will build — and the only thing we actually need — is a
**bidirectional bridge between Xenia and the physical hardware**: the XT as a control
surface, total recall to the metal, live bank transfer, and extended creative editing
(morph/nudge). The spike proves this bridge is an **additive module riding existing
gearmulator seams**, not a fork of its UI and not a from-scratch app. We build it as a
module against gearmulator, contribute it upstream if welcome, and otherwise carry it as a
thin rebased feature branch. Either way the code is the same.

## 2. Why the standalone editor no longer makes sense

The Microwave XT is a purely **digital** synth (DSP56300 + MC68K running original
firmware). The only thing separating the hardware's audio from Xenia's is the output DAC.
gearmulator emulates the actual processors running authentic ROMs, so the engine problem is
already solved by an actively maintained, community-backed project. Concretely, Xenia
already provides:

| Capability | Status in Xenia |
|---|---|
| Bit-accurate engine | ✅ DSP56300 + MC68K emulation, authentic firmware |
| Full parameter editor UI | ✅ `xtJucePlugin/` (RmlUi) |
| Patch librarian | ✅ patch manager + PatchDB, reads/writes hardware-format files |
| Randomize | ✅ 2.2.6 (skin/script layer) |

Rebuilding any of that (the old ROADMAP Phases 1–2) only made sense as a **competitor** to
Xenia — the losing posture, since Xenia is faster-moving and community-backed. The
[D-01](../../../ROADMAP.md) verdict "fork most of gearmulator" was about *extracting*
Xenia's RmlUi editor into our app; it does **not** apply to *adding* a bridge (see §4).

## 3. The actual gap (our entire value proposition)

What Xenia does **not** do, and structurally is unlikely to prioritise (its mission is to
*replace* the hardware):

1. **Live hardware MIDI I/O** — bidirectional sync, physical XT ↔ Xenia.
2. **Total recall to the metal** — push state to the physical unit on project/preset load.
3. **Live bank dump/restore** to/from the physical XT (Xenia's librarian stops at files).
4. **Physical XT as a multi-mode-aware control surface** (knob turn → focused part).
5. **Extended creative editing** — morph, nudge (randomize already exists upstream).

Items 1–4 are one thing: the **hardware bridge**. Item 5 is creative tooling layered on
the existing randomize.

## 4. Spike findings — the bridge is ~80% pre-built seams

Read of `resources/gearmulator-main/source/` (file paths below). The infrastructure a
bridge needs already exists in `jucePluginLib`; a bridge *consumes* it rather than
*extracting* any subsystem — which is why D-01's closure problem does not arise and the UI
is untouched.

| Bridge need | Existing seam (file) |
|---|---|
| Dedicated external MIDI out/in, **off the audio thread** (HW.001, TEC.002) | `MidiPorts` — selectable external in/out, ring-buffered sender thread, user port picker. `jucePluginLib/midiports.h`, UI in `jucePluginEditorLib/midiPorts.cpp` |
| Route param MIDI to hardware vs host | `MidiRoutingMatrix` gates events by source. `jucePluginLib/processor.cpp:825–834` |
| Param → XT SysEx/NRPN/CC (outbound, MIR.003) | `Controller::sendParameterChange()`, `createMidiDataFromPacket()`; XT specialisation `xtJucePlugin/xtController.{cpp,h}`; wire format in `parameterDescriptions_xt.json` (`midipackets`) |
| XT → param (inbound, MIR.001) | `Controller::parseSysexMessage` / `parseMidiMessage` / `parseControllerMessage` |
| Loop-feedback prevention (Risk 2) | `Parameter::Origin {Unknown, PresetChange, Midi, HostAutomation, Ui, Derived}`. `jucePluginLib/parameter.h:24` |
| Observable param changes (outbound hook) | `Parameter::onValueChanged`. `parameter.h:36` |
| Bidirectional controller feedback (the control-surface pattern) | MIDI Learn host-feedback queue (`m_hostFeedbackQueue`). `processor.cpp:836–845` |
| Multi-mode part routing (MIR.002) | `getCurrentPart`, `getPartsForMidiChannel`, `onCurrentPartChanged`. `controller.h` |
| Total recall on load (REC.001) | `Controller::onStateLoaded()` virtual hook + state chunk |

Note: `hardwareLib/` is chip-level *emulation* (flash/i2c/LCD/SCI-MIDI for the DSP), and
`bridge/` is the network/server-plugin (remote DSP) feature — **neither is an
external-physical-hardware bridge**, confirming the gap is real and unclaimed.

## 5. Architecture of the bridge module

A single, self-contained component (working name `xtHwBridge`) owned by the XT plugin,
wired to the seams above. It does not modify the editor and adds no new UI subsystem
(only a settings toggle + the already-existing MIDI port picker).

```
            Xenia plugin (xtJucePlugin)
   ┌─────────────────────────────────────────────────┐
   │  Parameter::onValueChanged  ──►  xtHwBridge       │
   │  xtController (param↔SysEx)  ◄─►  (outbound mirror,│      MidiPorts
   │  onStateLoaded (total recall)──►  throttle queue,  │──────► external ──► PHYSICAL XT
   │  getCurrentPart / parts ──────►   part routing,    │◄────── port    ◄── (knobs/banks)
   │  Parameter::Origin (echo guard)◄─  inbound apply)  │
   └─────────────────────────────────────────────────┘
```

**Outbound (Xenia → XT):** subscribe to `onValueChanged` (or hook
`xtController::sendParameterChange`); for changes whose `Origin ∈ {Ui, HostAutomation,
PresetChange, Derived}`, build the XT message via the existing `createMidiDataFromPacket`
path and enqueue to the XT-aware throttle, which drains to the `MidiPorts` hardware output.

**Inbound (XT → Xenia):** feed the `MidiPorts` hardware input through the existing
`parseSysexMessage` / `parseMidiMessage`, applying with `Origin::Midi`. Echo prevention
falls out for free: outbound only fires on non-`Midi` origins.

**Total recall (REC.001):** on `onStateLoaded`, walk the active part(s)' params and emit a
full edit-buffer dump through the throttle.

**Net-new code (the only real work):**
1. `xtHwBridge` glue (subscribe, route, origin-gate).
2. **XT-aware throttle/pacing** for the slow MC68331 — *this is where our hard-won
   protocol knowledge transfers* (SNDP encoding, the D-03 Wave-rate finding: Wave params
   need ~100 ms pacing, normal params fine at 20 ms). The ring buffer exists; XT-specific
   pacing and parameter-thinning (PRD Risk 1) are new.
3. Total-recall walk on load.
4. Multi-mode-aware routing using `getCurrentPart` / `getPartsForMidiChannel`.
5. Creative tools: morph/nudge, extending the existing randomize.

## 6. Posture toward Xenia / governance

- **Contribute-upstream-first.** The presence of `MidiPorts` + routing matrix + MIDI Learn
  host-feedback shows external-gear interop is already in scope upstream, so a hardware
  bridge is plausibly welcome rather than against the grain. gearmulator is GPL-3.0,
  actively maintained, with an opinionated process (its own `CLAUDE.md`, YouTrack, Jenkins/
  GitHub CI). See the proposal: [`docs/proposals/2026-06-21-gearmulator-hardware-bridge-proposal.md`](../../proposals/2026-06-21-gearmulator-hardware-bridge-proposal.md).
- **Fallback: thin rebased feature branch** if declined. Cheap *because* the module is
  additive — we rebase one module onto upstream churn, not a forked UI. **We build it
  either way.**
- **Decoupling discipline:** keep `xtHwBridge` as one module with a narrow interface to the
  seams in §4, so it rebases cleanly and could be upstreamed later even if we start on a
  branch.

## 7. License

gearmulator is **GPL-3.0**. Upstream contribution is clean. A carried feature branch / fork
inherits GPL-3.0; our additions are GPL-3.0; per-file attribution continues in
[`ATTRIBUTIONS.md`](../../../ATTRIBUTIONS.md). (Prior plan to ship our own app under
AGPL-3.0 is moot — we are no longer shipping a separate editor.)

## 8. Supersession — impact on existing milestones

- **Standalone editor (ROADMAP Phases 1–6) → shelved.** M1.1–M1.6a code is not deleted but
  is no longer the product path. The **protocol/SysEx/SNDP knowledge** built there
  (sysex-protocol.md, rate-limit findings, parameter mapping, the SNDP encoding-bug fix)
  **transfers directly** into the bridge's throttle and message-build paths; the UI
  reimplementation (PageComponent/EditorComponent) does not carry over.
- **Branch `m1.6a-page-switch`** is left unmerged; no new editor-UI milestones are started.
- A new milestone track (**XHB-x**) replaces ROADMAP Phase 1+ (see §8.1).

## 8.1 Milestone track & effort estimate

Sessions are rough sizing (one focused block ≈ a task with TDD + review, the M1.6a
cadence), **not commitments**. Software estimates are unreliable; **XHB-C is the estimate
checkpoint** — once one parameter round-trips cleanly on real hardware, the per-message cost
is known and the remainder tightens to ±20%. Re-estimate then.

| ID | Milestone | Effort | Sessions | Gate? |
|---|---|---|---|---|
| XHB-A | Build env: gearmulator + Xenia building/debuggable; resolve OQ-2 (submodule vs fork vs build) | M | 1–2 | |
| XHB-B | Runtime seam confirmation (`MidiPorts`, `sendParameterChange`, `Origin`, `onStateLoaded`) | S | 1 | |
| XHB-C | **Vertical slice:** one param Xenia→XT, throttled, `Origin`-gated, no echo, on real XT | M | 2–3 | **gate** |
| XHB-D | Full bidirectional: inbound parse + apply as `Origin::Midi`; multi-mode part routing (MIR.002) | M | 2 | |
| XHB-E | XT-aware throttle + parameter thinning; re-validate D-03 pacing on real XT (OQ-3) | M | 1–2 | |
| XHB-F | Total recall (REC.001/002): single + Multi (MULD + per-part SNDD); resolve OQ-4 | L | 2–3 | |
| XHB-G | Live bank dump/restore to/from the metal (SNDR→SNDD state machine) | L | 2–3 | |
| XHB-H | Creative tools: morph / nudge (extend upstream randomize) | M | 1–2 | |
| XHB-I | Settings toggle, polish, upstream-prep (clang-format/conventions, docs) | M | 1–2 | |

**Headline:** bidirectional MVP (A–E) **≈ 7–10 sessions**; complete bridge (A–I)
**≈ 13–20 sessions**. Anchor on **MVP ≈ 8, full ≈ 15** until XHB-C lands.

**Swing factors (any one can add several sessions):** first-time gearmulator build pain
(DSP56300 JIT, RmlUi, many deps — most likely overrun); real-hardware wire-format/timing
debugging (cf. SNDP encoding, 14-byte UDI); XHB-F (Multi recall) and XHB-G (bank transfer)
are the meatiest/fiddliest; upstream structure decision (OQ-2) may force rework; the
no-subagents spend constraint slows individual sessions.

## 9. Open questions / risks

- **OQ-1 (governance):** Will the maintainer accept hardware-bridge code upstream, and on
  what terms (module boundary, per-synth vs shared)? Resolved by the proposal conversation
  (a). Does **not** block: we build regardless.
- **OQ-2 (build):** Do we build the full plugin from `resources/gearmulator-main`, track a
  fork remote, or work in a submodule? Decide at plan time. gearmulator dev uses CMake +
  per-synth flags (`-Dgearmulator_SYNTH_XENIA=ON`).
- **OQ-3 (throttle fidelity):** Re-validate D-03 pacing through the *bridge* path on real
  XT (Xenia is **not** faithful for Wave-rate; hardware test required).
- **OQ-4 (state model):** Confirm `onStateLoaded` exposes enough to reconstruct a full
  Multi (MULD + per-part SNDD) for REC.002, not just the current single.
- **Risk (third-party dependency):** value now rides on Xenia's health/roadmap. Mitigated
  by additive-module design (low rebase cost) and the build-regardless decision.

## 10. Verification strategy

Inherit the project's Xenia-+-real-hardware testing discipline: Xenia (loopback to a second
Xenia instance, or to hardware) for fast dev-loop; **real XT mandatory** for throttle
fidelity (OQ-3), total recall (REC.001/002), and bank transfer. A round-trip test
(param → bridge → XT → bridge → param, asserting no echo via `Origin`) is the gate for the
first vertical slice.
