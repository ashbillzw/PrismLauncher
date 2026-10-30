// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QPixmap>
#include <QHash>
#include <QPointer>
#include <QWidget>
#include "tasks/Task.h"
#include "niijima/HomeUpdateState.h"
class QShowEvent;
class QUrl;
class QLabel;
class QPushButton;
class QProgressBar;
class QComboBox;
class InstanceTask;
class MinecraftInstance;
class NiijimaHomeWindow : public QWidget {
    Q_OBJECT
   public:
    explicit NiijimaHomeWindow(QWidget* parent = nullptr);
   signals:
    void isClosing();

   protected:
    void showEvent(QShowEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void closeEvent(QCloseEvent*) override;

   private:
    void checkForUpdates();
    void cancelUpdateCheck();
    bool updateAvailable() const;
    void saveReleaseReceipt(const Niijima::PackRelease& release, const QString& source);
    void refresh();
    void refreshPackVersion();
    void refreshPrimaryAction();
    void watchGameWindow(MinecraftInstance* instance);
    void primaryAction();
    void onPrismClosing();
    void showDownloadError(const QString& message, const QString& details);
    void startReleaseLookup(const QUrl& indexUrl, const QString& requestedId);
    void remember(const QString& id);
    void startImport(const QUrl& url);
    void updateTransferText(const TaskStepProgress& progress);
    QPixmap m_frostedBackground;
    QLabel* m_status;
    QLabel* m_details;
    QLabel* m_packVersion;
    QPushButton* m_primary;
    QPushButton* m_cancel;
    QComboBox* m_instances;
    QProgressBar* m_progress;
    Niijima::HomeUpdateState m_update;
    QPointer<Task> m_checkTask;
    QHash<QString, quint64> m_releaseIdsByVersion;
    QString m_releaseSource;
    QPointer<Task> m_task;
    QPointer<InstanceTask> m_import;
    QString m_committedId;
    QString m_loginInstance;
    QHash<QUuid, TaskStepProgress> m_transfers;
};
