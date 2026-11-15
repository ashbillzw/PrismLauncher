// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

#include "Result.h"
#include "tasks/Task.h"

namespace Niijima {

inline const QString DefaultPackChannel = QStringLiteral("master");

struct PackRelease {
    quint64 id = 0;
    QString version;
    QUrl packUrl;
};

QString channelVersion(QString version, const QString& channel);
Result<quint64> parsePackId(const QString& text);

// IDs are canonical positive decimal quint64 values; an empty requestedId selects the greatest ID.
Result<PackRelease> parsePackIndex(const QByteArray& data, const QUrl& indexUrl, const QString& requestedId = {}, const QString& channel = DefaultPackChannel);
Result<QString> channelForCode(const QByteArray& data, const QString& code);

class PackIndexTask : public Task {
    Q_OBJECT
   public:
    explicit PackIndexTask(const QUrl& indexUrl, QString requestedId = {}, QObject* parent = nullptr, QString channel = DefaultPackChannel);
    void lookupCode(const QString& code) { m_code = code; }
    const QString& channel() const { return m_channel; }

    const PackRelease& release() const { return m_release; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    QUrl m_indexUrl;
    QString m_requestedId;
    QString m_channel;
    QString m_code;
    PackRelease m_release;
    Task::Ptr m_download;
};

}  // namespace Niijima