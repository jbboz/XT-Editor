#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace mw2xt {

enum class PageId { Main, Mod, Arp, Multi, Wave, PatchManager };

// Owns one child page per PageId and shows exactly one at a time.
// Switching is setVisible only — pages are never rebuilt or re-laid-out.
class PageContainer : public juce::Component {
public:
    PageContainer() = default;

    void   addPage(PageId id, std::unique_ptr<juce::Component> page);
    void   showPage(PageId id);
    PageId currentPage() const noexcept { return current; }
    bool   hasPage(PageId id) const noexcept;

    // Test accessor — returns the owned page for an id, or nullptr.
    juce::Component* getPageForTest(PageId id) const noexcept;

    void resized() override;

private:
    struct Slot { PageId id; std::unique_ptr<juce::Component> page; };
    std::vector<Slot> slots;
    PageId            current = PageId::Main;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageContainer)
};

} // namespace mw2xt
