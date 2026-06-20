#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include "../mw2xtUI/ParamRegistry.h"
#include "../mw2xtEditor/PageComponent.h"

namespace mw2xt
{

class EditorComponent : public juce::AudioProcessorEditor
{
public:
    explicit EditorComponent(EditorProcessor&);
    ~EditorComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    EditorProcessor& proc;

    juce::Image      rootBG;
    ParamRegistry    paramRegistry;
    PageComponent::SkinImages skinImages;

    std::unique_ptr<PageComponent> pageOsc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
};

} // namespace mw2xt
