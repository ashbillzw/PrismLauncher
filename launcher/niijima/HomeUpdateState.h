// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <optional>
#include "niijima/PackIndex.h"

namespace Niijima {
struct HomeUpdateState {
    quint64 generation = 0;
    bool loading = false;
    std::optional<PackRelease> release;

    quint64 begin()
    {
        ++generation;
        loading = true;
        release.reset();
        return generation;
    }
    void timeout(quint64 request)
    {
        if (request == generation)
            loading = false;
    }
    bool complete(quint64 request, const PackRelease& result)
    {
        if (request != generation)
            return false;
        release = result;
        loading = false;
        return true;
    }
};

inline bool needsPackUpdate(const PackRelease& target, quint64 installedId)
{
    // Test/production labels never affect pack-id ordering.
    return target.id > installedId;
}
} // namespace Niijima
