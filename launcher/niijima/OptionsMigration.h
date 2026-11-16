// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <optional>
#include <QStringList>
#include "PackIndex.h"

namespace Niijima {
struct MigrationSource {
    QString gameDir;
    QString packId;
    QString channel = DefaultPackChannel;
};

QJsonArray migrationFiles(const QString& gameDir);
MigrationSource migrationSource(const QString& directory, const QJsonArray& files);
bool migrationItemAvailable(const QString& game, const QJsonObject& rule);
QStringList legacyOptionsVersions();
Result<> migrateSettings(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir, const std::optional<QSet<QString>>& selected = std::nullopt);
} // namespace Niijima
