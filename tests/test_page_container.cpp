#include "TestRunner.h"
#include "mw2xtEditor/PageContainer.h"
#include <juce_gui_basics/juce_gui_basics.h>

using mw2xt::PageId;

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    TestRunner r;

    r.add("showPage makes exactly the chosen page visible", []() {
        mw2xt::PageContainer c;
        auto p1 = std::make_unique<juce::Component>(); auto* r1 = p1.get();
        auto p2 = std::make_unique<juce::Component>(); auto* r2 = p2.get();
        c.addPage(PageId::Main, std::move(p1));
        c.addPage(PageId::Arp,  std::move(p2));

        c.showPage(PageId::Main);
        EXPECT(r1->isVisible());
        EXPECT(!r2->isVisible());
        EXPECT(c.currentPage() == PageId::Main);

        c.showPage(PageId::Arp);
        EXPECT(!r1->isVisible());
        EXPECT(r2->isVisible());
        EXPECT(c.currentPage() == PageId::Arp);
    });

    r.add("hasPage reflects added pages", []() {
        mw2xt::PageContainer c;
        c.addPage(PageId::Main, std::make_unique<juce::Component>());
        EXPECT(c.hasPage(PageId::Main));
        EXPECT(!c.hasPage(PageId::Multi));
    });

    r.add("at most one page visible after any switch", []() {
        mw2xt::PageContainer c;
        auto* a = (c.addPage(PageId::Main, std::make_unique<juce::Component>()),
                   c.getPageForTest(PageId::Main));
        auto* b = (c.addPage(PageId::Mod, std::make_unique<juce::Component>()),
                   c.getPageForTest(PageId::Mod));
        c.showPage(PageId::Mod);
        int visible = (a->isVisible() ? 1 : 0) + (b->isVisible() ? 1 : 0);
        EXPECT_EQ(visible, 1);
    });

    return r.run();
}
