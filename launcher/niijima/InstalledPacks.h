// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QList>
#include <QString>
#include "PackIndex.h"

namespace Niijima {
struct InstalledPackCandidate {
    QString instanceId;
    QString root;
    QString gameRoot;
};
struct InstalledPackSelection {
    QString instanceId;
    quint64 packId = 0;
};
QString installedPackVersion(const QString& root);
QString packMarkerPath(const QString& root, const QString& gameRoot);
Result<> verifyInstalledPackId(const QString& root, const QString& gameRoot, quint64 packId, const QString& channel = DefaultPackChannel);
InstalledPackSelection selectLatestInstalledPack(const QList<InstalledPackCandidate>& candidates, const QString& channel = DefaultPackChannel);
} // namespace Niijima
