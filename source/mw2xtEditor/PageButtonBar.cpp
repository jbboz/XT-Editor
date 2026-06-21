#include "PageButtonBar.h"

namespace mw2xt {

PageButtonBar::PageButtonBar(juce::Image sheet, std::vector<ButtonDef> defs) {
    setInterceptsMouseClicks(false, true);   // pass clicks through to child buttons
    for (auto& d : defs) {
        auto btn = std::make_unique<SpritesheetButton>(sheet, 2);
        btn->setBounds(d.bounds);
        btn->setEnabled(d.enabled);
        const PageId id = d.id;
        if (d.enabled)
            btn->onToggled = [this, id](bool) { handleClick(id); };
        addAndMakeVisible(*btn);
        entries.push_back({ id, std::move(btn) });
    }
    selectPage(PageId::Main);
}

void PageButtonBar::handleClick(PageId id) {
    selectPage(id);
    if (onPageSelected)
        onPageSelected(id);
}

void PageButtonBar::selectPage(PageId id) {
    selected = id;
    for (auto& e : entries)
        e.btn->setToggleState(e.id == id, false);   // false = no callback
}

bool PageButtonBar::getToggleState(PageId id) const noexcept {
    for (const auto& e : entries)
        if (e.id == id)
            return e.btn->getToggleState();
    return false;
}

} // namespace mw2xt
