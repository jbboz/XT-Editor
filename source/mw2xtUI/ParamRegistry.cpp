#include "ParamRegistry.h"

namespace mw2xt {

void ParamRegistry::loadFromBinaryData(const char* jsonData, int jsonSize) {
    // Strip C++ // line comments before parsing (the JSON file uses them).
    juce::String cleaned;
    cleaned.preallocateBytes(static_cast<size_t>(jsonSize));
    const char* cur = jsonData;
    const char* end = jsonData + jsonSize;
    while (cur < end) {
        if (cur + 1 < end && cur[0] == '/' && cur[1] == '/') {
            while (cur < end && *cur != '\n')
                ++cur;
        } else {
            cleaned += *cur++;
        }
    }

    const auto root = juce::JSON::parse(cleaned);
    if (!root.isObject())
        return;

    // ── Build valuelist map ───────────────────────────────────────────────────
    using ValuePair = std::pair<int, std::string>;
    std::unordered_map<std::string, std::vector<ValuePair>> valueLists;

    if (const auto* vlObj = root["valuelists"].getDynamicObject()) {
        for (const auto& kv : vlObj->getProperties()) {
            const std::string key = kv.name.toString().toStdString();
            auto& list = valueLists[key];

            if (kv.value.isArray()) {
                int idx = 0;
                for (const auto& item : *kv.value.getArray())
                    list.emplace_back(idx++, item.toString().toStdString());
            } else if (kv.value.isObject()) {
                if (const auto* obj = kv.value.getDynamicObject()) {
                    for (const auto& entry : obj->getProperties())
                        list.emplace_back(entry.name.toString().getIntValue(),
                                          entry.value.toString().toStdString());
                    std::sort(list.begin(), list.end());
                }
            }
        }
    }

    // ── Parse parameterdescriptions array ─────────────────────────────────────
    const auto* paramsArr = root["parameterdescriptions"].getArray();
    if (!paramsArr)
        return;

    for (const auto& p : *paramsArr) {
        if (!p.isObject())
            continue;
        const juce::String name = p["name"].toString();
        if (name.isEmpty())
            continue;

        ParamInfo info;
        info.name       = name;
        info.sdataIndex = static_cast<int>(p["index"]);
        info.minVal     = p["min"].isVoid() ? 0   : static_cast<int>(p["min"]);
        info.maxVal     = p["max"].isVoid() ? 127 : static_cast<int>(p["max"]);

        const juce::String toText = p["toText"].toString();
        if (toText.isNotEmpty()) {
            auto it = valueLists.find(toText.toStdString());
            if (it != valueLists.end()) {
                for (const auto& [val, label] : it->second) {
                    info.items.add(juce::String(label));
                    info.itemParamValues.push_back(val);
                }
            }
        }

        entries[name.toStdString()] = std::move(info);
    }
}

const ParamInfo* ParamRegistry::getInfo(const juce::String& name) const noexcept {
    const auto it = entries.find(name.toStdString());
    return it != entries.end() ? &it->second : nullptr;
}

} // namespace mw2xt
