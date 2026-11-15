// SPDX-License-Identifier: GPL-3.0-only
#include "InstalledPacks.h"
#include "PackIndex.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

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
        if (readObject(path).value("token").toDouble() == 114514)
            return path;
    }
    return {};
}

Result<> writeInstalledPackId(const QString& root, const QString& gameRoot, quint64 packId, const QString& channel)
{
    const auto path = packMarkerPath(root, gameRoot);
    if (path.isEmpty() || packId == 0)
        return std::unexpected(QStringLiteral("Cannot record pack-id: invalid Niijima pack marker or pack-id."));
    auto metadata = readObject(path);
    if (metadata.value("channel").toString(DefaultPackChannel) != channel)
        return std::unexpected(QStringLiteral("Installed pack channel does not match the release."));
    if (metadata.value("pack-id").toString() == QString::number(packId))
        return {};
    if (metadata.contains("pack-id") && !metadata.value("pack-id").toString().isEmpty())
        return std::unexpected(QStringLiteral("Cannot record pack-id: invalid Niijima pack marker or pack-id."));
    metadata.insert("pack-id", QString::number(packId));
    const auto bytes = QJsonDocument(metadata).toJson();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return std::unexpected(file.errorString());
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
