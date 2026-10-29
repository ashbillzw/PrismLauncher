// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QPixmap>
#include <QPointer>
#include <QWidget>
#include "tasks/Task.h"
class QUrl;
class QLabel;
class QPushButton;
class QProgressBar;
class QComboBox;
class InstanceTask;
class NiijimaHomeWindow : public QWidget {
    Q_OBJECT
   public:
    explicit NiijimaHomeWindow(QWidget* parent = nullptr);
   signals:
    void isClosing();

   protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void closeEvent(QCloseEvent*) override;

   private:
    void refresh();
    void refreshPackVersion();
    void primaryAction();
    void onPrismClosing();
    void configure();
    void remember(const QString& id);
    void startImport(const QUrl& url);
    QPixmap m_frostedBackground;
    QLabel* m_status;
    QLabel* m_details;
    QLabel* m_packVersion;
    QPushButton* m_primary;
    QPushButton* m_cancel;
    QComboBox* m_instances;
    QProgressBar* m_progress;
    QPointer<Task> m_task;
    QPointer<InstanceTask> m_import;
    QString m_committedId;
};
