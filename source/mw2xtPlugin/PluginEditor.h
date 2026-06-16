#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
};

} // namespace mw2xt
