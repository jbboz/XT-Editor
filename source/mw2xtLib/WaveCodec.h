#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace mw2xt {

// 128 signed 8-bit samples. The first 64 are the unique (stored) half.
// The second 64 satisfy: wave[127-i] = -wave[i] for i=0..63.
// This mirrors the hardware's Waldorf §3.4 format.
using WaveData = std::array<int8_t, 128>;

// Encode the first 64 samples to 64 SysEx nibble-bytes (2 nibbles per sample).
// Per sample s: byte = uint8_t(s ^ 0x80); nibbles = {byte>>4, byte&0xF}.
// Adapted from gearmulator source/xtLib/xtState.cpp.
std::vector<uint8_t> encodeWave(const WaveData& wave);

// Decode 64 nibble-bytes back to a 128-sample WaveData.
// Reconstructs the mirror half: decoded[127-i] = -decoded[i].
WaveData decodeWave(const std::vector<uint8_t>& nibbles64);

} // namespace mw2xt
