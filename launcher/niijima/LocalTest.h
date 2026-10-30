// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

namespace Niijima {
QString distributionRoot();

struct LocalTest {
    bool enabled = false;
    bool testWarning = false;
    QString testWarningText;
    QString packs;
    QString mirror;
    QString error;

    static LocalTest load(const QString& filename);
    QUrl resolve(const QUrl& url) const;
};

class LocalTestNetwork : public QNetworkAccessManager {
public:
    explicit LocalTestNetwork(const LocalTest& config) : m_config(config) {}

protected:
    QNetworkReply* createRequest(Operation operation, const QNetworkRequest& request, QIODevice* outgoingData) override;

private:
    LocalTest m_config;
};
}  // namespace Niijima
