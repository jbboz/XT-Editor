#pragma once
#include <juce_core/juce_core.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace mw2xt {

struct ParamInfo {
    juce::String     name;
    int              sdataIndex    = -1;
    int              minVal        = 0;
    int              maxVal        = 127;
    juce::StringArray items;             // non-empty for discrete/combobox
    std::vector<int>  itemParamValues;   // itemParamValues[i] = raw param value for items[i]
};

class ParamRegistry {
public:
    // Parse parameterDescriptions_xt.json from binary data.
    // The JSON uses C++ // comments which are stripped before parsing.
    void loadFromBinaryData(const char* jsonData, int jsonSize);

    // Returns nullptr if name is not registered.
    const ParamInfo* getInfo(const juce::String& name) const noexcept;

private:
    std::unordered_map<std::string, ParamInfo> entries;
};

} // namespace mw2xt
