#include "PageComponent.h"
#include "EditorController.h"
#include <cmath>

namespace mw2xt {

// ── Helpers ──────────────────────────────────────────────────────────────────

static constexpr float kSkinScale = 0.5f;

static float skinToDisplay(const juce::var& v) {
    return v.toString().getFloatValue() * kSkinScale;
}

// ── Construction ─────────────────────────────────────────────────────────────

PageComponent::PageComponent(EditorController&    ctrl,
                             const ParamRegistry& reg,
                             const juce::var&     pageNode,
                             juce::Image          bg,
                             const SkinImages&    skins)
    : controller(ctrl), bgImage(std::move(bg))
{
    buildWidgets(pageNode, reg, skins);
    ctrl.getModel().addChangeListener(this);
    refreshFromModel();
}

PageComponent::~PageComponent() {
    controller.getModel().removeChangeListener(this);
}

// ── Model sync ───────────────────────────────────────────────────────────────

void PageComponent::changeListenerCallback(juce::ChangeBroadcaster*) {
    refreshFromModel();
}

void PageComponent::refreshFromModel() {
    const auto& sound = controller.getModel().getSound();
    for (const auto& e : entries) {
        if (e.sdataIndex < 0)
            continue;
        const uint8_t raw = sound[e.sdataIndex];
        if (e.type == WidgetEntry::Knob) {
            if (auto* k = static_cast<SpritesheetKnob*>(e.component)) {
                const double norm = static_cast<double>(raw - e.minVal)
                                  / std::max(1, e.maxVal - e.minVal);
                k->setNormalized(juce::jlimit(0.0, 1.0, norm));
            }
        } else if (e.type == WidgetEntry::Button) {
            if (auto* b = static_cast<SpritesheetButton*>(e.component))
                b->setToggleState(raw != 0);
        } else if (e.type == WidgetEntry::Combo) {
            if (auto* c = static_cast<juce::ComboBox*>(e.component)) {
                int id = 1;
                for (int i = 0; i < static_cast<int>(e.itemValues.size()); ++i) {
                    if (e.itemValues[static_cast<size_t>(i)] == static_cast<int>(raw)) {
                        id = i + 1;
                        break;
                    }
                }
                c->setSelectedId(id, juce::dontSendNotification);
            }
        } else if (e.type == WidgetEntry::Conditional) {
            if (auto* ci = static_cast<ConditionalImage*>(e.component))
                ci->refreshFromValue(raw);
        }
    }
}

// ── Paint ────────────────────────────────────────────────────────────────────

void PageComponent::paint(juce::Graphics& g) {
    if (bgImage.isValid())
        g.drawImage(bgImage, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0x18, 0x18, 0x18));
}

// ── Widget building ──────────────────────────────────────────────────────────

void PageComponent::buildWidgets(const juce::var&     pageNode,
                                 const ParamRegistry& reg,
                                 const SkinImages&    skins) {
    const auto* children = pageNode["children"].getArray();
    if (!children)
        return;

    for (const auto& child : *children) {
        const juce::String paramName =
            child["parameterAttachment"]["parameter"].toString();
        const ParamInfo* info = reg.getInfo(paramName);

        // ── Rotary knob ───────────────────────────────────────────────────────
        if (child.hasProperty("rotary") && child.hasProperty("spritesheet")) {
            const auto& ss = child["spritesheet"];
            const int x = juce::roundToInt(skinToDisplay(ss["x"]));
            const int y = juce::roundToInt(skinToDisplay(ss["y"]));
            const int w = juce::roundToInt(skinToDisplay(ss["width"]));
            const int h = juce::roundToInt(skinToDisplay(ss["height"]));
            const juce::String tex = ss["texture"].toString();
            const auto& sheet =
                (tex == "xt_encoder_ranged_red") ? skins.knobRedSheet : skins.knobSheet;

            auto* knob = knobs.add(std::make_unique<SpritesheetKnob>(sheet, 90));
            knob->setBounds(x, y, w, h);
            addAndMakeVisible(knob);

            WidgetEntry e;
            e.type      = WidgetEntry::Knob;
            e.component = knob;
            knob->onGestureStarted = [this]() {
                controller.getModel().getUndoManager().beginNewTransaction();
            };
            if (info) {
                e.sdataIndex = info->sdataIndex;
                e.minVal     = info->minVal;
                e.maxVal     = info->maxVal;
                const int sdataIdx = info->sdataIndex;
                const int minV     = info->minVal;
                const int maxV     = info->maxVal;
                knob->onValueChanged = [this, sdataIdx, minV, maxV](double norm) {
                    const auto v = static_cast<uint8_t>(
                        std::round(norm * static_cast<double>(maxV - minV) + minV));
                    controller.setSoundParam(sdataIdx, v);
                };
            }
            entries.push_back(std::move(e));
        }
        // ── Button / LED ──────────────────────────────────────────────────────
        else if (child.hasProperty("button")) {
            const auto& bt  = child["button"];
            const juce::String tex = bt["texture"].toString();

            // Skip inc/dec buttons (handled in later milestone).
            if (tex == "xtknob_minus" || tex == "xtknob_plus")
                continue;

            juce::Image sheet;
            if (tex == "xtknob")            sheet = skins.buttonSheet;
            else if (tex == "xtknob_small") sheet = skins.buttonSmallSheet;
            else if (tex == "led")          sheet = skins.ledSheet;
            else continue;

            const int x = juce::roundToInt(skinToDisplay(bt["x"]));
            const int y = juce::roundToInt(skinToDisplay(bt["y"]));
            const int w = juce::roundToInt(skinToDisplay(bt["width"]));
            const int h = juce::roundToInt(skinToDisplay(bt["height"]));
            const bool isToggle = (bt["isToggle"].toString() == "1");

            auto* btn = buttons.add(std::make_unique<SpritesheetButton>(sheet, 2));
            btn->setBounds(x, y, w, h);
            addAndMakeVisible(btn);

            WidgetEntry e;
            e.type      = WidgetEntry::Button;
            e.component = btn;
            if (info) {
                e.sdataIndex = info->sdataIndex;
                e.minVal     = info->minVal;
                e.maxVal     = info->maxVal;
                if (isToggle) {
                    btn->onGestureStarted = [this]() {
                        controller.getModel().getUndoManager().beginNewTransaction();
                    };
                    const int sdataIdx = info->sdataIndex;
                    btn->onToggled = [this, sdataIdx](bool on) {
                        controller.setSoundParam(sdataIdx, on ? 1u : 0u);
                    };
                }
            }
            entries.push_back(std::move(e));
        }
        // ── ComboBox ──────────────────────────────────────────────────────────
        else if (child.hasProperty("combobox")) {
            const auto& cb = child["combobox"];
            const int x = juce::roundToInt(skinToDisplay(cb["x"]));
            const int y = juce::roundToInt(skinToDisplay(cb["y"]));
            const int w = juce::roundToInt(skinToDisplay(cb["width"]));
            const int h = juce::roundToInt(skinToDisplay(cb["height"]));

            auto* combo = combos.add(std::make_unique<juce::ComboBox>());
            combo->setBounds(x, y, w, h);
            addAndMakeVisible(combo);

            WidgetEntry e;
            e.type      = WidgetEntry::Combo;
            e.component = combo;
            if (info && !info->items.isEmpty()) {
                e.sdataIndex = info->sdataIndex;
                e.minVal     = info->minVal;
                e.maxVal     = info->maxVal;
                e.itemValues = info->itemParamValues;
                for (int i = 0; i < info->items.size(); ++i)
                    combo->addItem(info->items[i], i + 1);

                const int            sdataIdx  = info->sdataIndex;
                const std::vector<int> itemVals = info->itemParamValues;
                combo->onChange = [this, combo, sdataIdx, itemVals]() {
                    controller.getModel().getUndoManager().beginNewTransaction();
                    const int idx = combo->getSelectedItemIndex();
                    if (idx >= 0 && idx < static_cast<int>(itemVals.size()))
                        controller.setSoundParam(sdataIdx,
                                                 static_cast<uint8_t>(itemVals[static_cast<size_t>(idx)]));
                };
            }
            entries.push_back(std::move(e));
        }
        // ── Conditional image overlay (e.g. F1Type filter icon) ───────────────
        else if (child.hasProperty("image") && child.hasProperty("condition")) {
            const auto& img  = child["image"];
            const auto& cond = child["condition"];
            const ParamInfo* condInfo =
                reg.getInfo(cond["enableOnParameter"].toString());
            if (!condInfo)
                continue;

            const juce::String tex = img["texture"].toString();
            const auto it = skins.namedImages.find(tex);
            if (it == skins.namedImages.end())
                continue;

            const int x = juce::roundToInt(skinToDisplay(img["x"]));
            const int y = juce::roundToInt(skinToDisplay(img["y"]));
            const int w = juce::roundToInt(skinToDisplay(img["width"]));
            const int h = juce::roundToInt(skinToDisplay(img["height"]));

            auto values = parseEnableValues(cond["enableOnValues"].toString());
            auto* ci = conditionals.add(std::make_unique<ConditionalImage>(
                it->second, condInfo->sdataIndex, std::move(values)));
            ci->setBounds(x, y, w, h);
            addAndMakeVisible(ci);

            WidgetEntry e;
            e.type       = WidgetEntry::Conditional;
            e.sdataIndex = condInfo->sdataIndex;
            e.component  = ci;
            entries.push_back(std::move(e));
        }
    }
}

} // namespace mw2xt
