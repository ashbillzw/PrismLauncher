// SPDX-License-Identifier: GPL-3.0-only
#include "OptionsMigration.h"
#include "FtbMapMerge.h"
#include "PackError.h"
#include "InstalledPacks.h"
#include "PackIndex.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QLockFile>
#include <QTemporaryDir>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <memory>
#include <vector>

namespace Niijima {
namespace {
QJsonObject jsonObject(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject{};
}

Result<QByteArray> contents(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Settings migration: open" << path << file.errorString();
        return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
    }
    auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        qWarning() << "Settings migration: read" << path << file.errorString();
        return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
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
    if (!parsePackId(marker.value("pack-id").toString()))
        return {};
    return marker.value("migration").toObject().value("files").toArray();
}

MigrationSource migrationSource(const QString& directory, const QJsonArray& files)
{
    const QDir selected(directory);
    const QStringList children{selected.filePath("minecraft"), selected.filePath(".minecraft")};
    QStringList candidates{directory};
    QString game = directory;
    for (const auto& child : children) {
        if (!QFileInfo(child).isDir())
            continue;
        candidates.append(child);
        if (game == directory)
            game = child;
        if (const auto marker = packMarkerPath({}, child); !marker.isEmpty()) {
            const auto data = jsonObject(marker);
            return {child, data.value("pack-id").toString(), data.value("channel").toString(DefaultPackChannel)};
        }
    }
    // Older installs may keep the marker in the instance root instead of the game directory.
    QDir parent = selected;
    const bool selectedGame = (selected.dirName() == "minecraft" || selected.dirName() == ".minecraft")
        && parent.cdUp() && parent.exists("instance.cfg");
    const auto marker = packMarkerPath(selectedGame ? parent.path() : QString{}, directory);
    if (!marker.isEmpty()) {
        const auto data = jsonObject(marker);
        return {selectedGame ? directory : game, data.value("pack-id").toString(), data.value("channel").toString(DefaultPackChannel)};
    }

    // Prefer the directory containing the most available entries (ties keep the original order).
    QString best;
    qsizetype bestCount = 0;
    for (const auto& candidate : candidates) {
        const auto count = std::count_if(files.begin(), files.end(), [&](const auto& item) {
            return migrationItemAvailable(candidate, item.toObject());
        });
        if (count > bestCount) {
            bestCount = count;
            best = candidate;
        }
    }
    if (!best.isEmpty())
        return {best, {}};
    return {};
}

QStringList legacyOptionsVersions()
{
    return { "1.5.1", "1.5.1.a", "1.5.1.ab", "1.5.1.f", "1.5.1.m",
             "1.5.0", "1.5.0.a", "1.5.0.ab", QStringLiteral("1.5.0（FCL）"), "1.5.0.m",
             "1.4.4", "1.4.4.a", "1.4.4.ab", "1.4.4.f", "1.4.4.m",
             "1.4.3", "1.4.3.a", "1.4.3.ab", "1.4.3.f", "1.4.2" };
}

static bool matchesPackId(const QJsonArray& selectors, const QString& sourceId)
{
    const auto id = parsePackId(sourceId);
    if (!id)
        return false;
    for (const auto& item : selectors) {
        const auto text = item.toString();
        if (text == sourceId)
            return true;
        if (!text.startsWith('@'))
            continue;
        const auto dash = text.indexOf('-');
        if (dash < 2)
            continue;
        const auto lower = parsePackId(text.mid(1, dash - 1));
        const auto upper = parsePackId(text.mid(dash + 1));
        if (lower && upper && *lower <= *id && *id <= *upper)
            return true;
    }
    return false;
}

static bool inside(const QString& directory, const QFileInfo& file)
{
    if (!file.exists())
        return false;
    const auto relative = QDir(QFileInfo(directory).canonicalFilePath()).relativeFilePath(file.canonicalFilePath());
    return QDir::isRelativePath(relative) && relative != ".." && !relative.startsWith("../");
}

static const QString ftbMapPath = QStringLiteral("local/ftbchunks/data");

static bool linked(const QFileInfo& entry)
{
    return entry.isSymLink() || entry.isJunction();
}

// A world with only registry tables or an empty waypoints.json has no exploration to migrate.
static QFileInfoList mapWorlds(const QString& game)
{
    const QDir maps(QDir(game).filePath(ftbMapPath));
    QFileInfoList worlds;
    if (!inside(game, QFileInfo(maps.path())))
        return worlds;
    for (const auto& world : maps.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (linked(world) || QUuid(world.fileName()).toString(QUuid::WithoutBraces) != world.fileName().toLower())
            continue;
        const QDir dir(world.absoluteFilePath());
        for (const auto& dim : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (linked(dim))
                continue;
            const QDir region(dim.absoluteFilePath());
            const QFileInfo waypoints(region.filePath("waypoints.json"));
            if (!region.entryList({"*.zip"}, QDir::Files | QDir::NoSymLinks).isEmpty()
                || (!linked(waypoints) && !jsonObject(waypoints.filePath()).value("waypoints").toArray().isEmpty())) {
                worlds.append(world);
                break;
            }
        }
    }
    return worlds;
}

bool migrationItemAvailable(const QString& game, const QJsonObject& rule)
{
    const auto path = QDir::fromNativeSeparators(rule.value("path").toString());
    if (game.isEmpty() || path.isEmpty() || !QDir::isRelativePath(path) || path.split('/').contains(".."))
        return false;
    if (rule.value("mode") == "mergeFtbMaps")
        return path == ftbMapPath && !mapWorlds(game).isEmpty();
    const QFileInfo file(QDir(game).filePath(path));
    return file.isFile() && inside(game, file);
}

static bool copyMapTree(const QDir& source, const QDir& target)
{
    if (!source.isReadable() || !target.mkpath("."))
        return false;
    for (const auto& item : source.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)) {
        const auto to = target.filePath(item.fileName());
        if (linked(item))
            return false;
        if (item.isDir() ? !copyMapTree(QDir(item.absoluteFilePath()), QDir(to))
                         : !item.isFile() || !QFile::copy(item.absoluteFilePath(), to))
            return false;
    }
    return true;
}

static Result<> migrateMapWorlds(const QString& source, const QString& target)
{
    const auto worlds = mapWorlds(source);
    const auto error = QStringLiteral("无法读取或写入配置文件");
    if (worlds.isEmpty())
        return std::unexpected(error);
    QDir output(target);
    for (const auto& part : ftbMapPath.split('/')) {
        const QFileInfo child(output.filePath(part));
        if (linked(child) || (!child.exists() && !output.mkdir(part)) || !output.cd(part))
            return std::unexpected(error);
    }
    QLockFile lock(output.filePath(".niijima-migration.lock"));
    if (!lock.tryLock())
        return std::unexpected(error);
    for (const auto& world : worlds) {
        const QFileInfo destination(output.filePath(world.fileName()));
        if (linked(destination) || inside(world.absoluteFilePath(), QFileInfo(output.path()))
            || (destination.exists() && (!destination.isDir() || destination == world || inside(destination.filePath(), world))))
            return std::unexpected(error);
        QTemporaryDir staged(output.filePath(".niijima-map-XXXXXX"));
        const QDir next(staged.filePath("new"));
        const QDir old(world.absoluteFilePath());
        if (!staged.isValid() || !copyMapTree(destination.exists() ? QDir(destination.absoluteFilePath()) : old, next)
            || (destination.exists() && !mergeFtbWorld(old, next)))
            return std::unexpected(error);
        const auto previous = staged.filePath("old");
        const bool replaced = destination.exists();
        if (replaced && !output.rename(destination.absoluteFilePath(), previous))
            return std::unexpected(error);
        if (!output.rename(staged.filePath("new"), destination.absoluteFilePath())) {
            if (replaced && !output.rename(previous, destination.absoluteFilePath())) {
                staged.setAutoRemove(false);
                qWarning() << "Cannot restore FTB map world; original kept at" << previous;
            }
            return std::unexpected(error);
        }
    }
    return {};
}

static Result<> prepareFile(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir,
                                                const QJsonObject& entry, std::vector<std::unique_ptr<QSaveFile>>& pending,
                                                QSet<QString>& destinations)
{
    const auto path = entry.value("path").toString();
    const QFileInfo from(QDir(source.gameDir).filePath(path));
    const QFileInfo to(QDir(targetDir).filePath(path));
    if (entry.value("mode") != "mergeChangedKeys" || path.isEmpty() || !QDir::isRelativePath(path)
        || QDir::fromNativeSeparators(path).split('/').contains(".."))
        return std::unexpected(packError(4001, QStringLiteral("Invalid migration rule: %1 (%2)").arg(path, entry.value("mode").toString())));
    if (!to.isFile())
        return std::unexpected(packError(4003, QStringLiteral("Missing declared destination file: %1").arg(to.filePath())));
    if (!from.isFile() || !inside(source.gameDir, from)) {
        const auto reason = QStringLiteral("Missing or moved source file %1 (channel %2, pack-id %3)")
                                .arg(from.filePath(), source.channel, source.packId);
        if (!source.packId.isEmpty())
            return std::unexpected(packError(4004, reason));
        qWarning().noquote() << "Settings migration:" << reason;
        return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
    }
    if (!inside(targetDir, to) || destinations.contains(to.canonicalFilePath()))
        return std::unexpected(packError(4001, QStringLiteral("Unsafe or repeated destination: %1").arg(to.filePath())));
    destinations.insert(to.canonicalFilePath());
    if (QFileInfo(from.canonicalFilePath()) == QFileInfo(to.canonicalFilePath())) {
        qWarning() << "Settings migration: source equals destination" << from.filePath();
        return std::unexpected(QStringLiteral("请选择迁移源版本文件夹"));
    }

    QString baseline;
    for (const auto& item : entry.value("defaults").toArray()) {
        const auto rule = item.toObject();
        const bool matches = source.packId.isEmpty()
            ? !legacyVersion.isEmpty() && (rule.value("fromLegacyVersions").toArray().contains(legacyVersion) || rule.value("fromLegacyVersion") == legacyVersion)
            : rule.value("channel").toString(DefaultPackChannel) == source.channel && matchesPackId(rule.value("fromPackIds").toArray(), source.packId);
        if (matches) {
            // This rule may read only a bundled options baseline within the new game directory.
            const auto defaultFile = rule.value("file").toString();
            const auto pathInfo = QFileInfo(QDir(targetDir).filePath(defaultFile));
            const auto relative = QDir(QFileInfo(targetDir).canonicalFilePath()).relativeFilePath(pathInfo.canonicalFilePath());
            if (!pathInfo.isFile() || !(relative.startsWith("ashbill/migration/") || relative.startsWith("migration/defaults/"))) {
                return std::unexpected(packError(4002, QStringLiteral("Missing or invalid baseline: %1").arg(pathInfo.filePath())));
            }
            baseline = pathInfo.canonicalFilePath();
            break;
        }
    }
    if (baseline.isEmpty()) {
        return std::unexpected(packError(4002, QStringLiteral("No baseline for %1 (channel %2, pack-id %3, legacy %4) in %5")
                                                   .arg(path, source.channel, source.packId, legacyVersion, targetDir)));
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
            return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
        }
        pending.push_back(std::move(output));
    }
    return {};
}

Result<> migrateSettings(const MigrationSource& source, const QString& legacyVersion, const QString& targetDir, const std::optional<QSet<QString>>& selected)
{
    if (QFileInfo(source.gameDir) == QFileInfo(targetDir)) {
        qWarning() << "Settings migration: selected the target directory" << targetDir;
        return std::unexpected(QStringLiteral("请选择迁移源版本文件夹"));
    }
    const auto rules = migrationFiles(targetDir);
    if (rules.isEmpty()) {
        return std::unexpected(packError(4001, QStringLiteral("No migration rules in %1").arg(targetDir)));
    }
    std::vector<std::unique_ptr<QSaveFile>> pending;
    QSet<QString> destinations;
    bool copyMaps = false;
    bool hasSelection = false;
    for (const auto& rule : rules) {
        const auto entry = rule.toObject();
        const auto path = entry.value("path").toString();
        if (selected && !selected->contains(path))
            continue;
        hasSelection = true;
        if (entry.value("mode") == "mergeFtbMaps") {
            if (path != ftbMapPath || copyMaps)
                return std::unexpected(packError(4001, QStringLiteral("Invalid FTB map rule")));
            copyMaps = true;
            continue;
        }
        auto result = prepareFile(source, legacyVersion, targetDir, rule.toObject(), pending, destinations);
        if (!result)
            return result;
    }
    if (!hasSelection)
        return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
    if (copyMaps) {
        auto result = migrateMapWorlds(source.gameDir, targetDir);
        if (!result)
            return result;
    }
    for (const auto& file : pending) {
        if (!file->commit()) {
            qWarning() << "Settings migration: commit" << file->fileName() << file->errorString();
            return std::unexpected(QStringLiteral("无法读取或写入配置文件"));
        }
    }
    return {};
}
} // namespace Niijima
