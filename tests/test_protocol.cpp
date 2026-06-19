#include "TestRunner.h"
#include "mw2xtLib/SoundData.h"
#include "mw2xtLib/MultiData.h"
#include "mw2xtLib/GlobalData.h"

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

    return r.run();
}
