#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace mw2xt {

// SDATA — 256 bytes, flat array indexed by Waldorf §3.1 parameter index.
// Reserved indices are preserved on round-trip; zeroed only on fresh-patch init.
struct SoundData {
    std::array<uint8_t, 256> data{};

    uint8_t& operator[](int i)       { return data[static_cast<size_t>(i)]; }
    uint8_t  operator[](int i) const { return data[static_cast<size_t>(i)]; }

    // SDATA 240-255: 16-byte patch name, ASCII, space-padded, no null terminator.
    std::string patchName() const {
        std::string s(reinterpret_cast<const char*>(data.data() + 240), 16);
        auto end = s.find_last_not_of(' ');
        return end == std::string::npos ? "" : s.substr(0, end + 1);
    }

    bool operator==(const SoundData& o) const { return data == o.data; }
    bool operator!=(const SoundData& o) const { return data != o.data; }
};
static_assert(sizeof(SoundData) == 256, "SoundData must be exactly 256 bytes");

} // namespace mw2xt
