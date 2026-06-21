#include "PageContainer.h"

namespace mw2xt {

void PageContainer::addPage(PageId id, std::unique_ptr<juce::Component> page) {
    if (page == nullptr)
        return;
    page->setVisible(false);
    page->setBounds(getLocalBounds());   // resized() re-applies once bounds are set
    addChildComponent(*page);            // adds as a hidden child
    slots.push_back({ id, std::move(page) });
}

void PageContainer::showPage(PageId id) {
    current = id;
    for (auto& s : slots)
        s.page->setVisible(s.id == id);
}

bool PageContainer::hasPage(PageId id) const noexcept {
    for (const auto& s : slots)
        if (s.id == id)
            return true;
    return false;
}

juce::Component* PageContainer::getPageForTest(PageId id) const noexcept {
    for (const auto& s : slots)
        if (s.id == id)
            return s.page.get();
    return nullptr;
}

void PageContainer::resized() {
    for (auto& s : slots)
        s.page->setBounds(getLocalBounds());
}

} // namespace mw2xt
