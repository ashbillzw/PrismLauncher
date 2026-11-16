// SPDX-License-Identifier: GPL-3.0-only
#include <QDir>
#include <QBuffer>
#include <QImage>
#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"
#include "niijima/FtbMapMerge.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include "niijima/OptionsMigration.h"

class OptionsMigrationTest : public QObject {
    Q_OBJECT
    static void put(const QString& file, const QByteArray& data)
    {
        QFile out(file);
        QVERIFY(out.open(QIODevice::WriteOnly));
        QCOMPARE(out.write(data), data.size());
    }
private slots:
    void compactRules()
    {
        QTemporaryDir temp;
        QDir root(temp.path());
        QVERIFY(root.mkpath("old"));
        QVERIFY(root.mkpath("new/ashbill/migration"));
        const auto old = root.filePath("old");
        const auto target = root.filePath("new");
        put(root.filePath("old/options.txt"), "key:custom");
        put(root.filePath("new/options.txt"), "key:new");
        put(root.filePath("new/ashbill/migration/old.txt"), "key:old");
        put(root.filePath("new/niijima-pack.json"), R"({"pack-id":"62","migration":{"files":[{"path":"options.txt","mode":"mergeChangedKeys","defaults":[{"fromPackIds":["@2-11","13","@18-61","@14-12","@015-17","@16-18446744073709551616"],"file":"ashbill/migration/old.txt"},{"fromLegacyVersions":["1.5.1","1.5.1.a"],"file":"ashbill/migration/old.txt"}]}]}})");
        for (const auto id : {"2", "6", "11", "13", "18", "61"})
            QVERIFY(Niijima::migrateSettings({old, id}, {}, target));
        for (const auto id : {"1", "12", "14", "15", "16", "17", "62"})
            QVERIFY(!Niijima::migrateSettings({old, id}, {}, target));
        QVERIFY(!Niijima::migrateSettings({old, "18", "admin"}, {}, target));
        QVERIFY(Niijima::migrateSettings({old, {}}, "1.5.1.a", target));
        QVERIFY(!Niijima::migrateSettings({old, {}}, "1.5.1.ab", target));
    }

    void locateSourceFromDeclaredFiles()
    {
        QTemporaryDir temp;
        QDir root(temp.path());
        QVERIFY(root.mkpath("old/.minecraft/config"));
        put(root.filePath("old/.minecraft/config/keys.txt"), "key:B");
        const QJsonArray files{QJsonObject{{"path", "config/keys.txt"}}};
        QCOMPARE(Niijima::migrationSource(root.filePath("old"), files).gameDir, root.filePath("old/.minecraft"));
        put(root.filePath("options.txt"), "key:A");
        QVERIFY(Niijima::migrationSource(root.path(), files).gameDir.isEmpty());
        const QJsonArray both{files.first(), QJsonObject{{"path", "options.txt"}}};
        QCOMPARE(Niijima::migrationSource(root.filePath("old"), both).gameDir, root.filePath("old/.minecraft"));

        // A marked game directory wins even when only the outer directory contains the requested file.
        put(root.filePath("old/options.txt"), "key:C");
        put(root.filePath("old/.minecraft/niijima-pack.json"), R"({"pack-id":"17","channel":"admin"})");
        const auto marked = Niijima::migrationSource(root.filePath("old"), both);
        QCOMPARE(marked.packId, "17");
        QCOMPARE(marked.channel, "admin");
        QCOMPARE(marked.gameDir, root.filePath("old/.minecraft"));
    }

    void preloadedMapWinsPerColumn()
    {
        QTemporaryDir tmp;
        QDir old(tmp.filePath("old")), target(tmp.filePath("target"));
        const QString dim = "minecraft_overworld", zone = dim + "/0EA53-0EA5E.zip";
        for (const auto& dir : {old, target}) {
            QVERIFY(dir.mkpath(dim));
            bool preloaded = dir == target;
            put(dir.filePath("block_map.txt"), preloaded ? "#000001 minecraft:stone\n" : "#000001 minecraft:grass_block\n");
            put(dir.filePath("biome_map.txt"), preloaded ? "#001 minecraft:ocean\n" : "#001 minecraft:plains\n");
            auto index = QByteArray::fromHex(preloaded ? "000100010001" : "00010002ff010001");
            put(dir.filePath(dim+"/dimension.regions"), qCompress(index).mid(4));
            MMCZip::ArchiveWriter zip(dir.filePath(zone));
            QVERIFY(zip.open());
            QVERIFY(zip.addFile("chunks.dat",QByteArray::fromHex("000300010400000000000000000001")));
            for (QString name : {"data.png","foliage.png","grass.png","water.png","blocks.png"}) {
                const bool data = name == "data.png";
                QImage image(512,512,data ? QImage::Format_ARGB32 : QImage::Format_RGB32);
                image.fill(data ? 0x80010000u : 0xff000000u);
                auto mark = [&](int x) {
                    // Height zero and negative heights are both valid mapped columns.
                    image.setPixel(x,0,data ? (preloaded ? 0x0000d001u : 0xffc0a001u) :
                                   name == "blocks.png" ? 0xff000001u :
                                   preloaded ? 0xff112233u : 0xff445566u);
                };
                mark(0);                       // covered by both
                mark(preloaded ? 2 : 1);       // disjoint within the same chunk
                QBuffer buf;
                QVERIFY(buf.open(QIODevice::WriteOnly));
                QVERIFY(image.save(&buf,"PNG"));
                QVERIFY(zip.addFile(name,buf.data()));
            }
            QVERIFY(zip.close());
        }
        QVERIFY(Niijima::mergeFtbWorld(old,target));
        QVERIFY(Niijima::mergeFtbWorld(old,target));
        MMCZip::ArchiveReader zip(target.filePath(zone));
        for (QString name : {"data.png","foliage.png","grass.png","water.png","blocks.png"}) {
            auto bytes = zip.readFile(name);
            QVERIFY(bytes);
            auto image = QImage::fromData(*bytes);
            const bool data = name == "data.png", block = name == "blocks.png";
            const QRgb preloaded = data ? 0x0000d001u : block ? 0xff000001u : 0xff112233u;
            const QRgb player = data ? 0xffc0a002u : block ? 0xff000002u : 0xff445566u;
            QCOMPARE(image.pixel(0,0),preloaded);
            QCOMPARE(image.pixel(1,0),player);
            QCOMPARE(image.pixel(2,0),preloaded);
            if (data) QCOMPARE(image.pixel(3,0),0x80010000u);
        }
        QFile file(target.filePath(dim+"/dimension.regions"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(qUncompress(QByteArray::fromHex("00000008")+file.readAll()),QByteArray::fromHex("000100020001ff01"));
    }


    void partialMapMigration()
    {
        QTemporaryDir tmp;
        const QDir root(tmp.path());
        const QString mapPath = "local/ftbchunks/data";
        const QString world = "9c01a448-6711-49b2-8e8e-09b405d134af";
        const QString other = "b1a2d71d-5a91-4f78-8036-cc8f59b00f40";
        const QString old = root.filePath("old/.minecraft"), target = root.filePath("new");
        const QString sourceDim = old + '/' + mapPath + '/' + world + "/minecraft_overworld";
        const QString destDim = target + '/' + mapPath + '/' + world + "/minecraft_overworld";
        const QString untouched = target + '/' + mapPath + '/' + other;
        QVERIFY(root.mkpath(sourceDim));
        QVERIFY(root.mkpath(destDim));
        QVERIFY(root.mkpath(untouched));
        const QJsonObject settings{{"path","options.txt"},{"mode","mergeChangedKeys"}};
        const QJsonObject maps{{"path",mapPath},{"mode","mergeFtbMaps"}};
        const QJsonArray rules{settings, maps};
        put(target + "/niijima-pack.json", QJsonDocument(QJsonObject{{"pack-id","62"},{"migration",QJsonObject{{"files",rules}}}}).toJson());
        put(sourceDim + "/waypoints.json", R"({"waypoints":[]})");
        QVERIFY(Niijima::migrationSource(root.filePath("old"), rules).gameDir.isEmpty());
        put(old + "/options.txt", "value:mine");
        QVERIFY(Niijima::migrationItemAvailable(old, settings));
        QVERIFY(!Niijima::migrationItemAvailable(old, maps));
        QCOMPARE(Niijima::migrationSource(root.filePath("old"), rules).gameDir, old);
        const QByteArray pins = R"({"waypoints":[{"name":"old"}]})";
        put(sourceDim + "/waypoints.json", pins);
        QVERIFY(Niijima::migrationItemAvailable(old, maps));
        QVERIFY(QFile::remove(old + "/options.txt"));
        QVERIFY(!Niijima::migrationItemAvailable(old, settings));
        QCOMPARE(Niijima::migrationSource(root.filePath("old"), rules).gameDir, old);
        put(destDim + "/target-only.zip", "prior");
        put(untouched + "/preserve.txt", "unchanged");
        put(target + "/options.txt", "value:target");
        const QSet<QString> selection{mapPath};
        QVERIFY(Niijima::migrateSettings({old, {}}, {}, target, selection));
        QVERIFY(Niijima::migrateSettings({old, {}}, {}, target, selection));
        QVERIFY(QFileInfo::exists(destDim + "/target-only.zip"));
        QVERIFY(QFileInfo::exists(untouched + "/preserve.txt"));
        QFile imported(destDim + "/waypoints.json");
        QVERIFY(imported.open(QIODevice::ReadOnly));
        QCOMPARE(imported.readAll(), pins);
        QFile options(target + "/options.txt");
        QVERIFY(options.open(QIODevice::ReadOnly));
        QCOMPARE(options.readAll(), "value:target");
    }


    void changedKeysOnly()
    {
        QTemporaryDir temp;
        QDir root(temp.path());
        QVERIFY(root.mkpath("old/minecraft"));
        QVERIFY(root.mkpath("new/migration/defaults"));
        auto oldDir = root.filePath("old/minecraft");
        auto target = root.filePath("new");
        put(root.filePath("old/niijima-pack.json"), R"({"pack-id":"17"})");
        put(root.filePath("old/minecraft/options.txt"), "version:2\nsame:A\npersonal:key:x\nremoved:retired\nempty:\n");
        put(root.filePath("new/options.txt"), "version:3\r\nsame:C\r\npersonal:C\r\nadded:1\r\nempty:B\r\n");
        put(root.filePath("new/migration/defaults/options.txt.defaults.17"), "version:1\nsame:A\npersonal:A\nremoved:0\nempty:A\n");
        put(root.filePath("new/niijima-pack.json"), R"({"pack-id":"18","migration":{"files":[{"path":"options.txt","mode":"mergeChangedKeys","defaults":[{"fromPackIds":["17"],"file":"migration/defaults/options.txt.defaults.17"},{"fromLegacyVersion":"1.5.1","file":"migration/defaults/options.txt.defaults.17"}]}]}})");
        auto source = Niijima::migrationSource(root.filePath("old"), Niijima::migrationFiles(target));
        QCOMPARE(source.packId, "17");
        QCOMPARE(source.gameDir, oldDir);
        QFile marker(root.filePath("new/niijima-pack.json"));
        QVERIFY(marker.open(QIODevice::ReadOnly));
        auto manifest = QJsonDocument::fromJson(marker.readAll()).object();
        marker.close();
        auto files = manifest["migration"].toObject()["files"].toArray();
        auto secondRule = files.first().toObject();
        secondRule["path"] = "second.txt";
        files.append(secondRule);
        manifest["migration"] = QJsonObject{{"files", files}};
        put(marker.fileName(), QJsonDocument(manifest).toJson());
        put(root.filePath("new/second.txt"), "personal:B\n");
        QCOMPARE(Niijima::migrateSettings(source, {}, target).error(), QStringLiteral("发生了错误（错误码：4004）"));
        QCOMPARE(Niijima::migrateSettings({oldDir, {}}, "1.5.1", target).error(), QStringLiteral("无法读取或写入配置文件"));
        QFile original(root.filePath("new/options.txt"));
        QVERIFY(original.open(QIODevice::ReadOnly));
        QVERIFY(original.readAll().contains("personal:C\r\n"));
        original.close();
        put(root.filePath("old/minecraft/second.txt"), "personal:Z\n");
        QVERIFY(Niijima::migrateSettings(source, {}, target));
        QFile second(root.filePath("new/second.txt"));
        QVERIFY(second.open(QIODevice::ReadOnly));
        QCOMPARE(second.readAll(), "personal:Z\n");
        second.close();
        QFile output(root.filePath("new/options.txt"));
        QVERIFY(output.open(QIODevice::ReadOnly));
        QCOMPARE(output.readAll(), "version:3\r\nsame:C\r\npersonal:key:x\r\nadded:1\r\nempty:\r\n");
        output.close();
        QCOMPARE(Niijima::migrateSettings({oldDir, "99"}, "1.5.1", target).error(), QStringLiteral("发生了错误（错误码：4002）"));
        QVERIFY(Niijima::migrateSettings({oldDir, {}}, "1.5.1", target));
        QCOMPARE(Niijima::migrateSettings({target, "17"}, {}, target).error(), QStringLiteral("请选择迁移源版本文件夹"));
        QVERIFY(!Niijima::migrateSettings({root.filePath("missing"), "17"}, {}, target));
    }
};
QTEST_GUILESS_MAIN(OptionsMigrationTest)
#include "OptionsMigration_test.moc"
