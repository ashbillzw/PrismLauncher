// SPDX-License-Identifier: GPL-3.0-only
#include "MigrationDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QPropertyAnimation>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr auto panelStyle = R"(
    QFrame#migrationPanel { background: transparent; border: none; }
    QFrame#panelBody, QFrame#migrationPanel[attached="true"] { background: #fffcf7; border-bottom-left-radius: 9px; border-bottom-right-radius: 9px; }
    QLabel { color: #352c22; background: transparent; }
    QLabel#migrationHeading { background: #f39801; color: white; border-top-left-radius: 9px; border-top-right-radius: 9px; padding: 9px 16px 10px; }
    QPushButton, QComboBox, QLineEdit { color: #382e23; background: white; border: 1px solid #b8a896; border-radius: 5px; padding: 6px 11px; }
    QPushButton:hover, QComboBox:hover { background: #fff0d7; border-color: #bd812f; }
    QPushButton:pressed { background: #f5dfbc; }
    QPushButton[primary="true"] { background: #f39801; border: 1px solid #c47a06; color: #2e2010; font-weight: 600; }
    QPushButton[primary="true"]:hover { background: #ffa919; }
    QPushButton[primary="true"]:pressed { background: #dd8800; }
    QPushButton:focus, QComboBox:focus, QLineEdit:focus { border: 1px solid #714211; }
    QPushButton:disabled { color: #877e72; background: #ede9e2; border-color: #d9d1c5; }
    QPushButton#dismiss { background: transparent; color: #665b50; border-color: transparent; }
    QPushButton#dismiss:hover, QPushButton#dismiss:focus { background: #f4eee5; border-color: #dbcdba; }
)";
QFrame* panel(QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName("migrationPanel");
    frame->setStyleSheet(panelStyle);
    return frame;
}
QLabel* heading(const QString& text, QWidget* parent, int size)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("migrationHeading");
    label->setTextFormat(Qt::PlainText);
    auto font = label->font();
    font.setPixelSize(size);
    font.setBold(true);
    label->setFont(font);
    return label;
}

QVBoxLayout* panelBody(QFrame* frame, QLabel* title)
{
    auto* shell = new QVBoxLayout(frame);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);
    shell->addWidget(title);
    auto* body = new QFrame(frame);
    body->setObjectName("panelBody");
    shell->addWidget(body);
    auto* form = new QVBoxLayout(body);
    form->setContentsMargins(14, 12, 14, 12);
    form->setSpacing(10);
    return form;
}

}

QFrame* migrationOffer(QWidget* home, int top, std::function<void()> confirmed)
{
    auto* clip = new QFrame(home);
    clip->setAttribute(Qt::WA_DeleteOnClose);
    auto* frame = panel(clip);
    frame->setProperty("attached", true);
    auto* row = new QHBoxLayout(frame);
    row->setContentsMargins(12, 6, 10, 6);
    row->setSpacing(6);
    row->addWidget(new QLabel(QStringLiteral("从旧版本迁移配置？"), frame));
    auto* cancel = new QPushButton(QStringLiteral("取消"), frame);
    cancel->setObjectName("dismiss");
    auto* accept = new QPushButton(QStringLiteral("确认"), frame);
    accept->setProperty("primary", true);
    row->addWidget(cancel);
    row->addWidget(accept);
    QObject::connect(cancel, &QPushButton::clicked, clip, &QWidget::close);
    QObject::connect(accept, &QPushButton::clicked, clip, [clip, confirmed] {
        clip->close();
        confirmed();
    });
    frame->adjustSize();
    clip->setGeometry(QRect(QPoint(15, top), frame->size()));
    auto* slide = new QPropertyAnimation(frame, "pos", clip);
    slide->setDuration(170);
    slide->setStartValue(QPoint(0, -frame->height()));
    slide->setEndValue(QPoint(0, 0));
    slide->setEasingCurve(QEasingCurve::OutCubic);
    clip->show();
    clip->raise();
    slide->start(QAbstractAnimation::DeleteWhenStopped);
    return clip;
}

HomePanelDialog::HomePanelDialog(QWidget* home, const QString& title, int width)
    : QDialog(home, Qt::Dialog | Qt::FramelessWindowHint)
{
    setWindowTitle(title);
    setWindowModality(Qt::WindowModal);
    setFixedSize(home->size());
    move(home->mapToGlobal(QPoint(0, 0)));
    auto* backdrop = new QLabel(this);
    backdrop->setGeometry(rect());
    backdrop->setPixmap(home->grab());
    auto* blur = new QGraphicsBlurEffect(backdrop);
    blur->setBlurRadius(6);
    backdrop->setGraphicsEffect(blur);
    auto* shade = new QWidget(this);
    shade->setGeometry(rect());
    shade->setStyleSheet("background: rgba(23,23,28,85);");
    auto* layout = new QVBoxLayout(this);
    auto* card = panel(this);
    card->setFixedWidth(width);
    layout->addWidget(card, 0, Qt::AlignCenter);
    m_title = heading(title, card, 17);
    form = panelBody(card, m_title);
}

std::optional<QString> requestHomeChannelCode(QWidget* home)
{
    HomePanelDialog dialog(home, QStringLiteral("管理员cdk"), 320);
    auto* input = new QLineEdit(&dialog);
    input->setAccessibleName(dialog.windowTitle());
    input->setEchoMode(QLineEdit::Password);
    dialog.form->addWidget(input);
    auto* buttons = new QDialogButtonBox(&dialog);
    buttons->addButton(QStringLiteral("取消"), QDialogButtonBox::RejectRole);
    auto* accept = buttons->addButton(QStringLiteral("确认"), QDialogButtonBox::AcceptRole);
    accept->setProperty("primary", true);
    accept->setDefault(true);
    dialog.form->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    input->setFocus();
    if (dialog.exec() == QDialog::Accepted)
        return input->text().trimmed();
    return std::nullopt;
}

void showHomeChannelError(QWidget* home, const QString& reason)
{
    HomePanelDialog dialog(home, QStringLiteral("管理员cdk"), 320);
    auto* message = new QLabel(reason, &dialog);
    message->setTextFormat(Qt::PlainText);
    message->setWordWrap(true);
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dialog.form->addWidget(message);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setProperty("primary", true);
    dialog.form->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    dialog.exec();
}

MigrationDialog::MigrationDialog(QWidget* home, QString target, Niijima::MigrationSource automatic, QString version, std::function<bool()> busy)
    : HomePanelDialog(home, QStringLiteral("选择迁移来源"), 372), m_target(std::move(target)), m_busy(std::move(busy))
{
    setAttribute(Qt::WA_DeleteOnClose);
    auto* card = form->parentWidget();
    m_versions = new QComboBox(card);
    m_versions->addItems(Niijima::legacyOptionsVersions());
    m_versions->setMaxVisibleItems(8);
    form->addWidget(m_versions);
    m_versions->hide();
    m_error = new QLabel(card);
    m_error->setStyleSheet("color: #903925;");
    form->addWidget(m_error);
    m_error->hide();
    auto* buttons = new QHBoxLayout;
    form->addLayout(buttons);
    auto* cancel = new QPushButton(QStringLiteral("取消"), card);
    cancel->setObjectName("dismiss");
    m_manualButton = new QPushButton(QStringLiteral("手动选择"), card);
    m_autoButton = new QPushButton(QStringLiteral("自动选择"), card);
    m_autoButton->setProperty("primary", true);
    m_autoButton->setEnabled(!automatic.packId.isEmpty());
    m_confirm = new QPushButton(QStringLiteral("确认"), card);
    m_confirm->setProperty("primary", true);
    buttons->addWidget(cancel);
    buttons->addStretch();
    buttons->addWidget(m_manualButton);
    buttons->addWidget(m_autoButton);
    m_autoVersion = new QLabel(card);
    m_autoVersion->setTextFormat(Qt::PlainText);
    m_autoVersion->setStyleSheet("font-size: 11px; color: #686563;");
    m_autoVersion->ensurePolished();
    m_autoVersion->setText(m_autoVersion->fontMetrics().elidedText(version, Qt::ElideMiddle, 88));
    m_autoVersion->setToolTip(version);
    m_autoVersion->setVisible(m_autoButton->isEnabled() && !version.isEmpty());
    buttons->addWidget(m_autoVersion, 0, Qt::AlignVCenter);
    buttons->addWidget(m_confirm);
    m_confirm->hide();
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_manualButton, &QPushButton::clicked, this, &MigrationDialog::selectFolder);
    connect(m_autoButton, &QPushButton::clicked, this, [this, automatic] { apply(automatic); });
    connect(m_confirm, &QPushButton::clicked, this, [this] { apply(m_manual, m_versions->currentText()); });
}

void MigrationDialog::selectFolder()
{
    const auto folder = QFileDialog::getExistingDirectory(this, QStringLiteral("选择旧版本文件夹"),
                                                          QFileInfo(m_target).absoluteDir().absolutePath());
    if (folder.isEmpty())
        return;
    m_manual = Niijima::migrationSource(folder);
    if (!m_manual.packId.isEmpty() || QFileInfo(m_manual.gameDir) == QFileInfo(m_target)) {
        apply(m_manual);
        return;
    }
    m_error->hide();
    m_title->setText(QStringLiteral("选择旧版本"));
    m_manualButton->hide();
    m_autoButton->hide();
    m_autoVersion->hide();
    m_versions->show();
    m_confirm->show();
    m_versions->setFocus();
}

void MigrationDialog::apply(const Niijima::MigrationSource& source, const QString& version)
{
    if (m_busy()) {
        qWarning() << "Settings migration: game is launching/running:" << m_target;
        m_error->setText(QStringLiteral("请先关闭游戏再迁移"));
    } else if (Niijima::migrateSettings(source, version, m_target)) {
        accept();
        return;
    } else {
        m_error->setText(QStringLiteral("无法读取或写入设置文件"));
    }
    m_error->show();
}
