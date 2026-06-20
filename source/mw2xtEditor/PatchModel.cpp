#include "PatchModel.h"

namespace mw2xt {

class SoundParamAction : public juce::UndoableAction {
public:
    SoundParamAction(PatchModel& m, int idx, uint8_t oldV, uint8_t newV)
        : model(m), index(idx), oldValue(oldV), newValue(newV) {}

    bool perform() override { model.applySoundParam(index, newValue); return true; }
    bool undo()    override { model.applySoundParam(index, oldValue); return true; }
    int  getSizeInUnits() override { return static_cast<int>(sizeof(*this)); }

private:
    PatchModel& model;
    int     index;
    uint8_t oldValue, newValue;
};

void PatchModel::setSoundParam(int index, uint8_t value) {
    if (editBuffer[index] == value)
        return;
    undoManager.perform(new SoundParamAction(*this, index, editBuffer[index], value));
}

void PatchModel::applySoundParam(int index, uint8_t value) {
    editBuffer[index] = value;
    dirty = true;
    if (onSoundParamChanged)
        onSoundParamChanged(index, value);
    sendChangeMessage();
}

void PatchModel::loadFromDump(const SoundData& sd) {
    compareBuffer = sd;
    editBuffer    = sd;
    undoManager.clearUndoHistory();
    dirty = false;
    sendChangeMessage();
}

void PatchModel::loadMultiFromDump(const MultiData& md) {
    multiBuffer = md;
    sendChangeMessage();
}

void PatchModel::loadGlobalFromDump(const GlobalData& gd) {
    globalBuffer = gd;
    sendChangeMessage();
}

void PatchModel::swapAB() {
    std::swap(editBuffer, compareBuffer);
    dirty = true;
    sendChangeMessage();
}

} // namespace mw2xt
