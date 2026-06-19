Script is at tools/sndp_rate_test.py. Here's how to run it:

Setup first:
1. Connect XT via MIDI interface (USB or DIN)
2. On the XT: set Global > MIDI > SysEx = On (or Ctl+SysEx) so it echoes parameter changes back — that's how you confirm it received each message
3. Open MIDI Monitor.app (or similar) on the XT's output port so you can see the echo stream visually

Then:
# See your ports
python3 tools/sndp_rate_test.py --list

# Run the test (adjust port name to match what --list shows)
python3 tools/sndp_rate_test.py --port "your-interface-name" --device 0

What it tests: four parameters — Filter Cutoff, Resonance, Osc Detune, and Wave (the one gearmulator rate-limits) — at intervals of 200ms, 100ms, 50ms, 20ms, 10ms, 5ms, 2ms, 1ms. Each sweep is 64 steps (0→127→0). You enter ok/drop/crash/skip after each and it writes results to sndp_rate_test_results.txt.

The key thing to watch: does Wave drop at a higher interval than the others, or do they all behave the same? That tells us whether the wave-specific rate limiting in gearmulator is substantiated or just conservative.
