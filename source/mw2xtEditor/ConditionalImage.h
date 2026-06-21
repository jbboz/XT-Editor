#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace mw2xt {

// Pure visual indicator: shows `img` only when the observed parameter's raw
// value is one of `enableValues`. No input, no undo, no MIDI.
class ConditionalImage : public juce::Component {
public:
    ConditionalImage(juce::Image img, int sdataIndex, std::vector<int> enableValues);

    int  getSdataIndex() const noexcept { return sdataIndex; }
    void refreshFromValue(uint8_t rawValue);   // setVisible(value in enableValues)

    void paint(juce::Graphics&) override;

private:
    juce::Image      image;
    int              sdataIndex;
    std::vector<int> enableValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConditionalImage)
};

// Parse a skin `enableOnValues` string ("6" or "6 7 8") into raw int values.
std::vector<int> parseEnableValues(const juce::String& spaceSeparated);

} // namespace mw2xt
