# Product Requirements Document (PRD)

## Project: Xenia-HW (Microwave XT Hybrid Integration Layer)

**Author:** Jake Boswell

**Date:** June 2026

**Status:** Draft / Exploratory

---

## 1. Executive Summary

The Waldorf Microwave XT remains a legendary instrument for wavetable synthesis. While modern software options exist, users frequently choose between two disconnected worlds: the flawless DAW integration and recall of the low-level digital emulator **Xenia (Gearmulator)**, or the tactile workflow and physical digital-to-analog converter (DAC) character of the **original hardware unit**.

**Xenia-HW** is a proposed open-source extension/fork of the Gearmulator/Xenia project. Instead of creating a redundant standalone editor from scratch, this project introduces a bidirectional hardware bridging layer directly into Xenia’s C++ (JUCE-based) architecture. This transforms Xenia into a total-recall software wrapper, a high-fidelity control surface matrix, and an advanced librarian for the physical Microwave XT hardware.

---

## 2. Problem Statement & Justification

### The Status Quo

1. **Fragile Legacy Editors:** Existing software editors (e.g., SoundDiver, MonstrumWave) are abandonware, reliant on fragile Java/Adobe runtimes, or unstable on modern macOS/Windows operating systems.
2. **The MIDI Learn Wall:** Simply using standard DAW MIDI learn to map the physical XT knobs to the Xenia emulator fails to support complex 14-bit NRPNs, lacks multitimbral "Multi-Mode" routing intelligence, and cannot push total-recall patch data back out to the physical unit upon loading a session.
3. **Development Redundancy:** Writing a standalone editor from scratch requires hundreds of hours recreating UI components, file parsers, and synth state-tracking engines that Xenia has already perfectly implemented.

### The Hybrid Solution

By injecting hardware-mirroring logic directly into Xenia's core engine loop, we leverage a production-ready, bit-accurate UI and data model to act as the ultimate middleware translation proxy between a modern DAW and vintage hardware.

---

## 3. High-Level Architecture & Data Flow

Xenia-HW intercepts internal state modifications within the emulator and mirrors them via physical MIDI DIN/USB, using a specialized scheduler to protect the vintage hardware's buffer.

```
+-----------------------------------------------------------------------+
|                            DAW / HOST                                 |
|  (Saves VST3/AU State, Automates Parameters, Triggers Project Load)   |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|                       XENIA INTERNALS (JUCE Core)                     |
|  - Tracks 256-byte Patch States       - Renders Authentic UI Layout    |
|  - Manages Multi-Mode Configurations  - Ingests/Parses .SYX Files     |
+-----------------------------------------------------------------------+
          |                                                   ^
          | (On UI Tweak / Automation)                        | (On Knob Turn)
          v                                                   |
+-----------------------------------------------------------------------+
|                         XENIA-HW BRIDGE LAYER                         |
|  - SysEx / NRPN Translation Engine                                    |
|  - Concurrency & Thread-Safe State Arbitrator                         |
|  - Intelligent Output Throttler & Buffer Queue                        |
+-----------------------------------------------------------------------+
          |                                                   ^
          | (Throttled Hardware SysEx/NRPN)                   | (Raw MIDI DIN Out)
          v                                                   |
+-----------------------------------------------------------------------+
|                       PHYSICAL MICROWAVE XT                           |
|  - Decodes MIDI Inputs Into Active Edit Buffers                       |
|  - Generates Native Sound Signature via Physical 40kHz DACs           |
+-----------------------------------------------------------------------+

```

---

## 4. Functional Requirements

### 4.1 Hardware Integration & Settings

* **HW.001: Dedicated Hardware Routing Bus:** The configuration menu must expose a distinct, dedicated MIDI Output port dropdown, completely separate from the host DAW's standard tracking outputs.
* **HW.002: Global Channel Matching:** The software must provide an explicit "Hardware Global Channel" configuration parameter to cleanly align background handshakes.
* **HW.003: Pass-Through Audio Suppress (Optional Mode):** Provide a toggle to mute Xenia's internal emulation audio engine, allowing it to function exclusively as a "Control & Recall Wrapper" for the physical unit.

### 4.2 Bidirectional Parameter Mirroring

* **MIR.001: Real-time UI Tracking:** When a knob is adjusted on the physical XT front panel, Xenia-HW must intercept the incoming NRPN/SysEx stream, parse the target parameter, and update the GUI/internal state smoothly in real-time.
* **MIR.002: Multi-Mode Part Aware Routing:** The bridge must dynamically inspect Xenia's active Multi part selection. If the physical hardware sends a message on the global channel, the bridge must direct that update to the currently focused virtual part layer.
* **MIR.003: Outbound Translation:** Tweaking any UI element in Xenia or playing back a DAW automation lane must instantly compile the corresponding Microwave XT hardware-compliant 7-bit CC, 14-bit NRPN, or single-parameter SysEx byte string.

### 4.3 Total Recall Engine

* **REC.001: Automatic Session Dumps:** Upon instantiation or initialization of a saved DAW project, Xenia-HW must capture the stored plugin state chunk and seamlessly package it into a sequence of hardware-compliant SysEx Edit Buffer Dumps.
* **REC.002: Multi-Mode Reconstruction:** For multitimbral environments, the plugin must systematically transmit the full Multi-configuration profile followed by individual Single-patch byte arrays for all active instrument slots.

### 4.4 Advanced Librarian System

* **LIB.001: Bidirectional Bank Management:** Users must be able to pull complete bank archives directly from the physical hardware memory into a local desktop browser matrix via classic hardware SysEx requests.
* **LIB.002: Drag-and-Drop Organization:** Enable simple reorganization, renaming, and staging of `.syx` patch profiles inside a visual grid, with instant hardware auditioning upon selecting a patch cell.

### 4.5 Patch Randomization Subsystem

* **RND.001: Intelligent Algorithmic Morphing:** Implement targeted variation routines (e.g., locking the core envelope shapes while mutating wavetable indices and modulation matrix paths).
* **RND.002: Instant Hardware Sync:** Randomized parameters must generate a localized burst transmission to update the physical synth's edit buffer instantly for tactile auditioning.

---

## 5. Technical & Non-Functional Requirements

### 5.1 Transmission Throttle & Traffic Control

* **TEC.001: SysEx Message Pacing:** The vintage Motorola MC68331 MCU inside the XT is easily overwhelmed. The bridge layer must integrate a precise millisecond-level scheduling queue to throttle long SysEx chains, entirely eliminating buffer overflow freezes or clicking sounds.
* **TEC.002: Thread Isolation:** All serial MIDI serialization, translation queuing, and communication tracking routines must be handled off the main audio rendering thread to prevent performance drops or GUI lagging inside the host DAW.

### 5.2 Codebase Coexistence

* **TEC.003: Native JUCE Design Pattern:** Built strictly using compliant modern C++ conventions that blend naturally into the existing Gearmulator framework layout, streamlining potential pull requests or feature upstreaming.

---

## 6. Project Risks & Mitigation Strategies

* **Risk 1: Hardware Buffer Flooding.** High-density automation data from a modern DAW could cause the physical XT to choke, drop notes, or freeze.
* *Mitigation:* Implement strict parameter thinning algorithms that drop intermediate redundant frames if data values alter faster than a configured threshold (e.g., maximum 50 messages per second per parameter).


* **Risk 2: MIDI Loop Feedback.** A knob movement on the physical hardware updates the UI, which could inadvertently echo the change back out to the hardware, resulting in a feedback storm.
* *Mitigation:* Enforce a strict token-ownership or state-blocking flag pattern while processing incoming hardware messages, preventing reflected transmissions.