#pragma once
#include <array>
#include <cstdint>

namespace mw2xt {

// GDATA — 32 bytes of global parameters (Waldorf §3.6).
struct GlobalData {
    std::array<uint8_t, 32> data{};

    uint8_t& operator[](int i)       { return data[static_cast<size_t>(i)]; }
    uint8_t  operator[](int i) const { return data[static_cast<size_t>(i)]; }

    bool operator==(const GlobalData& o) const { return data == o.data; }
    bool operator!=(const GlobalData& o) const { return data != o.data; }
};
static_assert(sizeof(GlobalData) == 32, "GlobalData must be exactly 32 bytes");

} // namespace mw2xt
