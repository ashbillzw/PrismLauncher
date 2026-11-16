// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QDebug>
#include <QString>

namespace Niijima {
// A proxy challenge, request timeout or rate limit can occur with a valid release.
inline bool releaseHttpError(int status)
{
    return status >= 400 && status != 407 && status != 408 && status != 429;
}

// Codes are documented in agent/启动器错误码.md.
inline QString packError(int code, const QString& reason)
{
    qWarning().noquote() << "Niijima error" << code << reason;
    return QStringLiteral("发生了错误（错误码：%1）").arg(code);
}
}  // namespace Niijima
