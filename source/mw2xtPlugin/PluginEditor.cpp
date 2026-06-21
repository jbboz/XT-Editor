#include "PluginEditor.h"
#include <BinaryData.h>
#include "mw2xtLib/Protocol.h"

namespace mw2xt
{

// ── Helpers ──────────────────────────────────────────────────────────────────

static juce::Image loadPng(const char* data, int size)
{
    return juce::ImageFileFormat::loadFrom(data, static_cast<size_t>(size));
}

// ── Construction ─────────────────────────────────────────────────────────────

EditorComponent::EditorComponent(EditorProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
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

    // ── Load param registry ───────────────────────────────────────────────────
    paramRegistry.loadFromBinaryData(BinaryData::parameterDescriptions_xt_json,
                                     BinaryData::parameterDescriptions_xt_jsonSize);

    // ── Parse skin layout ─────────────────────────────────────────────────────
    juce::var skinJson;
    {
        const juce::String skinText(BinaryData::xtDefault_json,
                                    static_cast<size_t>(BinaryData::xtDefault_jsonSize));
        skinJson = juce::JSON::parse(skinText);
    }

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

    // ── Connection bar ────────────────────────────────────────────────────────
    populateMidiDeviceList();

    connectBtn.onClick     = [this] { onConnectClicked(); };
    requestPatchBtn.onClick = [this] { proc.getController().requestCurrentPatch(); };
    undoBtn.onClick        = [this] { proc.getController().getModel().getUndoManager().undo(); };
    redoBtn.onClick        = [this] { proc.getController().getModel().getUndoManager().redo(); };
    swapABBtn.onClick      = [this] { proc.getController().swapAB(); };
    pushPatchBtn.onClick   = [this] {
        auto& ctrl = proc.getController();
        if (ctrl.isConnected())
            ctrl.getDevice().sendSndd(kLocSoundEditBufSingle, 0x00,
                                      ctrl.getModel().getSound());
    };

    proc.getController().onModeChanged = [this](uint8_t m) { onModeReceived(m); };

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    modeLabel.setJustificationType(juce::Justification::centredLeft);
    modeLabel.setText("", juce::dontSendNotification);
    updateConnectButton();

    addAndMakeVisible(midiPortSelector);
    addAndMakeVisible(connectBtn);
    addAndMakeVisible(requestPatchBtn);
    addAndMakeVisible(undoBtn);
    addAndMakeVisible(redoBtn);
    addAndMakeVisible(swapABBtn);
    addAndMakeVisible(pushPatchBtn);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(modeLabel);

    // Root skin: 3400×2000 at scale 0.5 → 1700×1000.
    // Non-resizable: widget positions are baked at construction time.
    setResizable(false, false);
    setSize(1700, 1000);
}

// ── MIDI device helpers ───────────────────────────────────────────────────────

void EditorComponent::populateMidiDeviceList()
{
    // Build a list of devices that appear in both output AND input lists,
    // since the XT needs bidirectional communication on the same port.
    midiPortSelector.clear(juce::dontSendNotification);

    const auto outputs = juce::MidiOutput::getAvailableDevices();
    const auto inputs  = juce::MidiInput::getAvailableDevices();

    // Collect output names for quick lookup.
    juce::StringArray outNames;
    for (const auto& d : outputs)
        outNames.add(d.name);

    int id = 1;
    for (const auto& d : inputs) {
        if (outNames.contains(d.name))
            midiPortSelector.addItem(d.name, id++);
    }

    // Also add output-only devices (some interfaces may differ in listing).
    for (const auto& d : outputs) {
        bool found = false;
        for (int i = 1; i <= midiPortSelector.getNumItems(); ++i)
            if (midiPortSelector.getItemText(i - 1) == d.name) { found = true; break; }
        if (!found)
            midiPortSelector.addItem(d.name, id++);
    }
}

void EditorComponent::updateConnectButton()
{
    const bool connected = proc.getController().isConnected();
    connectBtn.setButtonText(connected ? "Disconnect" : "Connect");
    const juce::String portName = midiPortSelector.getText();
    statusLabel.setText(connected ? ("Connected: " + portName) : "Not connected",
                        juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          connected ? juce::Colours::lightgreen
                                    : juce::Colours::orangered);
}

void EditorComponent::onConnectClicked()
{
    auto& ctrl = proc.getController();
    if (ctrl.isConnected()) {
        ctrl.disconnect();
        modeLabel.setText("", juce::dontSendNotification);
    } else {
        const juce::String portName = midiPortSelector.getText();
        if (portName.isNotEmpty()) {
            // Use the same port name for both output and input — standard for
            // hardware synths where the physical port is bidirectional.
            const bool ok = ctrl.connect(portName, portName);
            if (!ok)
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "MIDI Error",
                    "Could not open MIDI port:\n" + portName
                    + "\n\nCheck that the device is connected and no other app has it open.");
            else
                ctrl.queryMode();
        }
    }
    updateConnectButton();
}

void EditorComponent::onModeReceived(uint8_t mode)
{
    const bool isMulti = (mode != 0);
    modeLabel.setText(isMulti ? "MULTI" : "SINGLE", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId,
                        isMulti ? juce::Colours::orange : juce::Colours::lightgreen);
    if (isMulti)
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Multi Mode Active",
            "The XT is in Multi mode.\n"
            "SNDP (parameter edits) only affect the Single edit buffer.\n"
            "Switch the XT to Single mode to hear UI changes.");
}

// ── Layout ────────────────────────────────────────────────────────────────────

void EditorComponent::resized()
{
    // Connection bar: 24px controls with 4px top margin.
    // Single "XT Port" selector (same device used for output and input).
    int x = 8;
    midiPortSelector.setBounds(x, 4, 240, 24);  x += 244;
    connectBtn.setBounds(x, 4, 88, 24);         x += 92;
    requestPatchBtn.setBounds(x, 4, 80, 24);    x += 84;
    x += 12;
    undoBtn.setBounds(x, 4, 48, 24);            x += 52;
    redoBtn.setBounds(x, 4, 48, 24);            x += 52;
    swapABBtn.setBounds(x, 4, 58, 24);          x += 62;
    pushPatchBtn.setBounds(x, 4, 78, 24);       x += 86;
    x += 8;
    statusLabel.setBounds(x, 4, 200, 24);       x += 204;
    modeLabel.setBounds(x, 4, 60, 24);

    // pageOsc: skin image x=36, y=489, w=3322.7, h=1479.8 at scale=0.5
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

    // Dark strip behind the connection bar controls.
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.75f));
    g.fillRect(0, 0, getWidth(), 32);
}

} // namespace mw2xt
