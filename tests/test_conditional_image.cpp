#include "TestRunner.h"
#include "mw2xtEditor/ConditionalImage.h"
#include <juce_gui_basics/juce_gui_basics.h>

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    TestRunner r;

    r.add("hidden by default", []() {
        mw2xt::ConditionalImage c(juce::Image(), 64, {6});
        EXPECT(!c.isVisible());
    });
    r.add("visible when value matches", []() {
        mw2xt::ConditionalImage c(juce::Image(), 64, {6});
        c.refreshFromValue(6);
        EXPECT(c.isVisible());
    });
    r.add("hidden when value does not match", []() {
        mw2xt::ConditionalImage c(juce::Image(), 64, {6});
        c.refreshFromValue(6);
        c.refreshFromValue(5);
        EXPECT(!c.isVisible());
    });
    r.add("multiple enable values", []() {
        mw2xt::ConditionalImage c(juce::Image(), 64, {6, 7, 8});
        c.refreshFromValue(8);
        EXPECT(c.isVisible());
        c.refreshFromValue(9);
        EXPECT(!c.isVisible());
    });
    r.add("getSdataIndex returns ctor index", []() {
        mw2xt::ConditionalImage c(juce::Image(), 64, {6});
        EXPECT_EQ(c.getSdataIndex(), 64);
    });

    r.add("parseEnableValues: single value", []() {
        const auto v = mw2xt::parseEnableValues("6");
        EXPECT_EQ(v.size(), (size_t) 1);
        EXPECT_EQ(v[0], 6);
    });
    r.add("parseEnableValues: space-separated", []() {
        const auto v = mw2xt::parseEnableValues("6 7 8");
        EXPECT_EQ(v.size(), (size_t) 3);
        EXPECT_EQ(v[2], 8);
    });
    r.add("parseEnableValues: empty string", []() {
        EXPECT(mw2xt::parseEnableValues("").empty());
    });

    return r.run();
}
