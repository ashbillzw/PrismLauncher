// SPDX-License-Identifier: GPL-3.0-only
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QTemporaryDir>
#include "InstanceImportTask.h"
#include "archive/ArchiveWriter.h"
#include "niijima/PackError.h"
#include <limits>

#include "niijima/PackIndex.h"
#include "niijima/HomeUpdateState.h"

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
    void releaseImportFailures()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const auto path = temp.filePath("release.zip");
        const auto expect = [&](const QString& message) {
            InstanceImportTask task(QUrl::fromLocalFile(path), false);
            task.setNiijimaRelease(true);
            task.setStagingPath(temp.path());
            task.start();
            QTRY_VERIFY(task.isFinished());
            QCOMPARE(task.failReason(), message);
        };
        expect(QStringLiteral("无法读取或写入安装文件"));
        QFile broken(path);
        QVERIFY(broken.open(QIODevice::WriteOnly));
        broken.write("bad zip");
        broken.close();
        expect(QStringLiteral("发生了错误（错误码：5001）"));
        MMCZip::ArchiveWriter writer(path);
        QVERIFY(writer.open());
        QVERIFY(writer.addFile(QStringLiteral("modrinth.index.json"), QByteArray("{")));
        QVERIFY(writer.close());
        expect(QStringLiteral("发生了错误（错误码：5002）"));
        QVERIFY(Niijima::releaseHttpError(403));
        QVERIFY(Niijima::releaseHttpError(404));
        QVERIFY(Niijima::releaseHttpError(500));
        QVERIFY(!Niijima::releaseHttpError(0));
        QVERIFY(!Niijima::releaseHttpError(429));
    }

    void namedChannelUsesItsOwnIds()
    {
        const QByteArray data = R"({"cdks":{"sample":"future","unused":"ashbill","broken":"unknown"},"channels":{
            "master":{"58":{"version":"1.6","file":"base.zip"}},
            "admin":{"1":{"version":"1.6.a","file":"admin.zip"}},
            "future":{"1":{"version":"custom","file":"future.zip"}},
            "ashbill":{}}})";
        QCOMPARE(*Niijima::channelForCode(data, "sample"), QString("future"));
        QCOMPARE(Niijima::channelForCode(data, "wrong").error(), QStringLiteral("cdk无效"));
        QCOMPARE(Niijima::channelForCode(data, "unused").error(), QStringLiteral("发生了错误（错误码：1005）"));
        QCOMPARE(Niijima::channelForCode(data, "broken").error(), QStringLiteral("发生了错误（错误码：1004）"));
        QCOMPARE(Niijima::parsePackIndex(data, indexUrl)->id, quint64(58));
        QCOMPARE(Niijima::parsePackIndex(data, indexUrl, "1", "admin")->version, QString("1.6.a"));
        QCOMPARE(Niijima::parsePackIndex(data, indexUrl, {}, "future")->packUrl.fileName(), QString("future.zip"));
        QVERIFY(!Niijima::parsePackIndex(data, indexUrl, "58", "admin"));
        QVERIFY(!Niijima::parsePackIndex(data, indexUrl, {}, "ashbill"));
        QCOMPARE(Niijima::channelVersion("1.6", "ashbill"), QString("1.6.ab"));
        QCOMPARE(Niijima::channelVersion("1.6", "master"), QString("1.6"));
        QCOMPARE(Niijima::channelVersion("1.6", "admin"), QString("1.6.a"));
        QCOMPARE(Niijima::channelVersion("v1.6.ab", "ashbill"), QString("v1.6.ab"));
        QCOMPARE(Niijima::channelVersion("1.6.1", "master"), QString("1.6.1"));
        QCOMPARE(Niijima::channelVersion("1.6.test0060", "master"), QString("1.6.test0060"));
    }

    void loadingTimeoutKeepsLateUpdate()
    {
        Niijima::HomeUpdateState state;
        const auto request = state.begin();
        QVERIFY(state.loading);
        QVERIFY(!state.release);
        state.timeout(request);
        QVERIFY(!state.loading);
        QVERIFY(!state.release);
        const Niijima::PackRelease release{12, "1.6", QUrl("https://example.org/pack.zip")};
        QVERIFY(state.complete(request, release));
        QVERIFY(!state.loading);
        QCOMPARE(state.release->id, quint64(12));
    }

    void completedAndStaleChecks()
    {
        Niijima::HomeUpdateState state;
        const auto old = state.begin();
        const auto current = state.begin();
        state.timeout(old);
        QVERIFY(state.loading);
        const Niijima::PackRelease release{12, "1.6", QUrl("https://example.org/pack.zip")};
        QVERIFY(!state.complete(old, release));
        QVERIFY(!state.release);
        QVERIFY(state.complete(current, release));
        QVERIFY(!state.loading); // Fast success releases the gate before one second.
        state.timeout(current);
        QCOMPARE(state.release->id, quint64(12));
    }

    void releaseIdsNeverGoBackwards()
    {
        const Niijima::PackRelease production{12, "1.6.0", {}};
        const Niijima::PackRelease test{11, "1.6.test9999", {}};
        QVERIFY(Niijima::needsPackUpdate(production, 11));
        QVERIFY(!Niijima::needsPackUpdate(test, 12));
        QVERIFY(!Niijima::needsPackUpdate(production, 12));
        QVERIFY(!Niijima::needsPackUpdate(production, 13));
        QVERIFY(Niijima::needsPackUpdate(production, 0));
        // A new release ID still wins when the display label stays unchanged.
        const Niijima::PackRelease sameLabel{13, "1.6.0", {}};
        QVERIFY(Niijima::needsPackUpdate(sameLabel, 12));
    }

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

    void playerErrorOmitsTechnicalDetails()
    {
        const auto release = Niijima::parsePackIndex(oneRelease("../private.zip"), indexUrl);
        QVERIFY(!release);
        QCOMPARE(release.error(), QStringLiteral("发生了错误（错误码：1009）"));
        const auto lookup = Niijima::channelForCode("{", "sample");
        QVERIFY(!lookup);
        QCOMPARE(lookup.error(), QStringLiteral("发生了错误（错误码：1002）"));
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