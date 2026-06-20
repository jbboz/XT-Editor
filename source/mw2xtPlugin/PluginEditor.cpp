#include "PluginEditor.h"
#include <BinaryData.h>

namespace mw2xt
{

// ── Helpers ──────────────────────────────────────────────────────────────────

static juce::Image loadPng(const char* data, int size)
{
    return juce::ImageFileFormat::loadFrom(data, static_cast<size_t>(size));
}

// ── Construction ─────────────────────────────────────────────────────────────

EditorComponent::EditorComponent(EditorProcessor& proc)
    : juce::AudioProcessorEditor(&proc), proc(proc)
{
    // ── Load skin images ──────────────────────────────────────────────────────
    rootBG = loadPng(BinaryData::xtDefaultBG_png, BinaryData::xtDefaultBG_pngSize);

    skinImages.knobSheet        = loadPng(BinaryData::xt_encoder_ranged_png,
                                          BinaryData::xt_encoder_ranged_pngSize);
    skinImages.knobRedSheet     = loadPng(BinaryData::xt_encoder_ranged_red_png,
                                          BinaryData::xt_encoder_ranged_red_pngSize);
    skinImages.buttonSheet      = loadPng(BinaryData::xtknob_png,
                                          BinaryData::xtknob_pngSize);
    skinImages.buttonSmallSheet = loadPng(BinaryData::xtknob_small_png,
                                          BinaryData::xtknob_small_pngSize);
    skinImages.ledSheet         = loadPng(BinaryData::led_png,
                                          BinaryData::led_pngSize);

    // ── Load param registry from embedded JSON ────────────────────────────────
    paramRegistry.loadFromBinaryData(BinaryData::parameterDescriptions_xt_json,
                                     BinaryData::parameterDescriptions_xt_jsonSize);

    // ── Parse skin layout and build pageOsc ───────────────────────────────────
    juce::var skinJson;
    {
        const juce::String skinText(BinaryData::xtDefault_json,
                                    static_cast<size_t>(BinaryData::xtDefault_jsonSize));
        skinJson = juce::JSON::parse(skinText);
    }

    // Find pageOsc node in the skin tree.
    std::function<juce::var(const juce::var&, const juce::String&)> findPage =
        [&](const juce::var& node, const juce::String& name) -> juce::var {
            if (node["name"].toString() == name)
                return node;
            if (const auto* ch = node["children"].getArray())
                for (const auto& c : *ch) {
                    auto r = findPage(c, name);
                    if (!r.isVoid())
                        return r;
                }
            return {};
        };

    const juce::var pageOscNode = findPage(skinJson, "pageOsc");
    const juce::Image oscBg = loadPng(BinaryData::xtPageOsc_png,
                                      BinaryData::xtPageOsc_pngSize);

    pageOsc = std::make_unique<PageComponent>(
        proc.getController(), paramRegistry, pageOscNode, oscBg, skinImages);
    addAndMakeVisible(*pageOsc);

    // Root skin: 3400×2000 at scale 0.5 → 1700×1000 display pixels.
    // Widget positions are baked from the skin JSON at construction, so the
    // window must stay at this size (rescaling widgets is a later milestone).
    setResizable(false, false);
    setSize(1700, 1000);
}

// ── Layout ───────────────────────────────────────────────────────────────────

void EditorComponent::resized()
{
    // pageOsc image in skin: x=36, y=489, w=3322.7, h=1479.8 at scale=0.5
    if (pageOsc)
        pageOsc->setBounds(18, 244, 1661, 740);
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void EditorComponent::paint(juce::Graphics& g)
{
    if (rootBG.isValid())
        g.drawImage(rootBG, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0x12, 0x12, 0x12));
}

} // namespace mw2xt
