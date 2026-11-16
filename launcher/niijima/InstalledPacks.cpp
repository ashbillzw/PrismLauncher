// SPDX-License-Identifier: GPL-3.0-only
#include "niijima/PackError.h"
#include "InstalledPacks.h"
#include "PackIndex.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace Niijima {
namespace {
QJsonObject readObject(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject{};
}
}

QString installedPackVersion(const QString& root)
{
    return readObject(QDir(root).filePath("mrpack/modrinth.index.json")).value("versionId").toString().trimmed();
}

QString packMarkerPath(const QString& root, const QString& gameRoot)
{
    for (const auto& directory : { gameRoot, root }) {
        if (directory.isEmpty())
            continue;
        const auto path = QDir(directory).filePath("niijima-pack.json");
        if (parsePackId(readObject(path).value("pack-id").toString()))
            return path;
    }
    return {};
}

Result<> verifyInstalledPackId(const QString& root, const QString& gameRoot, quint64 packId, const QString& channel)
{
    const auto path = packMarkerPath(root, gameRoot);
    if (path.isEmpty() || packId == 0)
        return std::unexpected(packError(3002, QStringLiteral("Cannot record pack-id: invalid Niijima pack marker or pack-id.")));
    const auto metadata = readObject(path);
    if (metadata.value("channel").toString(DefaultPackChannel) != channel)
        return std::unexpected(packError(3003, QStringLiteral("Installed pack channel does not match the release.")));
    if (metadata.value("pack-id").toString() != QString::number(packId))
        return std::unexpected(packError(3002, QStringLiteral("Installed pack-id does not match the release.")));
    return {};
}

InstalledPackSelection selectLatestInstalledPack(const QList<InstalledPackCandidate>& candidates, const QString& channel)
{
    InstalledPackSelection selected;
    for (const auto& candidate : candidates) {
        if (candidate.instanceId.isEmpty() || !QFile::exists(QDir(candidate.root).filePath("instance.cfg")))
            continue;
        const auto path = packMarkerPath(candidate.root, candidate.gameRoot);
        if (path.isEmpty())
            continue;
        const auto marker = readObject(path);
        if (marker.value("channel").toString(DefaultPackChannel) != channel)
            continue;
        const auto packId = parsePackId(marker.value("pack-id").toString());
        if (!packId)
            continue;
        // Same pack-id means the same release; use a stable tie-breaker, not a separate preference list.
        if (selected.instanceId.isEmpty() || *packId > selected.packId ||
            (*packId == selected.packId && candidate.instanceId < selected.instanceId))
            selected = {candidate.instanceId, *packId};
    }
    return selected;
}
} // namespace Niijima
