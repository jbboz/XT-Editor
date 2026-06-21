#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <map>
#include "SpritesheetKnob.h"
#include "SpritesheetButton.h"
#include "../mw2xtUI/ParamRegistry.h"
#include "ConditionalImage.h"

namespace mw2xt {

class EditorController;

// Renders one skin page: draws background image, instantiates child widgets
// from a parsed xtDefault.json page node, and syncs to/from PatchModel.
class PageComponent : public juce::Component, public juce::ChangeListener {
public:
    struct SkinImages {
        juce::Image knobSheet;         // xt_encoder_ranged.png    (128×11520, 90 frames)
        juce::Image knobRedSheet;      // xt_encoder_ranged_red.png (128×11520, 90 frames)
        juce::Image buttonSheet;       // xtknob.png               (128×256,   2 frames)
        juce::Image buttonSmallSheet;  // xtknob_small.png         (64×128,    2 frames)
        juce::Image ledSheet;          // led.png                  (128×256,   2 frames)
        std::map<juce::String, juce::Image> namedImages; // keyed by skin texture name
    };

    PageComponent(EditorController&    ctrl,
                  const ParamRegistry& reg,
                  const juce::var&     pageNode,
                  juce::Image          bgImage,
                  const SkinImages&    skins);
    ~PageComponent() override;

    // Pull current model state into all widgets (no callbacks fired).
    void refreshFromModel();

    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void paint(juce::Graphics&) override;

private:
    struct WidgetEntry {
        enum Type { Knob, Button, Combo, Conditional } type = Knob;
        int              sdataIndex  = -1;
        int              minVal      = 0;
        int              maxVal      = 127;
        std::vector<int> itemValues; // combo only: itemValues[i] = raw param value
        juce::Component* component  = nullptr; // raw ptr, owned by knobs/buttons/combos
    };

    EditorController&              controller;
    juce::Image                    bgImage;
    std::vector<WidgetEntry>       entries;
    juce::OwnedArray<SpritesheetKnob>   knobs;
    juce::OwnedArray<SpritesheetButton> buttons;
    juce::OwnedArray<juce::ComboBox>    combos;
    juce::OwnedArray<ConditionalImage>  conditionals;

    void buildWidgets(const juce::var&     pageNode,
                      const ParamRegistry& reg,
                      const SkinImages&    skins);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageComponent)
};

} // namespace mw2xt
