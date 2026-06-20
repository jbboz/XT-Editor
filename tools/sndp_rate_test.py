#!/usr/bin/env python3
"""
SNDP Rate Limiter Hardware Test
Waldorf Microwave II/XT

Tests how quickly the XT firmware can accept SNDP (Sound Parameter Change)
messages before it starts dropping them. Run this against real hardware to
determine whether a rate limiter is needed and, if so, for which parameters.

Usage:
    python3 sndp_rate_test.py [--port <name>] [--device <0-15>] [--list]

Results are written to sndp_rate_test_results.txt alongside this script.
"""

import rtmidi
import time
import argparse
import sys
from typing import List, Optional

# ---------------------------------------------------------------------------
# SNDP message construction
# ---------------------------------------------------------------------------
# Waldorf SysEx header: F0 3E 0E <device> <idm> ...
WALDORF_MFRID   = 0x3E
WALDORF_DEVTYPE = 0x0E
IDM_SNDP        = 0x20  # Sound Parameter Change
IDM_SNDR        = 0x00  # Sound Dump Request  (to read back final value)
IDM_SNDD        = 0x10  # Sound Dump (response)

def sndp(device_id: int, param_index: int, value: int, location: int = 0x00) -> List[int]:
    """Build a single SNDP SysEx message (10 bytes: F0 3E 0E DEV 20 LL HH PP XX F7)."""
    hh = 0x01 if param_index >= 128 else 0x00
    pp = param_index & 0x7F
    vv = value & 0x7F
    return [0xF0, WALDORF_MFRID, WALDORF_DEVTYPE, device_id, IDM_SNDP, location, hh, pp, vv, 0xF7]

def sndr(device_id: int) -> List[int]:
    """Build a Sound Dump Request for the edit buffer."""
    # BB=20h (single edit buffer single mode), NN=00h
    return [0xF0, WALDORF_MFRID, WALDORF_DEVTYPE, device_id, IDM_SNDR, 0x20, 0x00, 0x7F, 0xF7]

# ---------------------------------------------------------------------------
# Parameters to test
# ---------------------------------------------------------------------------
# Format: (name, sdata_index, min_val, max_val)
# Using audible, continuous parameters that won't cause side effects.
PARAMS = [
    ("Filter1 Cutoff",  62,   0, 127),   # SDATA 62 — obvious audible sweep
    ("Filter1 Reson",   63,   0, 127),   # SDATA 63 — resonance
    ("Osc1 Detune",      3,   0, 127),   # SDATA 3  — detuning, clearly audible
    ("Wavetable",       25,   0, 127),   # SDATA 25 — wavetable position, the one gearmulator rate-limits
]

# ---------------------------------------------------------------------------
# Test intervals (milliseconds between consecutive SNDP sends)
# ---------------------------------------------------------------------------
# We go from slow → fast to find the threshold.
INTERVALS_MS = [200, 100, 50, 20, 10, 5, 2, 1]

# Number of value steps per rate test (always sweeps 0→127→0 in N steps)
STEPS_PER_SWEEP = 32

# Pause between individual parameter tests to let firmware settle
SETTLE_MS = 500

# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------
def list_ports():
    mo = rtmidi.MidiOut()
    ports = mo.get_ports()
    if not ports:
        print("No MIDI output ports found. Connect your interface and retry.")
        return
    print("Available MIDI output ports:")
    for i, name in enumerate(ports):
        print(f"  {i:2d}: {name}")

    mi = rtmidi.MidiIn()
    in_ports = mi.get_ports()
    print("\nAvailable MIDI input ports:")
    for i, name in enumerate(in_ports):
        print(f"  {i:2d}: {name}")


def find_port(ports: List[str], query: str) -> Optional[int]:
    """Find port by index or substring match."""
    try:
        idx = int(query)
        if 0 <= idx < len(ports):
            return idx
        return None
    except ValueError:
        query_lower = query.lower()
        for i, name in enumerate(ports):
            if query_lower in name.lower():
                return i
        return None


def run_rate_test(midi_out, midi_in, device_id: int, results: List[str]):
    print(f"\nDevice ID: {device_id} (channel {device_id + 1})\n")
    results.append(f"Device ID: {device_id}\n")

    for param_name, param_idx, p_min, p_max in PARAMS:
        print(f"\n{'='*60}")
        print(f"Parameter: {param_name}  (SDATA index {param_idx})")
        print(f"{'='*60}")
        results.append(f"\nParameter: {param_name} (SDATA {param_idx})\n")
        results.append(f"{'interval_ms':>12}  {'steps':>6}  {'sent':>6}  {'result':>10}\n")

        for interval_ms in INTERVALS_MS:
            interval_s = interval_ms / 1000.0

            # Build sweep: 0→127→0 in STEPS_PER_SWEEP steps
            sweep_up   = [int(p_min + (p_max - p_min) * i / (STEPS_PER_SWEEP - 1))
                          for i in range(STEPS_PER_SWEEP)]
            sweep_down = list(reversed(sweep_up))
            values     = sweep_up + sweep_down
            last_value = values[-1]

            # Send sweep
            t0 = time.perf_counter()
            send_errors = 0
            for i, v in enumerate(values):
                msg = sndp(device_id, param_idx, v)
                if i == 0:
                    print(f"  [first msg: {' '.join(f'{b:02X}' for b in msg)}]", flush=True)
                try:
                    midi_out.send_message(msg)
                except Exception as e:
                    send_errors += 1
                    if send_errors == 1:
                        print(f"\n  [send error: {e}]", flush=True)
                time.sleep(interval_s)
            elapsed = time.perf_counter() - t0
            if send_errors:
                print(f"  [{send_errors} send errors total]", flush=True)

            # Let the XT settle, then request dump to read back state
            time.sleep(SETTLE_MS / 1000.0)

            # Result label — user observes and notes; we print the expected last value
            print(f"  interval={interval_ms:>4}ms  steps={len(values)}  "
                  f"elapsed={elapsed*1000:.0f}ms  expected_final={last_value}",
                  end="", flush=True)
            results.append(f"{interval_ms:>12}  {len(values):>6}  "
                           f"{len(values):>6}  expected_final={last_value}\n")

            # Prompt user for observation
            obs = input("  → observation (ok/drop/crash/skip): ").strip().lower()
            if not obs:
                obs = "ok"
            results[-1] = results[-1].rstrip("\n") + f"  obs={obs}\n"

            if obs in ("crash", "skip"):
                print("  Stopping this parameter's rate sweep.")
                break

        # Reset parameter to a neutral mid value
        midi_out.send_message(sndp(device_id, param_idx, 64))
        time.sleep(SETTLE_MS / 1000.0)


def main():
    parser = argparse.ArgumentParser(description="SNDP rate limiter hardware test")
    parser.add_argument("--port", default=None,
                        help="MIDI output port name or index (default: auto-select first)")
    parser.add_argument("--in-port", default=None,
                        help="MIDI input port name or index (for readback)")
    parser.add_argument("--device", type=int, default=0,
                        help="SysEx device ID 0-15 (default: 0 = channel 1)")
    parser.add_argument("--list", action="store_true",
                        help="List MIDI ports and exit")
    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    # --- Open output port ---
    midi_out = rtmidi.MidiOut()
    out_ports = midi_out.get_ports()
    if not out_ports:
        print("ERROR: No MIDI output ports found. Connect your interface.")
        sys.exit(1)

    out_idx = 0
    if args.port is not None:
        found = find_port(out_ports, args.port)
        if found is None:
            print(f"ERROR: Output port '{args.port}' not found.")
            list_ports()
            sys.exit(1)
        out_idx = found

    midi_out.open_port(out_idx)
    print(f"Output: {out_ports[out_idx]}")

    # --- Open input port (optional) ---
    midi_in = None
    in_ports_list = rtmidi.MidiIn().get_ports()
    if args.in_port is not None and in_ports_list:
        midi_in = rtmidi.MidiIn()
        in_idx = find_port(in_ports_list, args.in_port)
        if in_idx is not None:
            midi_in.open_port(in_idx)
            midi_in.ignore_types(sysex=False)
            print(f"Input:  {in_ports_list[in_idx]}")

    print("""
Test instructions
-----------------
For each rate, the script sends a full sweep of the parameter (0→127→0)
at the specified interval and asks for your observation:

  ok    — XT responded smoothly throughout the sweep
  drop  — some values were skipped or the sweep sounded stepped/frozen
  crash — XT froze, audio stopped, or firmware locked up
  skip  — abort this rate (e.g., you can't hear the parameter)

Have a MIDI monitor (MIDI Monitor.app or similar) open on the XT's
output port so you can see if it echoes SNDP back. The XT echoes
parameter changes when in SysEx send mode (GDATA 26 = SysEx or Ctl+SysEx).

Press Enter to begin. Ctrl-C to abort at any time.
""")
    input()

    results = [f"SNDP Rate Test — {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n"]

    try:
        run_rate_test(midi_out, midi_in, args.device, results)
    except KeyboardInterrupt:
        print("\nAborted.")
        results.append("\n[Aborted by user]\n")

    # --- Write results ---
    out_path = __file__.replace(".py", "_results.txt")
    with open(out_path, "w") as f:
        f.writelines(results)
    print(f"\nResults written to {out_path}")

    midi_out.close_port()
    if midi_in:
        midi_in.close_port()


if __name__ == "__main__":
    main()
