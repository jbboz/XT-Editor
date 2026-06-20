#pragma once

#include "PatchModel.h"
#include "HardwareMidiDevice.h"

namespace mw2xt {

// Mediates between PatchModel (in-memory state) and HardwareMidiDevice (wire).
// All public methods must be called on the JUCE message thread.
class EditorController {
public:
    EditorController();
    ~EditorController() = default;

    PatchModel&        getModel()  noexcept { return model; }
    HardwareMidiDevice& getDevice() noexcept { return device; }

    // Open named MIDI ports. Returns true if both output and input opened.
    bool connect(const juce::String& outputName, const juce::String& inputName);
    void disconnect();
    bool isConnected() const noexcept { return device.isOpen(); }

    // Single SDATA edit: pushes to undo stack and sends SNDP to hardware.
    void setSoundParam(int index, uint8_t value);

    // A/B compare swap: swaps model buffers then sends full SNDD to hardware.
    void swapAB();

    // Request the XT edit buffer (SNDR BB=0x20 NN=0x00).
    void requestCurrentPatch();

private:
    PatchModel         model;
    HardwareMidiDevice device;

    void wireMidiCallbacks();
    void routeCC(int cc, int value, int channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorController)
};

} // namespace mw2xt
