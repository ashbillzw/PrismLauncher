// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QList>
#include <QString>
#include "Result.h"

namespace Niijima {
struct InstalledPackCandidate {
    QString instanceId;
    QString root;
};
struct InstalledPackSelection {
    QString instanceId;
    quint64 packId = 0;
};
QString installedPackVersion(const QString& root);
Result<> writeInstalledPackId(const QString& root, quint64 packId);
InstalledPackSelection selectLatestInstalledPack(const QList<InstalledPackCandidate>& candidates);
} // namespace Niijima
