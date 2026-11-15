// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDialog>
#include <QString>
#include <functional>
#include <optional>
#include "niijima/OptionsMigration.h"
class QLabel;
class QComboBox;
class QPushButton;
class QFrame;

class QVBoxLayout;
class HomePanelDialog : public QDialog {
public:
    HomePanelDialog(QWidget* home, const QString& title, int width);
    QVBoxLayout* form;
protected:
    QLabel* m_title;
};
std::optional<QString> requestHomeChannelCode(QWidget* home);
void showHomeChannelError(QWidget* home, const QString& reason);

class MigrationDialog final : public HomePanelDialog {
public:
    MigrationDialog(QWidget* home, QString targetGameDir, Niijima::MigrationSource automatic, QString version, std::function<bool()> busy);
private:
    void apply(const Niijima::MigrationSource& source, const QString& version = {});
    void selectFolder();
    QString m_target;
    Niijima::MigrationSource m_manual;
    std::function<bool()> m_busy;
    QLabel* m_error;
    QLabel* m_autoVersion;
    QComboBox* m_versions;
    QPushButton *m_manualButton, *m_autoButton, *m_confirm;
};

QFrame* migrationOffer(QWidget* home, int top, std::function<void()> confirmed);
