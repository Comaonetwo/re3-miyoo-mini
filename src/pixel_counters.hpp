#pragma once
#include <cstdint>
#ifndef SOFT_PIXEL_STATS
#define SOFT_PIXEL_STATS 1
#endif
namespace soft {
// Optional fine-grained diagnostics only. Keep the storage layout identical
// across modes; core pixel/triangle/error counters and safety checks stay live.
#if SOFT_PIXEL_STATS
using PixelDiagnosticCounter = uint64_t;
#else
struct PixelDiagnosticCounter {
    uint64_t reserved;
    constexpr PixelDiagnosticCounter(uint64_t = 0) : reserved(0) {}
    constexpr operator uint64_t() const { return 0; }
    PixelDiagnosticCounter& operator+=(uint64_t) { return *this; }
    PixelDiagnosticCounter& operator-=(uint64_t) { return *this; }
    PixelDiagnosticCounter& operator++() { return *this; }
    uint64_t operator++(int) { return 0; }
};
#endif
constexpr bool pixelDiagnosticsEnabled() { return SOFT_PIXEL_STATS != 0; }
static_assert(sizeof(PixelDiagnosticCounter)==sizeof(uint64_t), "counter layout");
static_assert(alignof(PixelDiagnosticCounter)==alignof(uint64_t), "counter alignment");
}
