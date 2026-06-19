#include "Protocol.h"

namespace mw2xt {

// ─── Checksum ──────────────────────────────────────────────────────────

uint8_t computeChecksum(const uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += data[i];
    return sum & 0x7F;
}

bool checksumValid(uint8_t computed, uint8_t received) {
    return received == 0x7F || received == computed;
}

// ─── Helper: build a 265-byte dump frame (SNDD / MULD / WCTD layout) ───

static std::vector<uint8_t> buildDump265(uint8_t deviceId, uint8_t idm,
                                          uint8_t bb, uint8_t nn,
                                          const uint8_t* payload, size_t payloadLen) {
    std::vector<uint8_t> f;
    f.reserve(265);
    f.push_back(kSysExBegin);
    f.push_back(kMfrWaldorf);
    f.push_back(kDevTypeXT);
    f.push_back(deviceId);
    f.push_back(idm);
    f.push_back(bb);
    f.push_back(nn);
    for (size_t i = 0; i < payloadLen; ++i)
        f.push_back(payload[i]);
    f.push_back(computeChecksum(payload, payloadLen));
    f.push_back(kSysExEnd);
    return f;
}

static bool validateDump265(const std::vector<uint8_t>& f, uint8_t expectedIdm) {
    if (f.size() != kSnddFrameSize)  return false;
    if (f[0] != kSysExBegin)         return false;
    if (f[1] != kMfrWaldorf)         return false;
    if (f[2] != kDevTypeXT)          return false;
    if (f[4] != expectedIdm)         return false;
    if (f[264] != kSysExEnd)         return false;
    return checksumValid(computeChecksum(f.data() + 7, 256), f[263]);
}

// ─── SNDD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeSndd(uint8_t deviceId, uint8_t bb,
                                  uint8_t nn, const SoundData& sd) {
    return buildDump265(deviceId, kIdmSndd, bb, nn, sd.data.data(), 256);
}

std::optional<SoundData> decodeSndd(const std::vector<uint8_t>& f) {
    if (!validateDump265(f, kIdmSndd))
        return std::nullopt;
    SoundData sd;
    for (size_t i = 0; i < 256; ++i)
        sd.data[i] = f[7 + i];
    return sd;
}

// ─── MULD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeMuld(uint8_t deviceId, uint8_t bb,
                                  uint8_t nn, const MultiData& md) {
    std::array<uint8_t, 256> payload{};
    for (size_t i = 0; i < 32; ++i)
        payload[i] = md.mdata[i];
    for (size_t inst = 0; inst < 8; ++inst)
        for (size_t j = 0; j < 28; ++j)
            payload[32 + inst * 28 + j] = md.idata[inst][static_cast<int>(j)];
    return buildDump265(deviceId, kIdmMuld, bb, nn, payload.data(), 256);
}

std::optional<MultiData> decodeMuld(const std::vector<uint8_t>& f) {
    if (!validateDump265(f, kIdmMuld))
        return std::nullopt;
    MultiData md;
    for (size_t i = 0; i < 32; ++i)
        md.mdata[i] = f[7 + i];
    for (size_t inst = 0; inst < 8; ++inst)
        for (size_t j = 0; j < 28; ++j)
            md.idata[inst][static_cast<int>(j)] = f[7 + 32 + inst * 28 + j];
    return md;
}

// ─── GLBD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeGlbd(uint8_t deviceId, const GlobalData& gd) {
    std::vector<uint8_t> f;
    f.reserve(kGlbdFrameSize);
    f.push_back(kSysExBegin);
    f.push_back(kMfrWaldorf);
    f.push_back(kDevTypeXT);
    f.push_back(deviceId);
    f.push_back(kIdmGlbd);
    for (uint8_t b : gd.data)
        f.push_back(b);
    f.push_back(computeChecksum(gd.data.data(), 32));
    f.push_back(kSysExEnd);
    return f;
}

std::optional<GlobalData> decodeGlbd(const std::vector<uint8_t>& f) {
    if (f.size() != kGlbdFrameSize)  return std::nullopt;
    if (f[0] != kSysExBegin)          return std::nullopt;
    if (f[1] != kMfrWaldorf)          return std::nullopt;
    if (f[2] != kDevTypeXT)           return std::nullopt;
    if (f[4] != kIdmGlbd)             return std::nullopt;
    if (f[38] != kSysExEnd)           return std::nullopt;
    if (!checksumValid(computeChecksum(f.data() + 5, 32), f[37]))
        return std::nullopt;
    GlobalData gd;
    for (size_t i = 0; i < 32; ++i)
        gd.data[i] = f[5 + i];
    return gd;
}

// ─── WAVD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeWavd(uint8_t deviceId, uint8_t waveHH, uint8_t waveLL,
                                  const std::vector<uint8_t>& nibbles128) {
    if (nibbles128.size() != 128) return {};
    std::vector<uint8_t> f;
    f.reserve(kWavdFrameSize);
    f.push_back(kSysExBegin);
    f.push_back(kMfrWaldorf);
    f.push_back(kDevTypeXT);
    f.push_back(deviceId);
    f.push_back(kIdmWavd);
    f.push_back(waveHH);
    f.push_back(waveLL);
    for (uint8_t b : nibbles128)
        f.push_back(b);
    f.push_back(computeChecksum(nibbles128.data(), nibbles128.size()));
    f.push_back(kSysExEnd);
    return f;
}

std::vector<uint8_t> decodeWavd(const std::vector<uint8_t>& f) {
    if (f.size() != kWavdFrameSize) return {};
    if (f[0] != kSysExBegin)        return {};
    if (f[1] != kMfrWaldorf)        return {};
    if (f[2] != kDevTypeXT)         return {};
    if (f[4] != kIdmWavd)           return {};
    if (f[136] != kSysExEnd)        return {};
    if (!checksumValid(computeChecksum(f.data() + 7, 128), f[135]))
        return {};
    return std::vector<uint8_t>(f.begin() + 7, f.begin() + 135);
}

// ─── WCTD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeWctd(uint8_t deviceId, uint8_t tableHH, uint8_t tableLL,
                                  const std::vector<uint8_t>& nibbles256) {
    if (nibbles256.size() != 256) return {};
    return buildDump265(deviceId, kIdmWctd, tableHH, tableLL,
                        nibbles256.data(), nibbles256.size());
}

std::vector<uint8_t> decodeWctd(const std::vector<uint8_t>& f) {
    if (!validateDump265(f, kIdmWctd)) return {};
    return std::vector<uint8_t>(f.begin() + 7, f.begin() + 263);
}

// ─── DISD ──────────────────────────────────────────────────────────────

std::vector<uint8_t> encodeDisd(uint8_t deviceId, const DisplayState& ds) {
    std::vector<uint8_t> f;
    f.reserve(kDisdFrameSize);
    f.push_back(kSysExBegin);
    f.push_back(kMfrWaldorf);
    f.push_back(kDevTypeXT);
    f.push_back(deviceId);
    f.push_back(kIdmDisd);
    for (char c : ds.upperRow) f.push_back(static_cast<uint8_t>(c));
    for (char c : ds.lowerRow) f.push_back(static_cast<uint8_t>(c));
    f.push_back(ds.ledBitmask);
    // Checksum covers bytes 5-85 (80 LCD + 1 LED = 81 bytes)
    f.push_back(computeChecksum(f.data() + 5, 81));
    f.push_back(kSysExEnd);
    return f;
}

std::optional<DisplayState> decodeDisd(const std::vector<uint8_t>& f) {
    if (f.size() != kDisdFrameSize) return std::nullopt;
    if (f[0] != kSysExBegin)        return std::nullopt;
    if (f[1] != kMfrWaldorf)        return std::nullopt;
    if (f[2] != kDevTypeXT)         return std::nullopt;
    if (f[4] != kIdmDisd)           return std::nullopt;
    if (f[87] != kSysExEnd)         return std::nullopt;
    if (!checksumValid(computeChecksum(f.data() + 5, 81), f[86]))
        return std::nullopt;
    DisplayState ds;
    for (size_t i = 0; i < 40; ++i) ds.upperRow[i] = static_cast<char>(f[5 + i]);
    for (size_t i = 0; i < 40; ++i) ds.lowerRow[i] = static_cast<char>(f[45 + i]);
    ds.ledBitmask = f[85];
    return ds;
}

} // namespace mw2xt
