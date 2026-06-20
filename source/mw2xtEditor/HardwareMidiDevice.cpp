#include "HardwareMidiDevice.h"

namespace mw2xt {

HardwareMidiDevice::HardwareMidiDevice() = default;

HardwareMidiDevice::~HardwareMidiDevice()
{
    close();
}

bool HardwareMidiDevice::open(const juce::String& outputDeviceName,
                               const juce::String& inputDeviceName)
{
    close();

    for (auto& info : juce::MidiOutput::getAvailableDevices()) {
        if (info.name == outputDeviceName) {
            midiOut = juce::MidiOutput::openDevice(info.identifier);
            break;
        }
    }

    for (auto& info : juce::MidiInput::getAvailableDevices()) {
        if (info.name == inputDeviceName) {
            midiIn = juce::MidiInput::openDevice(info.identifier, this);
            if (midiIn)
                midiIn->start();
            break;
        }
    }

    return isOpen();
}

void HardwareMidiDevice::close()
{
    stopTimer();
    if (midiIn) {
        midiIn->stop();
        midiIn.reset();
    }
    midiOut.reset();
}

bool HardwareMidiDevice::isOpen() const
{
    return midiOut != nullptr && midiIn != nullptr;
}

// ── Autodetect ────────────────────────────────────────────────────────────────

HardwareMidiDevice::DeviceInfo HardwareMidiDevice::autodetect(int timeoutMs)
{
    {
        juce::ScopedLock lock(udiLock);
        udiResult = {};
        udiEvent.reset();
    }
    awaitingUdi.store(true, std::memory_order_release);

    // Universal Device Inquiry broadcast: F0 7E 7F 06 01 F7
    static constexpr uint8_t udi[] = { 0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7 };
    sendRaw(udi, sizeof(udi));

    const bool responded = udiEvent.wait(timeoutMs);
    awaitingUdi.store(false, std::memory_order_release);

    if (!responded)
        return {};  // valid=false

    juce::ScopedLock lock(udiLock);
    if (udiResult.valid)
        deviceId = udiResult.deviceId;
    return udiResult;
}

// ── Outbound sends ────────────────────────────────────────────────────────────

void HardwareMidiDevice::sendSndp(uint8_t buffer, int paramIndex, uint8_t value)
{
    if (paramIndex == kWaveParamIndex) {
        // 100 ms throttle-with-trailing-send (NFR-2).
        // The first change after a quiet period goes out immediately.
        // Subsequent changes within the window update the pending value;
        // the timer fires kWaveThrottleMs after the last send to flush it.
        wavePendingBuffer = buffer;
        wavePendingValue  = value;
        waveHasPending    = true;

        const juce::int64 nowMs = juce::Time::currentTimeMillis();
        if (nowMs - waveLastSentMs >= kWaveThrottleMs) {
            waveLastSentMs = nowMs;
            waveHasPending = false;
            sendRaw(encodeSndp(deviceId, buffer, paramIndex, value));
        }
        if (!isTimerRunning())
            startTimer(kWaveThrottleMs);
        return;
    }
    sendRaw(encodeSndp(deviceId, buffer, paramIndex, value));
}

void HardwareMidiDevice::timerCallback()
{
    if (waveHasPending) {
        waveLastSentMs = juce::Time::currentTimeMillis();
        waveHasPending = false;
        sendRaw(encodeSndp(deviceId, wavePendingBuffer, kWaveParamIndex, wavePendingValue));
    } else {
        stopTimer();
    }
}

void HardwareMidiDevice::sendMulp(uint8_t buffer, uint8_t paramIndex, uint8_t value)
{
    sendRaw(encodeMulp(deviceId, buffer, paramIndex, value));
}

void HardwareMidiDevice::sendGlbp(uint8_t paramIndex, uint8_t value)
{
    sendRaw(encodeGlbp(deviceId, paramIndex, value));
}

void HardwareMidiDevice::sendSndr(uint8_t bb, uint8_t nn)
{
    sendRaw(encodeSndr(deviceId, bb, nn));
}

void HardwareMidiDevice::sendMulr(uint8_t bb, uint8_t nn)
{
    sendRaw(encodeMulr(deviceId, bb, nn));
}

void HardwareMidiDevice::sendWavr(uint8_t hh, uint8_t ll)
{
    sendRaw(encodeWavr(deviceId, hh, ll));
}

void HardwareMidiDevice::sendWctr(uint8_t hh, uint8_t ll)
{
    sendRaw(encodeWctr(deviceId, hh, ll));
}

void HardwareMidiDevice::sendGlbr()
{
    sendRaw(encodeGlbr(deviceId));
}

void HardwareMidiDevice::sendDisr()
{
    sendRaw(encodeDisr(deviceId));
}

void HardwareMidiDevice::sendRmtp(uint8_t element, uint8_t value)
{
    sendRaw(encodeRmtp(deviceId, element, value));
}

void HardwareMidiDevice::sendModr()
{
    // MODR: F0 3E 0E DEV 07 00 F7 (7 bytes; XSUM=00 per empty-data convention)
    const uint8_t msg[] = { kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId,
                             kIdmModr, 0x00, kSysExEnd };
    sendRaw(msg, sizeof(msg));
}

void HardwareMidiDevice::sendSndd(uint8_t bb, uint8_t nn, const SoundData& sd)
{
    sendRaw(encodeSndd(deviceId, bb, nn, sd));
}

// ── Raw send ─────────────────────────────────────────────────────────────────

void HardwareMidiDevice::sendRaw(const uint8_t* data, size_t len)
{
    if (!midiOut)
        return;  // silently fail when disconnected (NFR-M1.3-2)
    midiOut->sendMessageNow(juce::MidiMessage(data, static_cast<int>(len)));
}

// ── Inbound ──────────────────────────────────────────────────────────────────

void HardwareMidiDevice::handleIncomingMidiMessage(juce::MidiInput*,
                                                    const juce::MidiMessage& msg)
{
    // Runs on the JUCE MIDI thread. Marshal everything to the message thread.
    if (msg.isSysEx()) {
        // getRawData() returns the complete SysEx including 0xF0 and 0xF7.
        const uint8_t* raw  = msg.getRawData();
        const int      size = msg.getRawDataSize();
        std::vector<uint8_t> bytes(raw, raw + size);

        juce::MessageManager::callAsync([this, b = std::move(bytes)]() mutable {
            dispatchSysEx(b);
        });
    } else if (msg.isController()) {
        const int cc  = msg.getControllerNumber();
        const int val = msg.getControllerValue();
        const int ch  = msg.getChannel();
        juce::MessageManager::callAsync([this, cc, val, ch]() {
            if (ccCb)
                ccCb(cc, val, ch);
        });
    }
}

// ── SysEx dispatch ────────────────────────────────────────────────────────────

void HardwareMidiDevice::dispatchSysEx(const std::vector<uint8_t>& bytes)
{
    if (bytes.size() < 2 || bytes.front() != 0xF0 || bytes.back() != 0xF7)
        return;

    // Universal Device Inquiry Identity Reply:
    // F0 7E <devId> 06 02 3E 0E 00 <memberLow> <memberHigh> V1 V2 V3 V4 F7
    // = 15 bytes
    if (bytes.size() == 15 &&
        bytes[1] == 0x7E   &&
        bytes[3] == 0x06   &&
        bytes[4] == 0x02   &&
        bytes[5] == 0x3E   &&  // Waldorf manufacturer
        bytes[6] == 0x0E   &&  // Microwave II equipment ID
        bytes[7] == 0x00)
    {
        if (awaitingUdi.load(std::memory_order_acquire)) {
            juce::ScopedLock lock(udiLock);
            udiResult.valid             = true;
            udiResult.deviceId          = bytes[2];
            udiResult.familyMemberLow   = bytes[8];
            udiResult.familyMemberHigh  = bytes[9];
            udiResult.firmwareVersion[0] = static_cast<char>(bytes[10]);
            udiResult.firmwareVersion[1] = static_cast<char>(bytes[11]);
            udiResult.firmwareVersion[2] = static_cast<char>(bytes[12]);
            udiResult.firmwareVersion[3] = static_cast<char>(bytes[13]);
            udiEvent.signal();
        }
        return;
    }

    // All other Waldorf SysEx: F0 3E 0E DEV IDM [data...] [XSUM] F7
    if (bytes.size() < 5 ||
        bytes[1] != kMfrWaldorf ||
        bytes[2] != kDevTypeXT)
        return;

    const uint8_t idm = bytes[4];

    switch (idm) {
        case kIdmSndd:
            if (soundDumpCb) {
                if (auto sd = decodeSndd(bytes))
                    soundDumpCb(*sd, bytes[5], bytes[6]);
            }
            break;

        case kIdmMuld:
            if (multiDumpCb) {
                if (auto md = decodeMuld(bytes))
                    multiDumpCb(*md, bytes[5], bytes[6]);
            }
            break;

        case kIdmGlbd:
            if (globalDumpCb) {
                if (auto gd = decodeGlbd(bytes))
                    globalDumpCb(*gd);
            }
            break;

        case kIdmDisd:
            if (displayCb) {
                if (auto ds = decodeDisd(bytes))
                    displayCb(*ds);
            }
            break;

        case kIdmModd:
            // MODD: F0 3E 0E DEV 17 MM F7 (7 bytes)
            if (modeDumpCb && bytes.size() == 7)
                modeDumpCb(bytes[5]);
            break;

        default:
            break;
    }
}

} // namespace mw2xt
