#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../mw2xtEditor/EditorController.h"

namespace mw2xt
{

class EditorProcessor : public juce::AudioProcessor
{
public:
    EditorProcessor();
    ~EditorProcessor() override = default;

    const juce::String getName() const override { return JucePlugin_Name; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    EditorController& getController() noexcept { return controller; }

private:
    EditorController controller;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorProcessor)
};

} // namespace mw2xt
