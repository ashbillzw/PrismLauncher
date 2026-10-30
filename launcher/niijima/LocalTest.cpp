// SPDX-License-Identifier: GPL-3.0-only
#include "LocalTest.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace Niijima {
QString distributionRoot()
{
    return QStringLiteral("https://mc1.ashbill.com:25572/synology-root/minecraft/niijima-launcher/");
}

LocalTest LocalTest::load(const QString& filename)
{
    LocalTest config;
    if (!QFileInfo::exists(filename))
        return config;
    config.enabled = true;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        config.error = QStringLiteral("无法读取本地测试配置：%1").arg(filename);
        return config;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    const auto object = document.object();
    const auto warning = object.value("testWarning");
    config.testWarningText = warning.toString();
    config.testWarning = object.contains("testWarning") && (!warning.isString() || !config.testWarningText.isEmpty());
    const QDir directory = QFileInfo(filename).absoluteDir();
    config.packs = directory.absoluteFilePath(object.value("packs").toString());
    config.mirror = directory.absoluteFilePath(object.value("mirror").toString());
    if (!document.isObject() || object.value("packs").toString().isEmpty() || object.value("mirror").toString().isEmpty() ||
        !QFileInfo(QDir(config.packs).filePath("latest.json")).isFile() || !QFileInfo(QDir(config.mirror).filePath("meta/v1/index.json")).isFile() ||
        !QDir(config.mirror).exists("assets") || !QDir(config.mirror).exists("files")) {
        config.error = QStringLiteral("本地测试配置无效，请检查 packs/latest.json 和 mirror 目录：%1").arg(filename);
    }
    return config;
}

QUrl LocalTest::resolve(const QUrl& url) const
{
    if (!enabled)
        return url;
    const QUrl root(distributionRoot());
    if (url.scheme() != root.scheme() || url.authority() != root.authority() || !url.path().startsWith(root.path()))
        return url;
    // An invalid local setup must fail locally, not silently contact the production distribution.
    if (!error.isEmpty())
        return QUrl(QStringLiteral("file:"));
    const QString relative = url.path().mid(root.path().size());
    if (QDir::isAbsolutePath(relative) || relative.contains('\\') || relative.split('/').contains(".."))
        return QUrl(QStringLiteral("file:"));
    if (relative.startsWith("packs/"))
        return QUrl::fromLocalFile(QDir(packs).absoluteFilePath(relative.mid(6)));
    if (relative.startsWith("niijima-mirror/"))
        return QUrl::fromLocalFile(QDir(mirror).absoluteFilePath(relative.mid(15)));
    return url;
}

QNetworkReply* LocalTestNetwork::createRequest(Operation operation, const QNetworkRequest& request, QIODevice* outgoingData)
{
    auto local = request;
    if (operation == GetOperation || operation == HeadOperation)
        local.setUrl(m_config.resolve(request.url()));
    return QNetworkAccessManager::createRequest(operation, local, outgoingData);
}
}  // namespace Niijima
