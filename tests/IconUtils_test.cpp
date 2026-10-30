// SPDX-License-Identifier: GPL-3.0-only
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "icons/IconUtils.h"

class IconUtilsTest : public QObject {
    Q_OBJECT
    static bool writeFile(const QString& path, const QByteArray& data = "icon")
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }

private slots:
    void nativeIconsTakePriority()
    {
        QTemporaryDir temporary;
        const QDir root(temporary.path());
        const auto pcl = root.filePath("overrides/PCL/Logo.png");
        QVERIFY(writeFile(pcl));
        QVERIFY(writeFile(root.filePath("overrides/PCL/Setup.ini"), "Logo:PCL\\Logo.png\nLogoCustom:True\n"));
        QCOMPARE(IconUtils::findPackIcon(root.path(), "custom"), QFileInfo(pcl).canonicalFilePath());
        const auto overrideIcon = root.filePath("overrides/icon.png");
        QVERIFY(writeFile(overrideIcon));
        QCOMPARE(IconUtils::findPackIcon(root.path(), "custom"), overrideIcon);
        const auto rootIcon = root.filePath("icon.png");
        QVERIFY(writeFile(rootIcon));
        QCOMPARE(IconUtils::findPackIcon(root.path(), "custom"), rootIcon);
        const auto keyedIcon = root.filePath("custom.png");
        QVERIFY(writeFile(keyedIcon));
        QCOMPARE(IconUtils::findPackIcon(root.path(), "custom"), keyedIcon);
    }

    void pclUsesConfiguredPath()
    {
        QTemporaryDir temporary;
        const QDir root(temporary.path());
        const auto icon = root.filePath("overrides/art/brand.png");
        QVERIFY(writeFile(icon));
        const auto config = root.filePath("overrides/PCL/Setup.ini");
        QVERIFY(writeFile(config, "Logo:art\\brand.png\r\nLogoCustom:True\r\n"));
        QCOMPARE(IconUtils::findPackIcon(root.path(), "default"), QFileInfo(icon).canonicalFilePath());
        QVERIFY(writeFile(config, "Logo:art\\brand.png\nLogoCustom:False\n"));
        QVERIFY(IconUtils::findPackIcon(root.path(), "default").isEmpty());
        QVERIFY(writeFile(config, "Logo:art\\missing.png\nLogoCustom:True\n"));
        QVERIFY(IconUtils::findPackIcon(root.path(), "default").isEmpty());
    }

    void pclDoesNotReadOutsideThePackLayer()
    {
        QTemporaryDir temporary;
        const QDir root(temporary.path());
        const auto outside = root.filePath("outside.png");
        QVERIFY(writeFile(outside));
        const auto config = root.filePath("overrides/PCL/Setup.ini");
        for (const auto& logo : {QStringLiteral("../outside.png"), outside, QStringLiteral("C:/outside.png"),
                                QStringLiteral("https://example.org/icon.png")}) {
            QVERIFY(writeFile(config, "Logo:" + logo.toUtf8() + "\nLogoCustom:True\n"));
            QVERIFY(IconUtils::findPackIcon(root.path(), "default").isEmpty());
        }
    }
};

QTEST_GUILESS_MAIN(IconUtilsTest)
#include "IconUtils_test.moc"
