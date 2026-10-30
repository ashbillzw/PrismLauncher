// SPDX-License-Identifier: GPL-3.0-only
#include <utility>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "niijima/LocalTest.h"
#include "niijima/MemoryPolicy.h"
#include "net/ChecksumValidator.h"
#include "net/FileSink.h"

class NiijimaLocalTestTest : public QObject {
    Q_OBJECT

private slots:
    void memoryPolicy()
    {
        using Niijima::defaultMaxMemoryMiB;
        QCOMPARE(defaultMaxMemoryMiB(0), 4096);
        for (const auto [ram, heap] : {std::pair{3, 2}, {6, 4}, {11, 8}, {16, 12}, {24, 18},
                                       {32, 24}, {48, 28}, {64, 32}, {96, 40}, {128, 48}, {256, 48}})
            QCOMPARE(defaultMaxMemoryMiB(ram * 1024), heap * 1024);
        int previous = 0;
        for (uint64_t ram = 1024; ram <= 256 * 1024; ++ram) {
            const auto heap = defaultMaxMemoryMiB(ram);
            QVERIFY(heap >= previous);
            QVERIFY(heap <= 48 * 1024);
            QVERIFY(static_cast<uint64_t>(heap) <= ram);
            previous = heap;
        }
        QVERIFY(!Niijima::isLowMemory(16 * 1024, 14 * 1024));
        QVERIFY(Niijima::isLowMemory(12 * 1024, 12 * 1024));
        QVERIFY(Niijima::isLowMemory(16 * 1024 - 1, 15 * 1024));
        QVERIFY(!Niijima::isLowMemory(0, 15 * 1024));
        QVERIFY(Niijima::isLowMemory(0, 15 * 1024 - 1));
        QVERIFY(!Niijima::isLowMemory(0, 0));
    }

    void automaticAndManualMemory()
    {
        // An old persisted 4 GiB limit must not defeat the default automatic mode.
        for (const auto [ram, heap] : {std::pair{16, 12}, {64, 32}, {128, 48}, {256, 48}}) {
            const auto automatic = Niijima::heapMemory(true, Niijima::defaultMaxMemoryMiB(ram * 1024), 2048, 4096);
            QCOMPARE(automatic.minimumMiB, 512);
            QCOMPARE(automatic.maximumMiB, heap * 1024);
            const auto manual = Niijima::heapMemory(false, Niijima::defaultMaxMemoryMiB(ram * 1024), 2048, 4096);
            QCOMPARE(manual.minimumMiB, 2048);
            QCOMPARE(manual.maximumMiB, 4096);
        }
        const auto reversedManual = Niijima::heapMemory(false, 32768, 8192, 1024);
        QCOMPARE(reversedManual.minimumMiB, 1024);
        QCOMPARE(reversedManual.maximumMiB, 8192);
        const auto unknown = Niijima::heapMemory(true, Niijima::defaultMaxMemoryMiB(0), 8192, 16384);
        QCOMPARE(unknown.minimumMiB, 512);
        QCOMPARE(unknown.maximumMiB, 4096);
    }

    void warningKeyPresence()
    {
        QTemporaryDir temporary;
        const auto path = temporary.filePath("config.json");
        for (const auto& json : {QByteArray("{}"), QByteArray("{\"testWarning\":true}"),
                                 QByteArray("{\"testWarning\":false}"), QByteArray("{\"testWarning\":null}")}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(json);
            file.close();
            const auto config = Niijima::LocalTest::load(path);
            QCOMPARE(config.testWarning, json.contains("testWarning"));
            QVERIFY(config.testWarningText.isEmpty());
        }
    }

    void warningCustomText()
    {
        QTemporaryDir temporary;
        const auto path = temporary.filePath("config.json");
        const QString text = QStringLiteral("这是一条测试警告。\n第二行");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(QJsonObject{{"testWarning", text}}).toJson());
        file.close();
        const auto config = Niijima::LocalTest::load(path);
        QVERIFY(config.testWarning);
        QCOMPARE(config.testWarningText, text);
    }

    void missingAndInvalidConfig()
    {
        QTemporaryDir temporary;
        const auto path = temporary.filePath("config.json");
        QVERIFY(!Niijima::LocalTest::load(path).enabled);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{}");
        file.close();
        const auto config = Niijima::LocalTest::load(path);
        QVERIFY(config.enabled);
        QVERIFY(!config.error.isEmpty());
    }

    void localRequestsKeepChecksumValidation()
    {
        QTemporaryDir temporary;
        QDir root(temporary.path());
        QVERIFY(root.mkpath("mirror/meta/v1"));
        QVERIFY(root.mkpath("mirror/assets"));
        QVERIFY(root.mkpath("mirror/files"));
        for (const auto& name : { "pack.zip", "mirror/meta/v1/index.json", "mirror/files/test.bin" }) {
            QFile file(root.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("test payload");
        }
        QFile configFile(root.filePath("config.json"));
        QVERIFY(configFile.open(QIODevice::WriteOnly));
        configFile.write(QJsonDocument(QJsonObject{ { "pack", "pack.zip" }, { "mirror", "mirror" } }).toJson());
        configFile.close();
        const auto config = Niijima::LocalTest::load(configFile.fileName());
        QVERIFY2(config.error.isEmpty(), qPrintable(config.error));
        QCOMPARE(config.pack, root.filePath("pack.zip"));
        const QUrl url(Niijima::distributionRoot() + "niijima-mirror/files/test.bin");
        QVERIFY(config.resolve(url).isLocalFile());
        const QUrl unrelated("https://example.org/file");
        QCOMPARE(config.resolve(unrelated), unrelated);
        QCOMPARE(Niijima::LocalTest().resolve(url), url);

        Niijima::LocalTestNetwork network(config);
        for (const bool validHash : { true, false }) {
            const auto target = root.filePath(validHash ? "good.bin" : "bad.bin");
            QNetworkRequest request(url);
            Net::FileSink sink(target);
            sink.addValidator(new Net::ChecksumValidator(
                QCryptographicHash::Sha256, QCryptographicHash::hash(validHash ? "test payload" : "wrong", QCryptographicHash::Sha256)));
            QVERIFY(sink.init(request).has_value());
            const std::unique_ptr<QNetworkReply> reply(network.get(request));
            QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);
            QCOMPARE(reply->error(), QNetworkReply::NoError);
            QVERIFY(reply->url().isLocalFile());
            QVERIFY(sink.write(reply->readAll()).has_value());
            QCOMPARE(sink.finalize(*reply).has_value(), validHash);
            if (!validHash)
                sink.abort();
            QCOMPARE(QFile::exists(target), validHash);
        }
        const std::unique_ptr<QNetworkReply> missing(network.get(QNetworkRequest(QUrl(Niijima::distributionRoot() + "niijima-mirror/files/missing.bin"))));
        QTRY_VERIFY_WITH_TIMEOUT(missing->isFinished(), 5000);
        QVERIFY(missing->error() != QNetworkReply::NoError);
        QVERIFY(missing->url().isLocalFile());
    }
};

QTEST_GUILESS_MAIN(NiijimaLocalTestTest)
#include "NiijimaLocalTest_test.moc"
