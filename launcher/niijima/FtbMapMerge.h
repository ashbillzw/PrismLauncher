// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDir>
namespace Niijima {
bool mergeFtbWorld(const QDir& old, const QDir& stagedDestination);
}