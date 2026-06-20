#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>
#include <vector>
#include <atomic>
#include "mw2xtLib/Protocol.h"
#include "mw2xtLib/SoundData.h"
#include "mw2xtLib/MultiData.h"
#include "mw2xtLib/GlobalData.h"

namespace mw2xt {

// Wraps JUCE MIDI I/O and exposes typed send/receive methods for all
// Waldorf Microwave II/XT SysEx and CC message types.
//
// Threading:
//   - sendSndp() and timerCallback() share wave-throttle state and must both
//     run on the JUCE message thread. All other send methods are thread-safe.
//   - autodetect() is a blocking call — must NOT be called on the message
//     thread or the audio thread. Use a background thread or juce::Thread.
//   - All inbound callbacks fire on the JUCE message thread.
class HardwareMidiDevice :
    public juce::MidiInputCallback,
    private juce::Timer
{
public:
    struct DeviceInfo {
        bool     valid              = false;
        uint8_t  deviceId           = 0;
        uint8_t  familyMemberLow    = 0;   // see sysex-protocol.md §Device Inquiry
        uint8_t  familyMemberHigh   = 0;
        char     firmwareVersion[4] = {};  // ASCII, e.g. "2.09"
    };

    HardwareMidiDevice();
    ~HardwareMidiDevice() override;

    // Open by device name (exact match against juce::MidiOutput/MidiInput::getAvailableDevices()).
    // Returns true if both output and input opened successfully.
    bool open(const juce::String& outputDeviceName, const juce::String& inputDeviceName);
    void close();
    bool isOpen() const;

    // Blocking Universal Device Inquiry. Sends F0 7E 7F 06 01 F7 and waits
    // up to timeoutMs for the XT's Identity Reply. Returns DeviceInfo with
    // valid=false on timeout. On success, updates getDeviceId().
    DeviceInfo autodetect(int timeoutMs = 500);

    void    setDeviceId(uint8_t id) noexcept { deviceId = id; }
    uint8_t getDeviceId()    const noexcept  { return deviceId; }

    // ── Outbound sends ────────────────────────────────────────────────────────

    // SNDP: Wave parameter (SDATA 14) is throttled to 100 ms (NFR-2); all
    // others send immediately. Call from the message thread only.
    void sendSndp(uint8_t buffer, int paramIndex, uint8_t value);

    void sendMulp(uint8_t buffer, uint8_t paramIndex, uint8_t value);
    void sendGlbp(uint8_t paramIndex, uint8_t value);
    void sendSndr(uint8_t bb, uint8_t nn);
    void sendMulr(uint8_t bb, uint8_t nn);
    void sendWavr(uint8_t hh, uint8_t ll);
    void sendWctr(uint8_t hh, uint8_t ll);
    void sendGlbr();
    void sendDisr();
    void sendRmtp(uint8_t element, uint8_t value);
    void sendModr();

    // Dump TX (editor→hardware) — for loading patches into the XT.
    void sendSndd(uint8_t bb, uint8_t nn, const SoundData& sd);

    // ── Inbound callbacks ─────────────────────────────────────────────────────
    // All callbacks fire on the JUCE message thread.

    using SoundDumpCb  = std::function<void(const SoundData&, uint8_t bb, uint8_t nn)>;
    using MultiDumpCb  = std::function<void(const MultiData&, uint8_t bb, uint8_t nn)>;
    using GlobalDumpCb = std::function<void(const GlobalData&)>;
    using DisplayCb    = std::function<void(const DisplayState&)>;
    using CcCb         = std::function<void(int cc, int value, int channel)>;
    using ModeDumpCb   = std::function<void(uint8_t mode)>;  // 0=Sound 1=Multi

    void onSoundDump (SoundDumpCb  cb) { soundDumpCb  = std::move(cb); }
    void onMultiDump (MultiDumpCb  cb) { multiDumpCb  = std::move(cb); }
    void onGlobalDump(GlobalDumpCb cb) { globalDumpCb = std::move(cb); }
    void onDisplay   (DisplayCb    cb) { displayCb    = std::move(cb); }
    void onCc        (CcCb         cb) { ccCb         = std::move(cb); }
    void onModeDump  (ModeDumpCb   cb) { modeDumpCb   = std::move(cb); }

private:
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void timerCallback() override;

    void dispatchSysEx(const std::vector<uint8_t>& bytes);
    void sendRaw(const uint8_t* data, size_t len);

    template<size_t N>
    void sendRaw(const std::array<uint8_t, N>& arr) { sendRaw(arr.data(), N); }
    void sendRaw(const std::vector<uint8_t>& v)     { sendRaw(v.data(), v.size()); }

    std::unique_ptr<juce::MidiOutput> midiOut;
    std::unique_ptr<juce::MidiInput>  midiIn;
    uint8_t deviceId { 0x00 };

    // Wave parameter throttle (SDATA 14 / NFR-2).
    // State accessed only on the message thread (sendSndp + timerCallback).
    static constexpr int kWaveParamIndex = 14;
    static constexpr int kWaveThrottleMs = 100;
    bool        waveHasPending    { false };
    uint8_t     wavePendingBuffer { 0 };
    uint8_t     wavePendingValue  { 0 };
    juce::int64 waveLastSentMs    { 0 };

    // Autodetect state. udiLock guards udiResult; awaitingUdi is an atomic flag.
    juce::CriticalSection udiLock;
    juce::WaitableEvent   udiEvent { false };  // false = manual reset
    DeviceInfo            udiResult;
    std::atomic<bool>     awaitingUdi { false };

    SoundDumpCb  soundDumpCb;
    MultiDumpCb  multiDumpCb;
    GlobalDumpCb globalDumpCb;
    DisplayCb    displayCb;
    CcCb         ccCb;
    ModeDumpCb   modeDumpCb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardwareMidiDevice)
};

} // namespace mw2xt
