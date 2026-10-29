// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

#include "Result.h"
#include "tasks/Task.h"

namespace Niijima {

struct PackRelease {
    quint64 id = 0;
    QString version;
    QUrl packUrl;
};

// IDs are canonical positive decimal quint64 values; an empty requestedId selects the greatest ID.
Result<PackRelease> parsePackIndex(const QByteArray& data, const QUrl& indexUrl, const QString& requestedId = {});

class PackIndexTask : public Task {
    Q_OBJECT
   public:
    explicit PackIndexTask(const QUrl& indexUrl, QString requestedId = {}, QObject* parent = nullptr);

    const PackRelease& release() const { return m_release; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    QUrl m_indexUrl;
    QString m_requestedId;
    PackRelease m_release;
    Task::Ptr m_download;
};

}  // namespace Niijima