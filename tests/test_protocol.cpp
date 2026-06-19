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

    return r.run();
}
