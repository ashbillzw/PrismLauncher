// SPDX-License-Identifier: GPL-3.0-only
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include "niijima/InstalledPacks.h"

namespace {
bool makeInstance(const QString& root, const QString& packId, int token = 114514)
{
    if (!QDir().mkpath(root))
        return false;
    QFile config(QDir(root).filePath("instance.cfg"));
    if (!config.open(QIODevice::WriteOnly))
        return false;
    config.close();
    QFile marker(QDir(root).filePath("niijima-pack.json"));
    if (!marker.open(QIODevice::WriteOnly))
        return false;
    const auto bytes = QJsonDocument(QJsonObject{{"token", token}, {"pack-id", packId}}).toJson();
    return marker.write(bytes) == bytes.size();
}
}

class NiijimaInstalledPacksTest : public QObject {
    Q_OBJECT
private slots:
    void selectsNumericMaximumOfflineAndIgnoresNames()
    {
        QTemporaryDir dir;
        QVERIFY(makeInstance(dir.filePath("old"), "2"));
        QVERIFY(makeInstance(dir.filePath("new"), "10"));
        QVERIFY(makeInstance(dir.filePath("unrelated"), "999", 123));
        const auto result = Niijima::selectLatestInstalledPack({
            {"新岛铁道 test9999", dir.filePath("old")},
            {"renamed instance", dir.filePath("new")},
            {"新岛铁道 fake", dir.filePath("unrelated")}
        });
        QCOMPARE(result.instanceId, QStringLiteral("renamed instance"));
        QCOMPARE(result.packId, quint64(10));
    }
    void deterministicTieAndMissingInstance()
    {
        QTemporaryDir dir;
        QVERIFY(makeInstance(dir.filePath("a"), "3"));
        QVERIFY(makeInstance(dir.filePath("b"), "3"));
        const QList<Niijima::InstalledPackCandidate> candidates{
            {"b", dir.filePath("b")}, {"a", dir.filePath("a")}
        };
        QCOMPARE(Niijima::selectLatestInstalledPack(candidates).instanceId, QStringLiteral("a"));
        QVERIFY(QFile::remove(dir.filePath("a/instance.cfg")));
        QCOMPARE(Niijima::selectLatestInstalledPack(candidates).instanceId, QStringLiteral("b"));
    }
    void skipsInvalidPackIds()
    {
        QTemporaryDir dir;
        for (const auto& value : {"", "0", "01", "+3", "18446744073709551616"}) {
            QVERIFY(makeInstance(dir.path(), value));
            QVERIFY(Niijima::selectLatestInstalledPack({{"test", dir.path()}}).instanceId.isEmpty());
        }
    }
    void writesOnlyPackIdAlongsideToken()
    {
        QTemporaryDir dir;
        QVERIFY(makeInstance(dir.path(), ""));
        QVERIFY(Niijima::writeInstalledPackId(dir.path(), 3).has_value());
        QFile marker(dir.filePath("niijima-pack.json"));
        QVERIFY(marker.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(marker.readAll()).object();
        QCOMPARE(object.size(), 2);
        QCOMPARE(object.value("token").toInt(), 114514);
        QCOMPARE(object.value("pack-id").toString(), QStringLiteral("3"));
        QVERIFY(!Niijima::writeInstalledPackId(dir.path(), 0).has_value());
    }
};
QTEST_GUILESS_MAIN(NiijimaInstalledPacksTest)
#include "NiijimaInstalledPacks_test.moc"
