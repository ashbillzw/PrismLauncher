// SPDX-License-Identifier: GPL-3.0-only
#include "niijima/PackIndex.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <limits>
#include <utility>

#include "Application.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

namespace Niijima {
namespace {

bool validIndexUrl(const QUrl& url)
{
    const bool local = url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
    return url.isValid() && !url.host().isEmpty() && (url.scheme() == "https" || (local && url.scheme() == "http")) &&
           url.userInfo().isEmpty() && !url.hasFragment();
}

bool validFileName(const QString& name)
{
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\') || name.contains(':'))
        return false;
    for (const QChar ch : name) {
        if (ch.unicode() < 0x20 || ch.unicode() == 0x7f)
            return false;
    }
    return true;
}

}  // namespace

Result<quint64> parsePackId(const QString& text)
{
    if (text.isEmpty() || text.startsWith(QLatin1Char('0')))
        return std::unexpected(QStringLiteral("pack-id 必须是无前导零的十进制正整数。"));

    quint64 value = 0;
    for (const QChar ch : text) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return std::unexpected(QStringLiteral("pack-id 必须是无前导零的十进制正整数。"));
        const quint64 digit = ch.unicode() - ushort('0');
        if (value > (std::numeric_limits<quint64>::max() - digit) / 10)
            return std::unexpected(QStringLiteral("pack-id 超出支持的整数范围。"));
        value = value * 10 + digit;
    }
    return value;
}

Result<PackRelease> parsePackIndex(const QByteArray& data, const QUrl& indexUrl, const QString& requestedId)
{
    if (!validIndexUrl(indexUrl))
        return std::unexpected(QStringLiteral("版本索引地址无效；请使用 HTTPS 地址。"));

    const QString requested = requestedId.trimmed();
    quint64 requestedNumber = 0;
    if (!requested.isEmpty()) {
        auto id = parsePackId(requested);
        if (!id)
            return std::unexpected(id.error());
        requestedNumber = *id;
    }

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
        return std::unexpected(QStringLiteral("版本索引不是有效 JSON：%1").arg(error.errorString()));
    if (!document.isObject() || document.object().isEmpty())
        return std::unexpected(QStringLiteral("版本索引必须是至少包含一个版本的 JSON 对象。"));

    PackRelease selected;
    const auto object = document.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        auto id = parsePackId(it.key());
        if (!id)
            return std::unexpected(QStringLiteral("版本索引中的 pack-id“%1”无效：%2").arg(it.key(), id.error()));
        if (!it.value().isObject())
            return std::unexpected(QStringLiteral("版本 %1 的记录必须是对象。").arg(it.key()));

        const auto entry = it.value().toObject();
        const auto version = entry.value(QStringLiteral("version"));
        const auto file = entry.value(QStringLiteral("file"));
        if (!version.isString() || version.toString().trimmed().isEmpty())
            return std::unexpected(QStringLiteral("版本 %1 缺少有效的 version 字符串。").arg(it.key()));
        if (!file.isString() || !validFileName(file.toString()))
            return std::unexpected(QStringLiteral("版本 %1 的 file 必须是 packs 目录内的单个文件名。").arg(it.key()));

        if ((requestedNumber != 0 && *id == requestedNumber) || (requestedNumber == 0 && *id > selected.id)) {
            // setPath treats the filename as decoded text: Chinese, spaces, %, ? and # remain filename characters.
            // Resolve only this relative filename so an already encoded index path is not encoded a second time.
            QUrl relative;
            relative.setPath(file.toString(), QUrl::DecodedMode);
            selected = PackRelease{ *id, version.toString().trimmed(), indexUrl.resolved(relative) };
        }
    }

    if (selected.id == 0)
        return std::unexpected(QStringLiteral("版本索引中不存在指定的 pack-id %1。").arg(requested));
    return selected;
}

PackIndexTask::PackIndexTask(const QUrl& indexUrl, QString requestedId, QObject* parent)
    : m_indexUrl(indexUrl), m_requestedId(std::move(requestedId))
{
    setParent(parent);
    setObjectName(QStringLiteral("Niijima::PackIndex"));
    setAbortable(true);
}

void PackIndexTask::executeTask()
{
    m_release = {};
    if (!validIndexUrl(m_indexUrl)) {
        emitFailed(QStringLiteral("版本索引地址无效；请检查 NiijimaPackIndexUrl。"));
        return;
    }
    // Reject a malformed pinned ID without making an unnecessary request.
    if (const QString requested = m_requestedId.trimmed(); !requested.isEmpty()) {
        if (auto id = parsePackId(requested); !id) {
            emitFailed(id.error());
            return;
        }
    }

    setStatus(QStringLiteral("正在获取整合包版本…"));
    auto download = makeShared<NetJob>(QStringLiteral("Niijima::PackIndex"), APPLICATION->network());
    download->setAskRetry(false);
    auto [action, response] = Net::ApiRequest::makeByteArray(m_indexUrl);
    download->addNetAction(action);
    connect(action.get(), &Task::progress, this, &Task::setProgress);
    connect(download.get(), &Task::stepProgress, this, &PackIndexTask::propagateStepProgress);
    connect(download.get(), &Task::details, this, &Task::setDetails);
    connect(download.get(), &Task::failed, this, [this](const QString& reason) {
        if (isRunning())
            emitFailed(QStringLiteral("获取整合包版本失败：%1").arg(reason));
    });
    connect(download.get(), &Task::aborted, this, [this] {
        if (isRunning())
            emitAborted();
    });
    connect(download.get(), &Task::succeeded, this, [this, response] {
        if (!isRunning())
            return;
        auto result = parsePackIndex(*response, m_indexUrl, m_requestedId);
        if (!result) {
            emitFailed(result.error());
            return;
        }
        m_release = std::move(*result);
        emitSucceeded();
    });
    m_download = download;
    download->start();
}

bool PackIndexTask::abort()
{
    if (!isRunning())
        return false;
    if (m_download && m_download->isRunning() && !m_download->abort())
        return false;
    // NetJob also emits aborted synchronously; avoid emitting a second terminal signal.
    if (isRunning())
        emitAborted();
    return true;
}

}  // namespace Niijima