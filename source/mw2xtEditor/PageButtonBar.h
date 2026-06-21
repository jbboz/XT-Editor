#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <vector>
#include "PageContainer.h"      // PageId
#include "SpritesheetButton.h"

namespace mw2xt {

// Transparent overlay holding the skin's page buttons as a radio group.
class PageButtonBar : public juce::Component {
public:
    struct ButtonDef {
        PageId               id;
        juce::Rectangle<int> bounds;   // display coords (skin * 0.5)
        bool                 enabled;
    };

    PageButtonBar(juce::Image sheet, std::vector<ButtonDef> defs);

    void   selectPage(PageId id);                       // radio state only; no callback
    PageId selectedPage() const noexcept { return selected; }
    bool   getToggleState(PageId id) const noexcept;

    std::function<void(PageId)> onPageSelected;         // fired on user click

private:
    struct Entry { PageId id; std::unique_ptr<SpritesheetButton> btn; };
    std::vector<Entry> entries;
    PageId             selected = PageId::Main;

    void handleClick(PageId id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageButtonBar)
};

} // namespace mw2xt
