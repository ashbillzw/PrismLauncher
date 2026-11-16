// SPDX-License-Identifier: GPL-3.0-only
#include "niijima/PackError.h"
#include "LocalTest.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace Niijima {
QString distributionRoot(const QString& configured)
{
    const QUrl url(configured.trimmed(), QUrl::StrictMode);
    if (url.scheme() != "https" || !url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment())
        return QStringLiteral("https://mc.ashbill.com/launcher/");
    return url.toString(QUrl::StripTrailingSlash | QUrl::FullyEncoded) + '/';
}

QUrl LocalTest::distributionUrl(const QUrl& url) const
{
    // Published component metadata and older caches contain absolute download URLs.
    static const QByteArray official = distributionRoot().toLatin1();
    static const QByteArray previous = "https://mc1.ashbill.com:25572/synology-root/minecraft/niijima-launcher/";
    const QByteArray encoded = url.toEncoded();
    const QByteArray target = root.toLatin1();
    for (const auto* prefix : { &official, &previous }) {
        if (encoded.startsWith(*prefix)) {
            if (target.size() >= prefix->size() && encoded.startsWith(target))
                return url;
            return QUrl::fromEncoded(target + encoded.mid(prefix->size()));
        }
    }
    return url;
}

LocalTest LocalTest::load(const QString& filename)
{
    LocalTest config;
    if (!QFileInfo::exists(filename))
        return config;
    config.enabled = true;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open local test config:" << filename << file.errorString();
        config.error = QStringLiteral("无法读取本地测试配置，或配置不正确。");
        return config;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (!document.isObject()) {
        config.error = packError(2002, QStringLiteral("Invalid local test JSON object in %1: %2").arg(filename, parseError.errorString()));
        return config;
    }
    const auto object = document.object();
    const auto warning = object.value("testWarning");
    config.testWarningText = warning.toString();
    config.testWarning = object.contains("testWarning") && (!warning.isString() || !config.testWarningText.isEmpty());
    const QDir directory = QFileInfo(filename).absoluteDir();
    config.packs = directory.absoluteFilePath(object.value("packs").toString());
    config.mirror = directory.absoluteFilePath(object.value("mirror").toString());
    if (object.value("packs").toString().isEmpty() || object.value("mirror").toString().isEmpty() ||
        !QFileInfo(QDir(config.packs).filePath("latest.json")).isFile() || !QFileInfo(QDir(config.mirror).filePath("meta/v1/index.json")).isFile() ||
        !QDir(config.mirror).exists("assets") || !QDir(config.mirror).exists("files")) {
        qWarning() << "Invalid local test paths in" << filename << "packs:" << config.packs << "mirror:" << config.mirror;
        config.error = QStringLiteral("无法读取本地测试配置，或配置不正确。");
    }
    return config;
}

QUrl LocalTest::resolve(const QUrl& requested) const
{
    const QUrl url = distributionUrl(requested);
    if (!enabled)
        return url;
    const QUrl base(root);
    if (url.scheme() != base.scheme() || url.authority() != base.authority() || !url.path().startsWith(base.path()))
        return url;
    // An invalid local setup must fail locally, not silently contact the production distribution.
    if (!error.isEmpty())
        return QUrl(QStringLiteral("file:"));
    const QString relative = url.path().mid(base.path().size());
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
