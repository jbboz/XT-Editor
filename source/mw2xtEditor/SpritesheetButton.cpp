#include "SpritesheetButton.h"

namespace mw2xt {

SpritesheetButton::SpritesheetButton(juce::Image s, int fc)
    : sheet(std::move(s)), frameCount(fc) {}

void SpritesheetButton::setToggleState(bool on, bool sendCallback) {
    toggled = on;
    repaint();
    if (sendCallback && onToggled)
        onToggled(toggled);
}

void SpritesheetButton::paint(juce::Graphics& g) {
    if (!sheet.isValid() || frameCount <= 0) {
        g.fillAll(toggled ? juce::Colours::orange : juce::Colours::darkgrey);
        return;
    }
    const int frameH = sheet.getHeight() / frameCount;
    const int frame  = (toggled && frameCount > 1) ? 1 : 0;
    g.drawImage(sheet,
                0, 0, getWidth(), getHeight(),
                0, frame * frameH, sheet.getWidth(), frameH);
}

void SpritesheetButton::mouseDown(const juce::MouseEvent&) {
    setToggleState(!toggled, true);
}

} // namespace mw2xt
