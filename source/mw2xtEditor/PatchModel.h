#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <functional>
#include "mw2xtLib/SoundData.h"
#include "mw2xtLib/MultiData.h"
#include "mw2xtLib/GlobalData.h"

namespace mw2xt {

// Holds the editor's working copy of sound, multi, and global state.
// All write methods are safe to call from the JUCE message thread only.
class PatchModel : public juce::ChangeBroadcaster {
public:
    PatchModel()  = default;
    ~PatchModel() override = default;

    // ── Read ──────────────────────────────────────────────────────────────────
    const SoundData&   getSound()   const noexcept { return editBuffer; }
    const SoundData&   getCompare() const noexcept { return compareBuffer; }
    const MultiData&   getMulti()   const noexcept { return multiBuffer; }
    const GlobalData&  getGlobal()  const noexcept { return globalBuffer; }
    bool               isDirty()    const noexcept { return dirty; }
    juce::UndoManager& getUndoManager() noexcept   { return undoManager; }

    // Set by EditorController: called synchronously whenever an SDATA byte
    // changes (including during undo/redo), so hardware stays in sync.
    std::function<void(int index, uint8_t value)> onSoundParamChanged;

    // ── Undo-able SDATA edit ──────────────────────────────────────────────────
    void setSoundParam(int index, uint8_t value);

    // ── Bulk loads from hardware ──────────────────────────────────────────────
    // Clears undo history and dirty flag; saves editBuffer as compareBuffer.
    void loadFromDump(const SoundData& sd);
    void loadMultiFromDump(const MultiData& md);
    void loadGlobalFromDump(const GlobalData& gd);

    // ── A/B compare ───────────────────────────────────────────────────────────
    // Swaps editBuffer ↔ compareBuffer, marks dirty.
    // Caller (EditorController) must follow up with sendSndd() to hardware.
    void swapAB();

private:
    friend class SoundParamAction;
    void applySoundParam(int index, uint8_t value);

    SoundData  editBuffer;
    SoundData  compareBuffer;
    MultiData  multiBuffer;
    GlobalData globalBuffer;
    bool       dirty { false };

    juce::UndoManager undoManager { 30 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchModel)
};

} // namespace mw2xt
