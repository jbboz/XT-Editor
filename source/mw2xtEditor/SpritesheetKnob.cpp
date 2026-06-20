#include "SpritesheetKnob.h"

namespace mw2xt {

SpritesheetKnob::SpritesheetKnob(juce::Image s, int fc)
    : sheet(std::move(s)), frameCount(fc)
{
    setRepaintsOnMouseActivity(false);
}

void SpritesheetKnob::setNormalized(double v, bool sendCallback) {
    value = juce::jlimit(0.0, 1.0, v);
    repaint();
    if (sendCallback && onValueChanged)
        onValueChanged(value);
}

void SpritesheetKnob::paint(juce::Graphics& g) {
    if (!sheet.isValid() || frameCount <= 0) {
        g.fillAll(juce::Colours::darkgrey);
        return;
    }
    const int frameH = sheet.getHeight() / frameCount;
    const int frame  = juce::jlimit(0, frameCount - 1,
                                    juce::roundToInt(value * (frameCount - 1)));
    g.drawImage(sheet,
                0, 0, getWidth(), getHeight(),
                0, frame * frameH, sheet.getWidth(), frameH);
}

void SpritesheetKnob::mouseDown(const juce::MouseEvent& e) {
    mouseDownY = e.getScreenY();
}

void SpritesheetKnob::mouseDrag(const juce::MouseEvent& e) {
    const int dy = mouseDownY - e.getScreenY();
    mouseDownY   = e.getScreenY();
    setNormalized(value + dy * (1.0 / 200.0), true);
}

void SpritesheetKnob::mouseWheelMove(const juce::MouseEvent&,
                                     const juce::MouseWheelDetails& w) {
    setNormalized(value + w.deltaY * (1.0 / 20.0), true);
}

} // namespace mw2xt
