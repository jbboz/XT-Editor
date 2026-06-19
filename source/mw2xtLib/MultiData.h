#pragma once
#include <array>
#include <cstdint>

namespace mw2xt {

// IDATA — 28 bytes per Multi-mode instrument (Waldorf §3.3; D-04 confirmed).
struct InstrumentData {
    std::array<uint8_t, 28> data{};

    uint8_t& operator[](int i)       { return data[static_cast<size_t>(i)]; }
    uint8_t  operator[](int i) const { return data[static_cast<size_t>(i)]; }

    bool operator==(const InstrumentData& o) const { return data == o.data; }
    bool operator!=(const InstrumentData& o) const { return data != o.data; }
};
static_assert(sizeof(InstrumentData) == 28, "InstrumentData must be exactly 28 bytes");

// MULD payload — 32 MDATA bytes + 8 IDATA = 256 bytes total (Waldorf §2.22).
struct MultiData {
    std::array<uint8_t, 32> mdata{};   // Multi-level parameters (Waldorf §3.2)
    std::array<InstrumentData, 8> idata{};  // Instruments 1-8
};
static_assert(sizeof(MultiData) == 256, "MultiData must be exactly 256 bytes");

} // namespace mw2xt
