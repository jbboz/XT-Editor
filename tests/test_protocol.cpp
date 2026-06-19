#include "TestRunner.h"
#include "mw2xtLib/SoundData.h"
#include "mw2xtLib/MultiData.h"
#include "mw2xtLib/GlobalData.h"
#include "mw2xtLib/Protocol.h"

int main() {
    TestRunner r;

    // SoundData
    r.add("SoundData zero-initializes", []() {
        mw2xt::SoundData s;
        EXPECT_EQ(s[0], 0u);
        EXPECT_EQ(s[255], 0u);
    });

    r.add("SoundData operator[] roundtrip", []() {
        mw2xt::SoundData s;
        s[42] = 0x7F;
        EXPECT_EQ(s[42], 0x7Fu);
    });

    r.add("SoundData operator== and !=", []() {
        mw2xt::SoundData a, b;
        EXPECT(a == b);
        a[10] = 1;
        EXPECT(a != b);
    });

    r.add("patchName trims spaces", []() {
        mw2xt::SoundData s;
        const char* name = "Hello           "; // 5 chars + 11 spaces = 16
        for (int i = 0; i < 16; ++i) s[240 + i] = static_cast<uint8_t>(name[i]);
        EXPECT_EQ(s.patchName(), std::string("Hello"));
    });

    r.add("patchName trims null bytes", []() {
        mw2xt::SoundData s;
        const char* name = "Hi";
        s[240] = 'H'; s[241] = 'i';
        // bytes 242-255 stay 0x00 (from zero-init)
        EXPECT_EQ(s.patchName(), std::string("Hi"));
    });

    r.add("patchName all-spaces returns empty", []() {
        mw2xt::SoundData s;
        for (int i = 240; i < 256; ++i) s[i] = ' ';
        EXPECT_EQ(s.patchName(), std::string(""));
    });

    // MultiData
    r.add("InstrumentData zero-initializes", []() {
        mw2xt::InstrumentData d;
        EXPECT_EQ(d[0], 0u);
        EXPECT_EQ(d[27], 0u);
    });

    r.add("InstrumentData operator== and !=", []() {
        mw2xt::InstrumentData a, b;
        EXPECT(a == b);
        a[0] = 1;
        EXPECT(a != b);
    });

    r.add("MultiData idata starts at byte 32", []() {
        // Confirm no padding between mdata and idata[0]
        mw2xt::MultiData m;
        const uint8_t* base = reinterpret_cast<const uint8_t*>(&m);
        const uint8_t* idata0 = reinterpret_cast<const uint8_t*>(&m.idata[0]);
        EXPECT_EQ(static_cast<size_t>(idata0 - base), 32u);
    });

    // GlobalData
    r.add("GlobalData zero-initializes", []() {
        mw2xt::GlobalData g;
        EXPECT_EQ(g[0], 0u);
        EXPECT_EQ(g[31], 0u);
    });

    r.add("GlobalData operator== and !=", []() {
        mw2xt::GlobalData a, b;
        EXPECT(a == b);
        a[0] = 1;
        EXPECT(a != b);
    });

    // ── SNDP ──────────────────────────────────────────────────────────────
    r.add("encodeSndp: correct 10-byte frame structure", []() {
        // Filter 1 Cutoff = SDATA index 62, value 100, device 0, sound mode
        auto f = mw2xt::encodeSndp(0x00, 0x00, 62, 100);
        EXPECT_EQ(f.size(), 10u);
        EXPECT_EQ(f[0], 0xF0u);  // SysEx begin
        EXPECT_EQ(f[1], 0x3Eu);  // Waldorf mfr
        EXPECT_EQ(f[2], 0x0Eu);  // MW2/XT device type
        EXPECT_EQ(f[3], 0x00u);  // device ID
        EXPECT_EQ(f[4], 0x20u);  // IDM = SNDP
        EXPECT_EQ(f[5], 0x00u);  // LL = sound mode edit buffer
        EXPECT_EQ(f[6], 0x00u);  // HH = 0x00 for index < 128
        EXPECT_EQ(f[7], 62u);    // PP = param index
        EXPECT_EQ(f[8], 100u);   // XX = value
        EXPECT_EQ(f[9], 0xF7u);  // SysEx end
    });

    r.add("encodeSndp: HH=0x01 for index >= 128", []() {
        // Mod 3 Source = SDATA index 200; 200 & 0x7F = 72
        auto f = mw2xt::encodeSndp(0x00, 0x00, 200, 5);
        EXPECT_EQ(f[6], 0x01u);
        EXPECT_EQ(f[7], 72u);
        EXPECT_EQ(f[8], 5u);
    });

    r.add("encodeSndp: index 127 uses HH=0x00 PP=0x7F", []() {
        auto f = mw2xt::encodeSndp(0x00, 0x00, 127, 0);
        EXPECT_EQ(f[6], 0x00u);
        EXPECT_EQ(f[7], 127u);
    });

    r.add("encodeSndp: index 128 uses HH=0x01 PP=0x00 (boundary)", []() {
        auto f = mw2xt::encodeSndp(0x00, 0x00, 128, 0);
        EXPECT_EQ(f[6], 0x01u);
        EXPECT_EQ(f[7], 0x00u);
    });

    r.add("encodeSndp: index 255 uses HH=0x01 PP=0x7F", []() {
        auto f = mw2xt::encodeSndp(0x00, 0x00, 255, 127);
        EXPECT_EQ(f[6], 0x01u);
        EXPECT_EQ(f[7], 127u);
    });

    // ── MULP ──────────────────────────────────────────────────────────────
    r.add("encodeMulp: IDM byte is 0x21 not 0x20", []() {
        auto f = mw2xt::encodeMulp(0x00, 0x20, 0x05, 42);
        EXPECT_EQ(f.size(), 9u);
        EXPECT_EQ(f[0], 0xF0u);
        EXPECT_EQ(f[1], 0x3Eu);
        EXPECT_EQ(f[2], 0x0Eu);
        EXPECT_EQ(f[3], 0x00u);
        EXPECT_EQ(f[4], 0x21u);  // MULP IDM — NOT 0x20
        EXPECT_EQ(f[5], 0x20u);  // LL = multi edit buffer
        EXPECT_EQ(f[6], 0x05u);  // PP
        EXPECT_EQ(f[7], 42u);    // XX
        EXPECT_EQ(f[8], 0xF7u);
    });

    // ── Checksum ──────────────────────────────────────────────────────────

    r.add("computeChecksum: basic sum & 0x7F", []() {
        uint8_t data[] = {0x01, 0x02, 0x03};
        EXPECT_EQ(mw2xt::computeChecksum(data, 3), 0x06u);
    });

    r.add("computeChecksum: overflow masks correctly", []() {
        // 3 * 0x7F = 0x17D; 0x17D & 0x7F = 0x7D
        uint8_t data[] = {0x7F, 0x7F, 0x7F};
        EXPECT_EQ(mw2xt::computeChecksum(data, 3), 0x7Du);
    });

    r.add("checksumValid: 0x7F is universally valid (bypass sentinel)", []() {
        EXPECT(mw2xt::checksumValid(0x42, 0x7F));
    });

    r.add("checksumValid: matching checksum is valid", []() {
        EXPECT(mw2xt::checksumValid(0x42, 0x42));
    });

    r.add("checksumValid: mismatched checksum is invalid", []() {
        EXPECT(!mw2xt::checksumValid(0x42, 0x43));
    });

    // ── SNDD round-trips (≥5 patches) ────────────────────────────────────

    auto makeSound = [](uint8_t fill, uint8_t cutoff, uint8_t oct) {
        mw2xt::SoundData sd;
        sd.data.fill(fill & 0x7F);
        sd[62] = cutoff;  // SDATA index 62 = Filter 1 Cutoff (Waldorf §3.1)
        sd[1]  = oct;     // SDATA index 1  = Osc 1 Octave
        return sd;
    };

    r.add("SNDD round-trip: patch A (fill=0, cutoff=64, oct=2)", [&]() {
        auto sd = makeSound(0, 64, 2);
        auto frame = mw2xt::encodeSndd(0x00, 0x20, 0x00, sd);
        EXPECT_EQ(frame.size(), mw2xt::kSnddFrameSize);
        auto decoded = mw2xt::decodeSndd(frame);
        EXPECT(decoded.has_value());
        EXPECT(*decoded == sd);
    });

    r.add("SNDD round-trip: patch B (fill=1, cutoff=0, oct=0)", [&]() {
        auto sd = makeSound(1, 0, 0);
        EXPECT(*mw2xt::decodeSndd(mw2xt::encodeSndd(0x00, 0x20, 0x00, sd)) == sd);
    });

    r.add("SNDD round-trip: patch C (fill=0x7F, cutoff=127, oct=5)", [&]() {
        auto sd = makeSound(0x7F, 127, 5);
        EXPECT(*mw2xt::decodeSndd(mw2xt::encodeSndd(0x00, 0x20, 0x00, sd)) == sd);
    });

    r.add("SNDD round-trip: patch D (all zeros)", [&]() {
        mw2xt::SoundData sd;
        EXPECT(*mw2xt::decodeSndd(mw2xt::encodeSndd(0x00, 0x00, 0x00, sd)) == sd);
    });

    r.add("SNDD round-trip: patch E (alternating 0x55/0x2A)", [&]() {
        mw2xt::SoundData sd;
        for (size_t i = 0; i < 256; ++i)
            sd.data[i] = (i % 2 == 0) ? 0x55 : 0x2A;
        EXPECT(*mw2xt::decodeSndd(mw2xt::encodeSndd(0x00, 0x01, 0x00, sd)) == sd);
    });

    r.add("SNDD decode: rejects wrong frame size", []() {
        EXPECT(!mw2xt::decodeSndd(std::vector<uint8_t>(264, 0)).has_value());
    });

    r.add("SNDD decode: rejects bad checksum", []() {
        mw2xt::SoundData sd;
        auto f = mw2xt::encodeSndd(0x00, 0x20, 0x00, sd);
        f[263] = (f[263] == 0x7E) ? 0x01u : 0x7Eu;  // corrupt (avoid 0x7F bypass)
        EXPECT(!mw2xt::decodeSndd(f).has_value());
    });

    r.add("SNDD decode: accepts 0x7F checksum bypass", []() {
        mw2xt::SoundData sd;
        auto f = mw2xt::encodeSndd(0x00, 0x20, 0x00, sd);
        f[263] = 0x7F;
        EXPECT(mw2xt::decodeSndd(f).has_value());
    });

    return r.run();
}
