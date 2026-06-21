#include "ConditionalImage.h"
#include <algorithm>

namespace mw2xt {

ConditionalImage::ConditionalImage(juce::Image img, int idx, std::vector<int> vals)
    : image(std::move(img)), sdataIndex(idx), enableValues(std::move(vals))
{
    setInterceptsMouseClicks(false, false);
    setVisible(false);
}

void ConditionalImage::refreshFromValue(uint8_t rawValue) {
    const bool on = std::find(enableValues.begin(), enableValues.end(),
                              static_cast<int>(rawValue)) != enableValues.end();
    setVisible(on);
}

void ConditionalImage::paint(juce::Graphics& g) {
    if (image.isValid())
        g.drawImage(image, getLocalBounds().toFloat());
}

std::vector<int> parseEnableValues(const juce::String& s) {
    std::vector<int> out;
    juce::StringArray toks;
    toks.addTokens(s, " ", "");
    for (const auto& t : toks) {
        const auto trimmed = t.trim();
        if (trimmed.isNotEmpty())
            out.push_back(trimmed.getIntValue());
    }
    return out;
}

} // namespace mw2xt
