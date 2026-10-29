// SPDX-License-Identifier: GPL-3.0-only
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <limits>

#include "niijima/PackIndex.h"

namespace {
const QUrl indexUrl(QStringLiteral("https://example.org/%E5%88%9B%E4%B8%96/packs/latest.json"));

QByteArray oneRelease(const QString& file, const QString& id = QStringLiteral("1"))
{
    const QJsonObject release{ { "version", "1.6" }, { "file", file } };
    return QJsonDocument(QJsonObject{ { id, release } }).toJson(QJsonDocument::Compact);
}
}  // namespace

class NiijimaPackIndexTest : public QObject {
    Q_OBJECT

   private slots:
    void selectsNumericIdRatherThanVersionText()
    {
        const QByteArray data = R"({"2":{"version":"99.0","file":"older.zip"},"10":{"version":"1.6","file":"newer.zip"}})";
        const auto latest = Niijima::parsePackIndex(data, indexUrl);
        QVERIFY2(latest.has_value(), latest ? "" : qPrintable(latest.error()));
        QCOMPARE(latest->id, quint64(10));
        QCOMPARE(latest->version, QStringLiteral("1.6"));
        QCOMPARE(latest->packUrl.fileName(), QStringLiteral("newer.zip"));

        const auto pinned = Niijima::parsePackIndex(data, indexUrl, QStringLiteral("2"));
        QVERIFY(pinned.has_value());
        QCOMPARE(pinned->id, quint64(2));
        QCOMPARE(pinned->packUrl.fileName(), QStringLiteral("older.zip"));
        QVERIFY(!Niijima::parsePackIndex(data, indexUrl, QStringLiteral("3")));
    }

    void supportsFullQuint64Range()
    {
        const QByteArray data = R"({"9999999999999999999":{"version":"2","file":"a.zip"},"18446744073709551615":{"version":"1","file":"b.zip"}})";
        const auto latest = Niijima::parsePackIndex(data, indexUrl);
        QVERIFY(latest.has_value());
        QCOMPARE(latest->id, std::numeric_limits<quint64>::max());
        QVERIFY(!Niijima::parsePackIndex(oneRelease(QStringLiteral("a.zip"), QStringLiteral("18446744073709551616")), indexUrl));
    }

    void rejectsNonCanonicalIds_data()
    {
        QTest::addColumn<QString>("id");
        QTest::newRow("zero") << QStringLiteral("0");
        QTest::newRow("leading zero") << QStringLiteral("01");
        QTest::newRow("sign") << QStringLiteral("+1");
        QTest::newRow("negative") << QStringLiteral("-1");
        QTest::newRow("decimal") << QStringLiteral("1.0");
        QTest::newRow("non-ascii digit") << QStringLiteral("１");
        QTest::newRow("overflow") << QStringLiteral("18446744073709551616");
    }

    void rejectsNonCanonicalIds()
    {
        QFETCH(QString, id);
        QVERIFY(!Niijima::parsePackIndex(oneRelease(QStringLiteral("a.zip"), id), indexUrl));
        QVERIFY(!Niijima::parsePackIndex(oneRelease(QStringLiteral("a.zip")), indexUrl, id));
    }

    void encodesFilenameWithoutChangingDirectory()
    {
        const QString filename = QStringLiteral("新岛铁道 v1.6.zip");
        const auto release = Niijima::parsePackIndex(oneRelease(filename), indexUrl);
        QVERIFY(release.has_value());
        QCOMPARE(release->packUrl.path(), QStringLiteral("/创世/packs/新岛铁道 v1.6.zip"));
        QVERIFY(release->packUrl.toEncoded().contains("%20"));
        QVERIFY(release->packUrl.toEncoded().contains("/%E5%88%9B%E4%B8%96/packs/"));
        QVERIFY(!release->packUrl.toEncoded().contains("%25E5"));

        const auto literalEscapes = Niijima::parsePackIndex(oneRelease(QStringLiteral("a%2Fb?c#d.zip")), indexUrl);
        QVERIFY(literalEscapes.has_value());
        QVERIFY(literalEscapes->packUrl.toEncoded().endsWith("/packs/a%252Fb%3Fc%23d.zip"));
        QVERIFY(!literalEscapes->packUrl.hasQuery());
        QVERIFY(!literalEscapes->packUrl.hasFragment());
    }

    void rejectsPathsAndAbsoluteUrls_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("empty") << QString();
        QTest::newRow("parent") << QStringLiteral("..");
        QTest::newRow("current") << QStringLiteral(".");
        QTest::newRow("parent path") << QStringLiteral("../pack.zip");
        QTest::newRow("subdirectory") << QStringLiteral("sub/pack.zip");
        QTest::newRow("backslash") << QStringLiteral("sub\\pack.zip");
        QTest::newRow("root") << QStringLiteral("/pack.zip");
        QTest::newRow("absolute URL") << QStringLiteral("https://other.example/pack.zip");
        QTest::newRow("scheme") << QStringLiteral("file:pack.zip");
        QTest::newRow("control") << QStringLiteral("pack\n.zip");
    }

    void rejectsPathsAndAbsoluteUrls()
    {
        QFETCH(QString, file);
        QVERIFY(!Niijima::parsePackIndex(oneRelease(file), indexUrl));
    }

    void rejectsMalformedDocuments()
    {
        const QList<QByteArray> invalid = {
            "not json", "[]", "{}", R"({"1":true})", R"({"1":{"version":1.6,"file":"a.zip"}})",
            R"({"1":{"version":"","file":"a.zip"}})", R"({"1":{"version":"1.6","file":42}})",
            R"({"1":{"version":"1.6","file":"a.zip"},"2":{"version":"1.7","file":"../b.zip"}})"
        };
        for (const auto& data : invalid)
            QVERIFY2(!Niijima::parsePackIndex(data, indexUrl, QStringLiteral("1")), data.constData());
        QVERIFY(!Niijima::parsePackIndex(oneRelease(QStringLiteral("a.zip")), QUrl(QStringLiteral("https://user:pass@example.org/latest.json"))));
        QVERIFY(!Niijima::parsePackIndex(oneRelease(QStringLiteral("a.zip")), QUrl(QStringLiteral("file:///tmp/latest.json"))));
    }
};

QTEST_GUILESS_MAIN(NiijimaPackIndexTest)
#include "NiijimaPackIndex_test.moc"