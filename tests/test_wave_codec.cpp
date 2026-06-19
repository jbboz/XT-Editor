#include "TestRunner.h"
#include "mw2xtLib/WaveCodec.h"
#include <array>
#include <cstdint>

int main() {
    TestRunner r;

    r.add("encodeWave: produces 64 nibble-bytes for 128-sample wave", []() {
        mw2xt::WaveData wave{};
        EXPECT_EQ(mw2xt::encodeWave(wave).size(), 128u);
    });

    r.add("encodeWave: XOR-flip sign format (Waldorf §3.4)", []() {
        // sample = +1 (signed); +1 ^ 0x80 = 0x81
        // high nibble = 0x81 >> 4 = 0x08; low nibble = 0x81 & 0xF = 0x01
        mw2xt::WaveData wave{};
        wave[0]   =  1;
        wave[127] = -1;  // mirror constraint: wave[127-0] = -wave[0]
        auto n = mw2xt::encodeWave(wave);
        EXPECT_EQ(n[0], uint8_t(0x08));
        EXPECT_EQ(n[1], uint8_t(0x01));
    });

    r.add("wave codec: round-trip on all-zero wave (silence)", []() {
        mw2xt::WaveData wave{};
        auto decoded = mw2xt::decodeWave(mw2xt::encodeWave(wave));
        for (int i = 0; i < 128; ++i)
            EXPECT_EQ(decoded[i], wave[i]);
    });

    r.add("wave codec: round-trip on ascending pattern", []() {
        // Construct wave satisfying mirror constraint: wave[127-i] = -wave[i]
        mw2xt::WaveData wave{};
        for (int i = 0; i < 64; ++i) {
            wave[i]       = static_cast<int8_t>(i * 2 - 63);
            wave[127 - i] = static_cast<int8_t>(-wave[i]);
        }
        auto decoded = mw2xt::decodeWave(mw2xt::encodeWave(wave));
        for (int i = 0; i < 128; ++i)
            EXPECT_EQ(decoded[i], wave[i]);
    });

    r.add("decodeWave: mirror constraint satisfied on decode", []() {
        // After decode, wave[127-i] must equal -wave[i] for i=0..63
        mw2xt::WaveData wave{};
        wave[0] = 10; wave[127] = -10;
        wave[1] = 20; wave[126] = -20;
        auto decoded = mw2xt::decodeWave(mw2xt::encodeWave(wave));
        for (int i = 0; i < 64; ++i)
            EXPECT_EQ(int(decoded[127 - i]), -int(decoded[i]));
    });

    return r.run();
}
