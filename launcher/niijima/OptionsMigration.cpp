// SPDX-License-Identifier: GPL-3.0-only
#include "OptionsMigration.h"
#include "InstalledPacks.h"
#include "PackIndex.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <memory>
#include <vector>

namespace Niijima {
namespace {
QJsonObject jsonObject(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject{};
}

Result<QByteArray, MigrationError> contents(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Settings migration: open" << path << file.errorString();
        return std::unexpected(MigrationError::FileError);
    }
    auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        qWarning() << "Settings migration: read" << path << file.errorString();
        return std::unexpected(MigrationError::FileError);
    }
    return bytes;
}

QHash<QByteArray, QByteArray> values(const QByteArray& bytes)
{
    QHash<QByteArray, QByteArray> result;
    for (auto line : bytes.split('\n')) {
        if (line.endsWith('\r'))
            line.chop(1);
        const auto colon = line.indexOf(':');
        if (colon > 0)
            result.insert(line.first(colon), line.sliced(colon + 1));
    }
    return result;
}
}

QJsonArray migrationFiles(const QString& gameDir)
{
    const auto marker = jsonObject(QDir(gameDir).filePath("niijima-pack.json"));
    if (marker.value("token").toDouble() != 114514)
        return {};
    return marker.value("migration").toObject().value("files").toArray();
}

MigrationSource migrationSource(const QString& directory)
{
    QDir selected(directory);
    QString game = directory;
    QString root = directory;
    for (const auto& child : {QStringLiteral(".minecraft"), QStringLiteral("minecraft")}) {
        if (!selected.exists("options.txt") && QFileInfo(selected.filePath(child + "/options.txt")).isFile()) {
            game = selected.filePath(child);
            break;
        }
    }
    if (game == directory && (selected.dirName() == ".minecraft" || selected.dirName() == "minecraft") && selected.cdUp()
        && selected.exists("instance.cfg"))
        root = selected.path();
    const auto marker = packMarkerPath(root, game);
    const auto data = marker.isEmpty() ? QJsonObject{} : jsonObject(marker);
    const auto id = data.value("pack-id").toString();
    return {game, parsePackId(id) ? id : QString{}, data.value("channel").toString(DefaultPackChannel)};
}

QStringList legacyOptionsVersions()
{
    return { "1.5.1", "1.5.1.a", "1.5.1.ab", "1.5.1.f", "1.5.1.m",
             "1.5.0", "1.5.0.a", "1.5.0.ab", QStringLiteral("1.5.0（FCL）"), "1.5.0.m",
             "1.4.4", "1.4.4.a", "1.4.4.ab", "1.4.4.f", "1.4.4.m",
             "1.4.3", "1.4.3.a", "1.4.3.ab", "1.4.3.f", "1.4.2" };
}

static bool inside(const QString& directory, const QFileInfo& file)
{
    const auto relative = QDir(QFileInfo(directory).canonicalFilePath()).relativeFilePath(file.canonicalFilePath());
    return QDir::isRelativePath(relative) && relative != ".." && !relative.startsWith("../");
}

static Result<void, MigrationError> prepareFile(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir,
                                                const QJsonObject& entry, std::vector<std::unique_ptr<QSaveFile>>& pending,
                                                QSet<QString>& destinations)
{
    const auto path = entry.value("path").toString();
    const QFileInfo from(QDir(source.gameDir).filePath(path));
    const QFileInfo to(QDir(targetDir).filePath(path));
    if (entry.value("mode") != "mergeChangedKeys" || path.isEmpty() || !QDir::isRelativePath(path)) {
        qWarning() << "Settings migration: unsupported file rule" << path << entry.value("mode").toString();
        return std::unexpected(MigrationError::FileError);
    }
    if (!from.isFile() || !to.isFile()) {
        qWarning() << "Settings migration: missing file" << (!from.isFile() ? from.filePath() : to.filePath());
        return std::unexpected(MigrationError::MissingOptions);
    }
    if (!inside(source.gameDir, from) || !inside(targetDir, to) || destinations.contains(to.canonicalFilePath())) {
        qWarning() << "Settings migration: path outside instance or repeated destination" << path;
        return std::unexpected(MigrationError::FileError);
    }
    destinations.insert(to.canonicalFilePath());
    if (QFileInfo(from.canonicalFilePath()) == QFileInfo(to.canonicalFilePath())) {
        qWarning() << "Settings migration: source equals destination" << from.filePath();
        return std::unexpected(MigrationError::SameInstance);
    }

    QString baseline;
    for (const auto& item : entry.value("defaults").toArray()) {
        const auto rule = item.toObject();
        const bool matches = source.packId.isEmpty()
            ? !legacyVersion.isEmpty() && rule.value("fromLegacyVersion") == legacyVersion
            : rule.value("channel").toString(DefaultPackChannel) == source.channel && rule.value("fromPackIds").toArray().contains(source.packId);
        if (matches) {
            // This rule may read only a bundled options baseline within the new game directory.
            const auto defaultFile = rule.value("file").toString();
            const auto pathInfo = QFileInfo(QDir(targetDir).filePath(defaultFile));
            const auto relative = QDir(QFileInfo(targetDir).canonicalFilePath()).relativeFilePath(pathInfo.canonicalFilePath());
            if (!pathInfo.isFile() || !relative.startsWith("migration/defaults/")) {
                qWarning() << "Settings migration: missing or outside defaults directory:" << pathInfo.filePath();
                return std::unexpected(MigrationError::MissingDefaults);
            }
            baseline = pathInfo.canonicalFilePath();
            break;
        }
    }
    if (baseline.isEmpty()) {
        qWarning() << "Settings migration: no matching baseline" << path << source.packId << legacyVersion << targetDir;
        return std::unexpected(MigrationError::MissingDefaults);
    }
    TRY_INTO(const auto original, contents(baseline));
    TRY_INTO(const auto previous, contents(from.absoluteFilePath()));
    TRY_INTO(const auto current, contents(to.absoluteFilePath()));
    const auto defaults = values(original);
    const auto old = values(previous);
    auto lines = current.split('\n');
    bool changed = false;
    for (auto& line : lines) {
        const auto colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        const auto key = line.first(colon);
        if ((path == "options.txt" && key == "version") || !old.contains(key) || !defaults.contains(key) || defaults.value(key) == old.value(key))
            continue;
        const auto merged = key + ':' + old.value(key) + (line.endsWith('\r') ? "\r" : "");
        if (line != merged) {
            line = merged;
            changed = true;
        }
    }
    if (changed) {
        auto output = std::make_unique<QSaveFile>(to.canonicalFilePath());
        const auto bytes = lines.join('\n');
        if (!output->open(QIODevice::WriteOnly) || output->write(bytes) != bytes.size()) {
            qWarning() << "Settings migration: write" << to.filePath() << output->errorString();
            return std::unexpected(MigrationError::FileError);
        }
        pending.push_back(std::move(output));
    }
    return {};
}

Result<void, MigrationError> migrateSettings(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir)
{
    const auto rules = migrationFiles(targetDir);
    if (rules.isEmpty()) {
        qWarning() << "Settings migration: no file rules in" << targetDir;
        return std::unexpected(MigrationError::FileError);
    }
    std::vector<std::unique_ptr<QSaveFile>> pending;
    QSet<QString> destinations;
    for (const auto& rule : rules) {
        auto result = prepareFile(source, legacyVersion, targetDir, rule.toObject(), pending, destinations);
        if (!result)
            return result;
    }
    for (const auto& file : pending) {
        if (!file->commit()) {
            qWarning() << "Settings migration: commit" << file->fileName() << file->errorString();
            return std::unexpected(MigrationError::FileError);
        }
    }
    return {};
}
} // namespace Niijima
