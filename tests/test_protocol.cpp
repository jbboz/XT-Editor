#include "TestRunner.h"
#include "mw2xtLib/SoundData.h"
#include "mw2xtLib/MultiData.h"
#include "mw2xtLib/GlobalData.h"

int main() {
    TestRunner r;

    r.add("SoundData is 256 bytes", []() {
        EXPECT_EQ(sizeof(mw2xt::SoundData), 256u);
    });

    r.add("InstrumentData is 28 bytes", []() {
        EXPECT_EQ(sizeof(mw2xt::InstrumentData), 28u);
    });

    r.add("MultiData is 256 bytes (32 MDATA + 8x28 IDATA)", []() {
        EXPECT_EQ(sizeof(mw2xt::MultiData), 256u);
    });

    r.add("GlobalData is 32 bytes", []() {
        EXPECT_EQ(sizeof(mw2xt::GlobalData), 32u);
    });

    return r.run();
}
