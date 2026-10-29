// SPDX-License-Identifier: GPL-3.0-only
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "niijima/LocalTest.h"
#include "net/ChecksumValidator.h"
#include "net/FileSink.h"

class NiijimaLocalTestTest : public QObject {
    Q_OBJECT

private slots:
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
