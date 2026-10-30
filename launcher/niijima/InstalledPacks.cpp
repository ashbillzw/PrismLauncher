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

Result<> writeInstalledPackId(const QString& root, quint64 packId)
{
    const auto path = QDir(root).filePath("niijima-pack.json");
    auto metadata = readObject(path);
    if (metadata.value("token").toDouble() != 114514 || packId == 0)
        return std::unexpected(QStringLiteral("Cannot record pack-id: invalid Niijima pack marker or pack-id."));
    metadata.insert("pack-id", QString::number(packId));
    const auto bytes = QJsonDocument(metadata).toJson();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return std::unexpected(file.errorString());
    return {};
}

InstalledPackSelection selectLatestInstalledPack(const QList<InstalledPackCandidate>& candidates)
{
    InstalledPackSelection selected;
    for (const auto& candidate : candidates) {
        if (candidate.instanceId.isEmpty() || !QFile::exists(QDir(candidate.root).filePath("instance.cfg")))
            continue;
        const auto metadata = readObject(QDir(candidate.root).filePath("niijima-pack.json"));
        if (metadata.value("token").toDouble() != 114514)
            continue;
        const auto packId = parsePackId(metadata.value("pack-id").toString());
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
