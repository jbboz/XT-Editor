#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace mw2xt {

// Rotary knob rendered from a vertical sprite strip.
// Drag up/down or use the scroll wheel to change value.
class SpritesheetKnob : public juce::Component {
public:
    SpritesheetKnob(juce::Image sheet, int frameCount);

    // v must be in [0, 1].  sendCallback = true fires onValueChanged.
    void   setNormalized(double v, bool sendCallback = false);
    double getNormalized() const noexcept { return value; }

    std::function<void(double)> onValueChanged;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Image sheet;
    int         frameCount;
    double      value      = 0.0;
    int         mouseDownY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpritesheetKnob)
};

} // namespace mw2xt
