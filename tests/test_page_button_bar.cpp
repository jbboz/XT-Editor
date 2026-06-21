#include "TestRunner.h"
#include "mw2xtEditor/PageButtonBar.h"
#include <juce_gui_basics/juce_gui_basics.h>

using mw2xt::PageId;
using Def = mw2xt::PageButtonBar::ButtonDef;

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    TestRunner r;

    auto makeBar = []() {
        std::vector<Def> defs {
            { PageId::Main, { 0, 0, 64, 64 }, true },
            { PageId::Arp,  { 70, 0, 64, 64 }, true },
        };
        return std::make_unique<mw2xt::PageButtonBar>(juce::Image(), std::move(defs));
    };

    r.add("selectPage updates selectedPage", [&]() {
        auto bar = makeBar();
        bar->selectPage(PageId::Arp);
        EXPECT(bar->selectedPage() == PageId::Arp);
    });

    r.add("selectPage is exclusive (radio)", [&]() {
        auto bar = makeBar();
        bar->selectPage(PageId::Main);
        EXPECT(bar->getToggleState(PageId::Main));
        EXPECT(!bar->getToggleState(PageId::Arp));
        bar->selectPage(PageId::Arp);
        EXPECT(!bar->getToggleState(PageId::Main));
        EXPECT(bar->getToggleState(PageId::Arp));
    });

    return r.run();
}
