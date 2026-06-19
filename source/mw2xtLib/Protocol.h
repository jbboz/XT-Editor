#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include "SoundData.h"
#include "MultiData.h"
#include "GlobalData.h"

namespace mw2xt {

// ─── Waldorf SysEx constants ─────────────────────────────────────────────

constexpr uint8_t kSysExBegin   = 0xF0;
constexpr uint8_t kSysExEnd     = 0xF7;
constexpr uint8_t kMfrWaldorf   = 0x3E;
constexpr uint8_t kDevTypeXT    = 0x0E;  // MW2 / XT machine type byte
constexpr uint8_t kDevBroadcast = 0x7F;

// IDM (command byte, position 4 in every frame)
constexpr uint8_t kIdmSndr = 0x00;  // Sound Request
constexpr uint8_t kIdmSndd = 0x10;  // Sound Dump
constexpr uint8_t kIdmSndp = 0x20;  // Sound Parameter Change  (no checksum)
constexpr uint8_t kIdmMulr = 0x01;  // Multi Request
constexpr uint8_t kIdmMuld = 0x11;  // Multi Dump
constexpr uint8_t kIdmMulp = 0x21;  // Multi Parameter Change  (no checksum)
constexpr uint8_t kIdmWavr = 0x02;  // Wave Request
constexpr uint8_t kIdmWavd = 0x12;  // Wave Dump
constexpr uint8_t kIdmWctr = 0x03;  // Wave Control Table Request
constexpr uint8_t kIdmWctd = 0x13;  // Wave Control Table Dump
constexpr uint8_t kIdmGlbr = 0x04;  // Global Request
constexpr uint8_t kIdmGlbd = 0x14;  // Global Dump
constexpr uint8_t kIdmGlbp = 0x24;  // Global Parameter Change (no checksum)
constexpr uint8_t kIdmDisr = 0x05;  // Display Request
constexpr uint8_t kIdmDisd = 0x15;  // Display Dump
constexpr uint8_t kIdmRmtp = 0x26;  // Remote Control Parameter Change
constexpr uint8_t kIdmModr = 0x07;  // Mode Request
constexpr uint8_t kIdmModd = 0x17;  // Mode Dump

// Sound dump locations (BB byte in SNDR / SNDD)
constexpr uint8_t kLocSoundBankA         = 0x00;
constexpr uint8_t kLocSoundBankB         = 0x01;
constexpr uint8_t kLocAllSounds          = 0x10;
constexpr uint8_t kLocSoundEditBufSingle = 0x20;

// Fixed frame sizes
constexpr size_t kSndpFrameSize = 10;
constexpr size_t kSnddFrameSize = 265;
constexpr size_t kSndrFrameSize = 9;
constexpr size_t kMulpFrameSize = 9;
constexpr size_t kMuldFrameSize = 265;
constexpr size_t kGlbpFrameSize = 8;
constexpr size_t kGlbdFrameSize = 39;
constexpr size_t kGlbrFrameSize = 7;
constexpr size_t kWavrFrameSize = 9;
constexpr size_t kWavdFrameSize = 137;
constexpr size_t kWctrFrameSize = 9;
constexpr size_t kWctdFrameSize = 265;
constexpr size_t kDisrFrameSize = 7;
constexpr size_t kDisdFrameSize = 88;
constexpr size_t kRmtpFrameSize = 9;

// ─── Checksum ────────────────────────────────────────────────────────────
// XSUM = Σ(data bytes) & 0x7F  (8-bit accumulation, mask to 7 bits).
// 0x7F is accepted as universally valid on decode (bypass sentinel).
uint8_t computeChecksum(const uint8_t* data, size_t len);
bool    checksumValid(uint8_t computed, uint8_t received);

// ─── SNDP — Sound Parameter Change (10 bytes, no checksum) ───────────────
// F0 3E 0E DEV 20 LL HH PP XX F7
// HH=0x00 for paramIndex 0-127, HH=0x01 for 128-255; PP = paramIndex & 0x7F
inline std::array<uint8_t, kSndpFrameSize>
encodeSndp(uint8_t deviceId, uint8_t buffer, int paramIndex, uint8_t value) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmSndp,
        buffer,
        static_cast<uint8_t>(paramIndex >= 128 ? 0x01 : 0x00),
        static_cast<uint8_t>(paramIndex & 0x7F),
        value,
        kSysExEnd
    };
}

// ─── MULP — Multi Parameter Change (9 bytes, no checksum) ────────────────
// F0 3E 0E DEV 21 LL PP XX F7
// IDM = 0x21 (Waldorf §2.23 title says "20h" — typo; wire byte is 0x21)
inline std::array<uint8_t, kMulpFrameSize>
encodeMulp(uint8_t deviceId, uint8_t buffer, uint8_t paramIndex, uint8_t value) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmMulp,
        buffer, paramIndex, value, kSysExEnd
    };
}

// ─── GLBP — Global Parameter Change (8 bytes, no checksum) ───────────────
// F0 3E 0E DEV 24 PP XX F7
inline std::array<uint8_t, kGlbpFrameSize>
encodeGlbp(uint8_t deviceId, uint8_t paramIndex, uint8_t value) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmGlbp,
        paramIndex, value, kSysExEnd
    };
}

// ─── Simple request frames ────────────────────────────────────────────────
// SNDR: F0 3E 0E DEV 00 BB NN ((BB+NN)&7F) F7
inline std::array<uint8_t, kSndrFrameSize>
encodeSndr(uint8_t deviceId, uint8_t locationBB, uint8_t locationNN) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmSndr,
        locationBB, locationNN,
        static_cast<uint8_t>((locationBB + locationNN) & 0x7F),
        kSysExEnd
    };
}

// MULR: F0 3E 0E DEV 01 BB NN ((BB+NN)&7F) F7
inline std::array<uint8_t, 9>
encodeMulr(uint8_t deviceId, uint8_t locationBB, uint8_t locationNN) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmMulr,
        locationBB, locationNN,
        static_cast<uint8_t>((locationBB + locationNN) & 0x7F),
        kSysExEnd
    };
}

// WAVR: F0 3E 0E DEV 02 HH LL ((HH+LL)&7F) F7
inline std::array<uint8_t, kWavrFrameSize>
encodeWavr(uint8_t deviceId, uint8_t waveHH, uint8_t waveLL) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmWavr,
        waveHH, waveLL,
        static_cast<uint8_t>((waveHH + waveLL) & 0x7F),
        kSysExEnd
    };
}

// WCTR: F0 3E 0E DEV 03 HH LL ((HH+LL)&7F) F7
inline std::array<uint8_t, kWctrFrameSize>
encodeWctr(uint8_t deviceId, uint8_t tableHH, uint8_t tableLL) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmWctr,
        tableHH, tableLL,
        static_cast<uint8_t>((tableHH + tableLL) & 0x7F),
        kSysExEnd
    };
}

// RMTP: F0 3E 0E DEV 26 UU MM ((UU+MM)&7F) F7
inline std::array<uint8_t, kRmtpFrameSize>
encodeRmtp(uint8_t deviceId, uint8_t element, uint8_t value) {
    return {
        kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmRmtp,
        element, value,
        static_cast<uint8_t>((element + value) & 0x7F),
        kSysExEnd
    };
}

// GLBR / DISR: F0 3E 0E DEV IDM 00 F7 (7 bytes)
inline std::array<uint8_t, kGlbrFrameSize>
encodeGlbr(uint8_t deviceId) {
    return { kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmGlbr, 0x00, kSysExEnd };
}
inline std::array<uint8_t, kDisrFrameSize>
encodeDisr(uint8_t deviceId) {
    return { kSysExBegin, kMfrWaldorf, kDevTypeXT, deviceId, kIdmDisr, 0x00, kSysExEnd };
}

// ─── Dump encode / decode ─────────────────────────────────────────────────

// SNDD: F0 3E 0E DEV 10 BB NN data[256] XSUM F7 = 265 bytes
std::vector<uint8_t> encodeSndd(uint8_t deviceId, uint8_t locationBB,
                                  uint8_t locationNN, const SoundData& sd);
std::optional<SoundData> decodeSndd(const std::vector<uint8_t>& frame);

// MULD: same layout as SNDD but IDM=0x11 and payload is MultiData (256 bytes).
std::vector<uint8_t> encodeMuld(uint8_t deviceId, uint8_t locationBB,
                                  uint8_t locationNN, const MultiData& md);
std::optional<MultiData> decodeMuld(const std::vector<uint8_t>& frame);

// GLBD: F0 3E 0E DEV 14 data[32] XSUM F7 = 39 bytes (no BB/NN).
std::vector<uint8_t> encodeGlbd(uint8_t deviceId, const GlobalData& gd);
std::optional<GlobalData> decodeGlbd(const std::vector<uint8_t>& frame);

// WAVD: F0 3E 0E DEV 12 HH LL nibbles[128] XSUM F7 = 137 bytes.
std::vector<uint8_t> encodeWavd(uint8_t deviceId, uint8_t waveHH, uint8_t waveLL,
                                  const std::vector<uint8_t>& nibbles128);
std::vector<uint8_t> decodeWavd(const std::vector<uint8_t>& frame);

// WCTD: F0 3E 0E DEV 13 HH LL nibbles[256] XSUM F7 = 265 bytes.
std::vector<uint8_t> encodeWctd(uint8_t deviceId, uint8_t tableHH, uint8_t tableLL,
                                  const std::vector<uint8_t>& nibbles256);
std::vector<uint8_t> decodeWctd(const std::vector<uint8_t>& frame);

// DISD: F0 3E 0E DEV 15 lcd[80] led XSUM F7 = 88 bytes.
struct DisplayState {
    std::array<char, 40> upperRow{};
    std::array<char, 40> lowerRow{};
    uint8_t ledBitmask{};
};
std::vector<uint8_t> encodeDisd(uint8_t deviceId, const DisplayState& ds);
std::optional<DisplayState> decodeDisd(const std::vector<uint8_t>& frame);

} // namespace mw2xt
