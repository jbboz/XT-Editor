#include "WaveCodec.h"
#include <cassert>

namespace mw2xt {

// Adapted verbatim from gearmulator source/xtLib/xtState.cpp
// (createWaveData / extractWaveDataFromSysEx, ~L1050-1112).
// Changes: removed xt:: namespace deps; WaveData -> std::array<int8_t,128>;
// SysEx -> std::vector<uint8_t>.

std::vector<uint8_t> encodeWave(const WaveData& wave) {
    std::vector<uint8_t> nibbles;
    nibbles.reserve(128);
    for (int i = 0; i < 64; ++i) {
        const uint8_t sample = static_cast<uint8_t>(wave[i] ^ 0x80);
        nibbles.push_back(sample >> 4);
        nibbles.push_back(sample & 0xF);
    }
    return nibbles;
}

WaveData decodeWave(const std::vector<uint8_t>& nibbles64) {
    assert(nibbles64.size() == 128);
    WaveData wave{};
    for (int i = 0; i < 64; ++i) {
        const uint8_t high = nibbles64[i * 2];
        const uint8_t low = nibbles64[i * 2 + 1];
        int sample = (high << 4) | low;
        sample ^= 0x80;
        wave[i]       = static_cast<int8_t>(sample);
        wave[127 - i] = static_cast<int8_t>(-sample);
    }
    return wave;
}

} // namespace mw2xt
