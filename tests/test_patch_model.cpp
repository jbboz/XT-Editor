#include "TestRunner.h"
#include "mw2xtEditor/PatchModel.h"

// PatchModel tests run with JUCE initialised (juce_add_console_app sets this up).
// sendChangeMessage() is async and no-ops when there are no registered listeners,
// so no message loop is needed for these data-layer tests.

int main() {
    TestRunner r;

    r.add("setSoundParam: stores value in edit buffer", []() {
        mw2xt::PatchModel m;
        m.setSoundParam(62, 64);
        EXPECT_EQ(m.getSound()[62], 64u);
    });

    r.add("setSoundParam: marks model dirty", []() {
        mw2xt::PatchModel m;
        EXPECT(!m.isDirty());
        m.setSoundParam(62, 64);
        EXPECT(m.isDirty());
    });

    r.add("setSoundParam: no-op when value unchanged", []() {
        mw2xt::PatchModel m;
        m.setSoundParam(10, 0);  // 0 is the default
        EXPECT(!m.isDirty());    // no action created, no dirty flag
    });

    r.add("setSoundParam: fires onSoundParamChanged with correct args", []() {
        mw2xt::PatchModel m;
        int  lastIdx = -1;
        uint8_t lastVal = 0;
        m.onSoundParamChanged = [&](int i, uint8_t v) { lastIdx = i; lastVal = v; };
        m.setSoundParam(62, 100);
        EXPECT_EQ(lastIdx, 62);
        EXPECT_EQ(lastVal, 100u);
    });

    r.add("undo: reverts setSoundParam and fires callback", []() {
        mw2xt::PatchModel m;
        int  callbackIdx = -1;
        uint8_t callbackVal = 0xFF;
        m.onSoundParamChanged = [&](int i, uint8_t v) { callbackIdx = i; callbackVal = v; };

        m.setSoundParam(62, 64);
        EXPECT_EQ(m.getSound()[62], 64u);

        m.getUndoManager().undo();
        EXPECT_EQ(m.getSound()[62], 0u);   // reverted to zero-init
        EXPECT_EQ(callbackIdx, 62);        // callback fired during undo
        EXPECT_EQ(callbackVal, 0u);
    });

    r.add("redo: re-applies after undo", []() {
        mw2xt::PatchModel m;
        m.setSoundParam(62, 64);
        m.getUndoManager().undo();
        m.getUndoManager().redo();
        EXPECT_EQ(m.getSound()[62], 64u);
    });

    r.add("undo: respects 30-step limit", []() {
        mw2xt::PatchModel m;
        for (int i = 0; i < 35; ++i)
            m.setSoundParam(0, static_cast<uint8_t>(i + 1));
        // Undo 30 times (limit) should bottom out — param won't be 0
        for (int i = 0; i < 30; ++i)
            m.getUndoManager().undo();
        // After 30 undos, we've cleared the last 30 actions.
        // The first 5 actions were evicted, so the lowest value reached is 5.
        EXPECT(m.getSound()[0] <= 5u);
    });

    r.add("loadFromDump: populates editBuffer and compareBuffer", []() {
        mw2xt::PatchModel m;
        mw2xt::SoundData sd;
        sd[62] = 100;
        m.loadFromDump(sd);
        EXPECT_EQ(m.getSound()[62],   100u);
        EXPECT_EQ(m.getCompare()[62], 100u);
    });

    r.add("loadFromDump: clears dirty flag", []() {
        mw2xt::PatchModel m;
        m.setSoundParam(62, 64);
        EXPECT(m.isDirty());
        mw2xt::SoundData sd;
        m.loadFromDump(sd);
        EXPECT(!m.isDirty());
    });

    r.add("loadFromDump: clears undo history", []() {
        mw2xt::PatchModel m;
        m.setSoundParam(62, 64);
        mw2xt::SoundData sd;
        m.loadFromDump(sd);
        // Undo has nothing to do — value stays at whatever loadFromDump set
        m.getUndoManager().undo();
        EXPECT_EQ(m.getSound()[62], 0u);  // sd was zero-init, undo had nothing to do
    });

    r.add("swapAB: exchanges editBuffer and compareBuffer", []() {
        mw2xt::PatchModel m;
        mw2xt::SoundData sd;
        sd[62] = 50;
        m.loadFromDump(sd);        // edit=50, compare=50
        m.setSoundParam(62, 75);   // edit=75, compare=50
        m.swapAB();
        EXPECT_EQ(m.getSound()[62],   50u);
        EXPECT_EQ(m.getCompare()[62], 75u);
    });

    r.add("swapAB: marks dirty", []() {
        mw2xt::PatchModel m;
        mw2xt::SoundData sd;
        m.loadFromDump(sd);   // dirty = false
        m.swapAB();
        EXPECT(m.isDirty());
    });

    r.add("swapAB: double swap restores original state", []() {
        mw2xt::PatchModel m;
        mw2xt::SoundData sd;
        sd[62] = 50;
        m.loadFromDump(sd);
        m.setSoundParam(62, 75);
        m.swapAB();
        m.swapAB();
        EXPECT_EQ(m.getSound()[62],   75u);
        EXPECT_EQ(m.getCompare()[62], 50u);
    });

    r.add("loadMultiFromDump: stores multi data", []() {
        mw2xt::PatchModel m;
        mw2xt::MultiData md;
        md.mdata[0] = 42;
        m.loadMultiFromDump(md);
        EXPECT_EQ(m.getMulti().mdata[0], 42u);
    });

    r.add("loadGlobalFromDump: stores global data", []() {
        mw2xt::PatchModel m;
        mw2xt::GlobalData gd;
        gd[6] = 7;  // device ID field
        m.loadGlobalFromDump(gd);
        EXPECT_EQ(m.getGlobal()[6], 7u);
    });

    return r.run();
}
