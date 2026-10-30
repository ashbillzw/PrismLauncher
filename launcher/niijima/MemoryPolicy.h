// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <array>
#include <cstdint>

namespace Niijima {
// MiB throughout: interpolate the user-defined total-RAM / maximum-heap anchors.
constexpr int defaultMaxMemoryMiB(uint64_t totalMiB)
{
    if (totalMiB == 0)
        return 4096;  // Hardware discovery failed; retain the upstream fallback.
    struct Point { uint64_t ram; uint64_t heap; };
    constexpr std::array<Point, 6> points{{
        {0, 0}, {6 * 1024, 4 * 1024}, {16 * 1024, 12 * 1024},
        {32 * 1024, 24 * 1024}, {64 * 1024, 32 * 1024}, {128 * 1024, 48 * 1024}
    }};
    for (std::size_t i = 1; i < points.size(); ++i) {
        const auto [ram, heap] = points[i];
        const auto [previousRam, previousHeap] = points[i - 1];
        if (totalMiB <= ram)
            return static_cast<int>(previousHeap + (totalMiB - previousRam) * (heap - previousHeap) / (ram - previousRam));
    }
    return 48 * 1024;
}

struct HeapMemory {
    int minimumMiB;
    int maximumMiB;
};

// Automatic values are transient; never replace the stored manual values.
constexpr HeapMemory heapMemory(bool automatic, int automaticMaximumMiB, int manualMinimumMiB, int manualMaximumMiB)
{
    if (automatic)
        return {automaticMaximumMiB < 512 ? automaticMaximumMiB : 512, automaticMaximumMiB};
    if (manualMinimumMiB < manualMaximumMiB)
        return {manualMinimumMiB, manualMaximumMiB};
    return {manualMaximumMiB, manualMinimumMiB};
}

constexpr bool isLowMemory(uint64_t installedMiB, uint64_t visibleMiB)
{
    if (installedMiB != 0)
        return installedMiB < 16 * 1024;
    // Visible RAM may exclude firmware/device reservations. Unknown is not low.
    return visibleMiB != 0 && visibleMiB < 15 * 1024;
}
}  // namespace Niijima
