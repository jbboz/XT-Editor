#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace mw2xt {

// Toggle button rendered from a 2-frame vertical sprite strip.
// Frame 0 = off, frame 1 = on.
class SpritesheetButton : public juce::Component {
public:
    SpritesheetButton(juce::Image sheet, int frameCount = 2);

    // sendCallback = true fires onToggled.
    void setToggleState(bool on, bool sendCallback = false);
    bool getToggleState() const noexcept { return toggled; }

    std::function<void(bool)> onToggled;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::Image sheet;
    int         frameCount;
    bool        toggled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpritesheetButton)
};

} // namespace mw2xt
