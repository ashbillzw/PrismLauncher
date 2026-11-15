// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QJsonArray>
#include <QStringList>
#include "PackIndex.h"

namespace Niijima {
struct MigrationSource {
    QString gameDir;
    QString packId;
    QString channel = DefaultPackChannel;
};

enum class MigrationError { SameInstance, MissingOptions, MissingDefaults, FileError };

QJsonArray migrationFiles(const QString& gameDir);
MigrationSource migrationSource(const QString& directory);
QStringList legacyOptionsVersions();
Result<void, MigrationError> migrateSettings(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir);
} // namespace Niijima
