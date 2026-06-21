#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include "../mw2xtUI/ParamRegistry.h"
#include "../mw2xtEditor/PageComponent.h"
#include "../mw2xtEditor/PageContainer.h"
#include "../mw2xtEditor/PageButtonBar.h"

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
    // ── Connection bar ────────────────────────────────────────────────────────
    // Single "XT Port" selector — same name used for both MIDI output and input.
    // This eliminates the common mistake of setting output and input to different ports.
    juce::ComboBox   midiPortSelector;
    juce::TextButton connectBtn      { "Connect" };
    juce::TextButton requestPatchBtn { "Req Patch" };
    juce::TextButton undoBtn         { "Undo" };
    juce::TextButton redoBtn         { "Redo" };
    juce::TextButton swapABBtn       { "A \xe2\x86\x94 B" };  // A ↔ B
    juce::TextButton pushPatchBtn    { "Push Patch" };
    juce::Label      statusLabel;
    juce::Label      modeLabel;

    void populateMidiDeviceList();
    void updateConnectButton();
    void onConnectClicked();
    void onModeReceived(uint8_t mode);

    // ── Skin / page ───────────────────────────────────────────────────────────
    EditorProcessor& proc;
    juce::Image      rootBG;
    ParamRegistry    paramRegistry;
    PageComponent::SkinImages skinImages;
    PageContainer                  pageContainer;
    std::unique_ptr<PageButtonBar> pageButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
};

} // namespace mw2xt
