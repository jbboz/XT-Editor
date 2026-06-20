#include "EditorController.h"
#include "mw2xtLib/Protocol.h"

namespace mw2xt {

EditorController::EditorController() {
    wireMidiCallbacks();
}

void EditorController::wireMidiCallbacks() {
    model.onSoundParamChanged = [this](int index, uint8_t value) {
        if (device.isOpen())
            device.sendSndp(kLocSoundEditBufSingle, index, value);
    };

    device.onSoundDump([this](const SoundData& sd, uint8_t, uint8_t) {
        model.loadFromDump(sd);
    });

    device.onMultiDump([this](const MultiData& md, uint8_t, uint8_t) {
        model.loadMultiFromDump(md);
    });

    device.onGlobalDump([this](const GlobalData& gd) {
        model.loadGlobalFromDump(gd);
    });

    device.onCc([this](int cc, int value, int channel) {
        routeCC(cc, value, channel);
    });
}

bool EditorController::connect(const juce::String& outputName,
                                const juce::String& inputName) {
    return device.open(outputName, inputName);
}

void EditorController::disconnect() {
    device.close();
}

void EditorController::setSoundParam(int index, uint8_t value) {
    // model.setSoundParam fires onSoundParamChanged → device.sendSndp
    model.setSoundParam(index, value);
}

void EditorController::swapAB() {
    model.swapAB();
    if (device.isOpen())
        device.sendSndd(kLocSoundEditBufSingle, 0x00, model.getSound());
}

void EditorController::requestCurrentPatch() {
    if (device.isOpen())
        device.sendSndr(kLocSoundEditBufSingle, 0x00);
}

void EditorController::routeCC(int cc, int value, int /*channel*/) {
    const int sdataIndex = ccToSdataIndex(cc);
    if (sdataIndex >= 0)
        setSoundParam(sdataIndex, static_cast<uint8_t>(value));
}

} // namespace mw2xt
