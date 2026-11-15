// SPDX-License-Identifier: GPL-3.0-only
#include <QDir>
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
    void changedKeysOnly()
    {
        QTemporaryDir temp;
        QDir root(temp.path());
        QVERIFY(root.mkpath("old/minecraft"));
        QVERIFY(root.mkpath("new/migration/defaults"));
        auto oldDir = root.filePath("old/minecraft");
        auto target = root.filePath("new");
        put(root.filePath("old/niijima-pack.json"), R"({"token":114514,"pack-id":"17"})");
        put(root.filePath("old/minecraft/options.txt"), "version:2\nsame:A\npersonal:key:x\nremoved:retired\nempty:\n");
        put(root.filePath("new/options.txt"), "version:3\r\nsame:C\r\npersonal:C\r\nadded:1\r\nempty:B\r\n");
        put(root.filePath("new/migration/defaults/options.txt.defaults.17"), "version:1\nsame:A\npersonal:A\nremoved:0\nempty:A\n");
        put(root.filePath("new/niijima-pack.json"), R"({"token":114514,"migration":{"files":[{"path":"options.txt","mode":"mergeChangedKeys","defaults":[{"fromPackIds":["17"],"file":"migration/defaults/options.txt.defaults.17"},{"fromLegacyVersion":"1.5.1","file":"migration/defaults/options.txt.defaults.17"}]}]}})");
        auto source = Niijima::migrationSource(root.filePath("old"));
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
        QVERIFY(!Niijima::migrateSettings(source, {}, target));
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
        QVERIFY(!Niijima::migrateSettings({oldDir, "99"}, "1.5.1", target));
        QVERIFY(Niijima::migrateSettings({oldDir, {}}, "1.5.1", target));
        QVERIFY(!Niijima::migrateSettings({target, "17"}, {}, target));
        QVERIFY(!Niijima::migrateSettings({root.filePath("missing"), "17"}, {}, target));
    }
};
QTEST_GUILESS_MAIN(OptionsMigrationTest)
#include "OptionsMigration_test.moc"
