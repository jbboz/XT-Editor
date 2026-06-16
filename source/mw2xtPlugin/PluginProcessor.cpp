#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace mw2xt
{

EditorProcessor::EditorProcessor()
    : juce::AudioProcessor(BusesProperties{})
{
}

void EditorProcessor::prepareToPlay(double /*sampleRate*/, int /*maxBlockSize*/) {}
void EditorProcessor::releaseResources() {}

void EditorProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& /*midi*/)
{
    audio.clear();
}

juce::AudioProcessorEditor* EditorProcessor::createEditor()
{
    return new EditorComponent(*this);
}

void EditorProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}
void EditorProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/) {}

} // namespace mw2xt

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new mw2xt::EditorProcessor();
}
