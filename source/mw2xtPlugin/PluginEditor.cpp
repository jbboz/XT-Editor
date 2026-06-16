#include "PluginEditor.h"

namespace mw2xt
{

EditorComponent::EditorComponent(EditorProcessor& processor)
    : juce::AudioProcessorEditor(&processor)
{
    setResizable(true, true);
    setResizeLimits(640, 400, 3840, 2400);
    setSize(960, 600);
}

void EditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(0x12, 0x12, 0x12));

    g.setColour(juce::Colour::fromRGB(0xff, 0x88, 0x00));
    g.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    g.drawText("MW2 / XT Editor", getLocalBounds().removeFromTop(80),
               juce::Justification::centred);

    g.setColour(juce::Colour::fromRGB(0x88, 0x88, 0x88));
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("Phase 1 scaffold (M1.1) — protocol layer arrives at M1.2",
               getLocalBounds().reduced(20),
               juce::Justification::centred);
}

void EditorComponent::resized() {}

} // namespace mw2xt
