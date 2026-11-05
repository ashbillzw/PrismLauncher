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
class NiijimaInstallStatus;
class InstanceTask;
class MinecraftInstance;
class NiijimaHomeWindow : public QWidget {
    Q_OBJECT
   public:
    explicit NiijimaHomeWindow(QWidget* parent = nullptr);
    void exitWithPrism();
   signals:
    void isClosing();

   protected:
    void showEvent(QShowEvent*) override;
    void changeEvent(QEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void closeEvent(QCloseEvent*) override;
#ifdef Q_OS_WIN
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
#endif

   private:
    void checkForUpdates(const QString& launchId = {});
    void launchInstance(MinecraftInstance* instance);
    void cancelUpdateCheck();
    bool updateAvailable() const;
    Result<> savePackId(quint64 packId);
    void selectLatestInstance();
    void refresh();
    void refreshPackVersion();
    void refreshPrimaryAction();
    void watchGameWindow(MinecraftInstance* instance);
    void primaryAction();
    void showDownloadError(const QString& message, const QString& details);
    void startReleaseLookup(const QUrl& indexUrl, const QString& requestedId);
    void startImport(const QUrl& url);
    void updateTransferText(const TaskStepProgress& progress);
    QPixmap m_frostedBackground;
    NiijimaInstallStatus* m_installStatus;
    QLabel* m_status;
    QLabel* m_details;
    QLabel* m_packVersion;
    QPushButton* m_primary;
    QPushButton* m_cancel;
    QString m_selectedInstance;
    quint64 m_selectedPackId = 0;
    QProgressBar* m_progress;
    Niijima::HomeUpdateState m_update;
    QPointer<Task> m_checkTask;
    QPointer<Task> m_task;
    QPointer<InstanceTask> m_import;
    QString m_committedId;
    QString m_loginInstance;
    QHash<QUuid, TaskStepProgress> m_transfers;
};
