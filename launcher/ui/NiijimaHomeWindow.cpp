// SPDX-License-Identifier: GPL-3.0-only
#include "niijima/PackError.h"
#include "NiijimaHomeWindow.h"
#include "dialogs/MigrationDialog.h"
#include <QFrame>
#include <QCloseEvent>
#include <QShowEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QTimer>
#include <QtMath>
#include <QVBoxLayout>
#include <QWindow>
#include "Application.h"
#include "MainWindow.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "LaunchController.h"
#include "launch/LaunchTask.h"
#include "MMCTime.h"
#include "StringUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "niijima/PackIndex.h"
#include "niijima/InstalledPacks.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"
#include "ui/widgets/NiijimaInstallStatus.h"
#include "ui/dialogs/MSALoginDialog.h"

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
constexpr int kWindowInset = 1;
constexpr int kTitleBarHeight = 46;
constexpr int kTitleBarBottom = kWindowInset + kTitleBarHeight;

#ifdef Q_OS_WIN
QString droppedPackFile(const QMimeData* data)
{
    const auto urls = data->urls();
    if (urls.size() != 1 || !urls.front().isLocalFile())
        return {};
    const auto path = urls.front().toLocalFile();
    return QFileInfo(path).isFile() ? path : QString();
}
#endif

enum class CaptionGlyph { Menu, Minimize, Close };

class CaptionButton final : public QPushButton {
public:
    CaptionButton(CaptionGlyph glyph, QWidget* parent) : QPushButton(parent), m_glyph(glyph)
    {
        setFixedSize(50, kTitleBarHeight);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPushButton::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(width() / 2.0, height() / 2.0);
        painter.setPen(QPen(Qt::white, 1.35, Qt::SolidLine, Qt::FlatCap));
        switch (m_glyph) {
            case CaptionGlyph::Menu:
                for (int y : { -4, 0, 4 })
                    painter.drawLine(QPointF(-6, y), QPointF(6, y));
                break;
            case CaptionGlyph::Minimize:
                painter.drawLine(QPointF(-5.5, 0), QPointF(5.5, 0));
                break;
            case CaptionGlyph::Close:
                painter.drawLine(QPointF(-5, -5), QPointF(5, 5));
                painter.drawLine(QPointF(-5, 5), QPointF(5, -5));
                break;
        }
    }

private:
    CaptionGlyph m_glyph;
};

// Measure the visible glyphs, so the font's trailing letter spacing does not add uneven padding.
class HomePrimaryButton : public QPushButton {
public:
    explicit HomePrimaryButton(QWidget* parent) : QPushButton(parent) { setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); }

    QSize sizeHint() const override
    {
        ensurePolished();
        const auto bounds = QFontMetricsF(font()).tightBoundingRect(QStringLiteral("启动游戏"));
        const qreal margin = QFontInfo(font()).pixelSize() * 0.6;
        return QSize(qCeil(bounds.width() + 2 * margin) + 2, qCeil(bounds.height() + 2 * margin) + 2);
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QStyleOptionButton option;
        initStyleOption(&option);
        const auto label = option.text;
        option.text.clear();
        QStylePainter painter(this);
        painter.drawControl(QStyle::CE_PushButton, option);
        const bool striped = isEnabled() && property("running").toBool();
        if (striped) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            const QRectF face = QRectF(rect()).adjusted(1, 1, -1, -1);
            QPainterPath clip;
            clip.addRoundedRect(face, 9, 9);
            painter.setClipPath(clip);
            const QColor yellow(isDown() ? "#cda51e" : underMouse() ? "#ffdb50" : "#f2c230");
            painter.fillRect(face, yellow);
            constexpr int stripeWidth = 16;
            for (int x = -height(); x < width() + height(); x += 2 * stripeWidth) {
                QPolygonF stripe;
                stripe << QPointF(x, height()) << QPointF(x + stripeWidth, height())
                       << QPointF(x + stripeWidth + height(), 0) << QPointF(x + height(), 0);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#242424"));
                painter.drawPolygon(stripe);
            }
            painter.restore();
        }
        auto labelFont = font();
        if (label.size() > 4) {
            const auto width = QFontMetricsF(labelFont).tightBoundingRect(QStringLiteral("启动游戏")).width();
            labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
            const auto plainWidth = QFontMetricsF(labelFont).tightBoundingRect(label).width();
            labelFont.setLetterSpacing(QFont::AbsoluteSpacing, (width - plainWidth) / (label.size() - 1));
        }
        painter.setFont(labelFont);
        painter.setPen(isEnabled() ? QColor(Qt::white) : QColor("#aab0ba"));
        const auto bounds = QFontMetricsF(labelFont).tightBoundingRect(label);
        const QPointF origin = QRectF(rect()).center() - bounds.center();
        if (striped) {
            QPainterPath lettering;
            lettering.addText(origin, labelFont, label);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.strokePath(lettering, QPen(QColor("#242424"), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }
        painter.drawText(origin, label);
    }
};

class HomeWordmark : public QWidget {
public:
    explicit HomeWordmark(QWidget* parent) : QWidget(parent), m_icon(QStringLiteral(":/niijima/home-title.svg")) {}

protected:
    void paintEvent(QPaintEvent*) override
    {
        // Paint in logical coordinates; QIcon handles the current screen's DPR and SVG aspect ratio.
        QPainter painter(this);
        m_icon.paint(&painter, rect(), Qt::AlignLeft | Qt::AlignVCenter);
    }

private:
    const QIcon m_icon;
};

QPixmap createFrostedBackground()
{
    const QPixmap source(":/niijima/home-background.png");
    if (source.isNull()) {
        qWarning("Could not load the Niijima home background");
        return {};
    }

    // Blur only the artwork. Widgets remain sharp above the painted glass layers.
    QGraphicsScene scene;
    auto* artwork = scene.addPixmap(source);
    auto* blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(18);
    blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
    artwork->setGraphicsEffect(blur);
    scene.setSceneRect(source.rect());
    QPixmap result(source.size());
    result.fill(Qt::transparent);
    QPainter painter(&result);
    scene.render(&painter, QRectF(result.rect()), scene.sceneRect());
    return result;
}
}  // namespace

NiijimaHomeWindow::NiijimaHomeWindow(QWidget* parent) : QWidget(parent)
{
    static const int titleFont = QFontDatabase::addApplicationFont(":/niijima/ZenMaruGothic-Bold.ttf");
    Q_UNUSED(titleFont);
    static const int regularTitleFont = QFontDatabase::addApplicationFont(":/niijima/ZenMaruGothic-Regular.ttf");
    if (regularTitleFont < 0)
        qWarning("Could not load the bundled Zen Maru Gothic Regular font");
    static const int chineseFont = QFontDatabase::addApplicationFont(":/niijima/NotoSansSC-VariableFont_wght.ttf");
    if (chineseFont < 0)
        qWarning("Could not load the bundled Noto Sans SC font");
    m_channel = APPLICATION->settings()->get("NiijimaPackChannel").toString();
    if (m_channel.isEmpty())
        m_channel = Niijima::DefaultPackChannel;
    setObjectName("niijimaHome");
#ifdef Q_OS_WIN
    setAcceptDrops(true);
#endif
    const auto launcherTitle = QStringLiteral("新岛启动器 v1.0");
    setWindowTitle(launcherTitle);
    setWindowIcon(APPLICATION->logo());
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    // Shared implicit pixmap data: prepare the blur once, not on repaint or resize.
    static const QPixmap background = createFrostedBackground();
    m_frostedBackground = background;
    resize(960, 600);
    setMinimumSize(780, 510);
    setStyleSheet(QStringLiteral(R"(
        #niijimaHome { color: #452800; }
        QWidget { font-family: 'Noto Sans SC', 'Microsoft YaHei UI'; }
        QWidget[niijimaLanguage="ja"] { font-family: 'Zen Maru Gothic'; }
        QLabel { background: transparent; color: #452800; }
        QLabel#homeTitle { color: #ffffff; }
        QPushButton { border: 1px solid #d6a65d; border-radius: 10px; padding: 12px 20px; background: #fff5e3; color: #543000; font-size: 14px; }
        QPushButton:hover { background: #ffe3b0; border-color: #b46d06; }
        QPushButton:pressed { background: #ffd082; }
        QPushButton:disabled { color: #9d8e78; border-color: #d7c6ae; background: #eee0cc; }
        QPushButton#homeMenu, QPushButton#minimize, QPushButton#close { background: transparent; border: 0; border-radius: 0; font-size: 20px; padding: 0; color: #ffffff; }
        /* The titlebar is inset 1px from the window's 14px corner. */
        QPushButton#close { border-top-right-radius: 13px; }
        QPushButton#homeMenu:hover, QPushButton#minimize:hover { background: rgba(255, 255, 255, 35); }
        QPushButton#homeMenu:pressed, QPushButton#minimize:pressed { background: rgba(255, 255, 255, 60); }
        QPushButton#close:hover { background: #c42b1c; color: white; }
        QMenu#channelMenu { background: #eee5d8; color: #322b23; border: 1px solid #e3d6c3; border-top: 3px solid #f39801; padding: 4px; }
        QMenu#channelMenu::item { padding: 7px 18px; border-radius: 4px; }
        QMenu#channelMenu::item:selected { background: #fff1d8; color: #48321a; }
        QMenu#channelMenu::item:disabled { color: #a49889; }
        QMessageBox { background: #eee5d8; }
        QMessageBox QPushButton:default { background: #f39801; color: #352c22; }
        QProgressBar { border: 0; background: #f9dca8; color: #452800; text-align: center; font-size: 12px; }
        QProgressBar::chunk { background: #ecb354; }
        QWidget#homeBody QLabel { color: #fff9ee; }
        QWidget#homeBody QPushButton { background: rgba(30, 36, 47, 155); border-color: rgba(255, 249, 238, 80); color: #fff9ee; }
        QWidget#homeBody QPushButton:hover { background: rgba(65, 73, 86, 190); border-color: rgba(255, 249, 238, 145); }
        QWidget#homeBody QPushButton:pressed { background: rgba(19, 26, 37, 210); }
        QWidget#homeBody QPushButton#cancel { padding: 0 16px; border-radius: 8px; }
        QWidget#homeBody QPushButton#primary { background: #f39801; border-color: #ffbd52; color: #ffffff; font-size: 36px; font-weight: 700; letter-spacing: 11px; padding: 0; }
        QWidget#homeBody QPushButton#primary:hover { background: #ffad28; }
        QWidget#homeBody QPushButton#primary:pressed { background: #de8600; }
        QWidget#homeBody QPushButton#prism { background: rgba(84, 75, 70, 205); border-color: rgba(194, 181, 166, 130); border-radius: 8px; font-size: 13px; padding: 8px 14px; }
        QWidget#homeBody QPushButton#prism:hover { background: rgba(105, 94, 86, 230); border-color: rgba(225, 213, 198, 170); }
        QWidget#homeBody QPushButton#prism:pressed { background: rgba(66, 59, 53, 235); }
        QWidget#homeBody QPushButton:disabled, QWidget#homeBody QPushButton#primary:disabled { color: #aab0ba; border-color: rgba(255, 255, 255, 35); background: rgba(37, 43, 53, 145); }
    )"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(kWindowInset, kWindowInset, kWindowInset, kWindowInset);
    root->setSpacing(0);
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(kTitleBarHeight);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(24, 0, 0, 0);
    titleLayout->setSpacing(0);
    auto* title = new QLabel(launcherTitle, titleBar);
    QFont heading("Noto Sans SC", 14, QFont::Bold);
    heading.setFamilies({ "Noto Sans SC", "Microsoft YaHei UI" });
    title->setObjectName("homeTitle");
    title->setFont(heading);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    auto* menuButton = new CaptionButton(CaptionGlyph::Menu, titleBar);
    menuButton->setObjectName("homeMenu");
    menuButton->setAccessibleName(QStringLiteral("菜单"));
    titleLayout->addWidget(menuButton);
    auto* menu = new QMenu(menuButton);
    menu->setObjectName("channelMenu");
    auto* migrate = menu->addAction(QStringLiteral("迁移配置"), this, [this] {
        if (m_migrationOffer)
            m_migrationOffer->close();
        openMigration(m_selectedInstance);
    });
    auto* channelAction = menu->addAction(QStringLiteral("管理员cdk"), this, [this] {
        if (m_channel == Niijima::DefaultPackChannel)
            enterChannelCode();
        else
            setPackChannel(Niijima::DefaultPackChannel);
    });
    connect(menuButton, &QPushButton::clicked, this, [this, menuButton, menu, migrate, channelAction] {
        selectLatestInstance();
        const auto* instance = APPLICATION->instances()->getInstanceById(m_selectedInstance);
        const bool idle = !m_task || !m_task->isRunning();
        migrate->setEnabled(idle && instance && !Niijima::migrationFiles(instance->gameRoot()).isEmpty());
        channelAction->setEnabled(idle);
        channelAction->setText(m_channel == Niijima::DefaultPackChannel ? QStringLiteral("管理员cdk") : QStringLiteral("重置渠道"));
        menu->popup(menuButton->mapToGlobal(QPoint(menuButton->width() - menu->sizeHint().width(), menuButton->height())));
    });
    auto* minimize = new CaptionButton(CaptionGlyph::Minimize, titleBar);
    minimize->setObjectName("minimize");
    minimize->setAccessibleName(QStringLiteral("最小化"));
    connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minimize);
    auto* close = new CaptionButton(CaptionGlyph::Close, titleBar);
    close->setObjectName("close");
    close->setAccessibleName(QStringLiteral("关闭"));
    connect(close, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(close);
    root->addWidget(titleBar);
    m_progress = new QProgressBar(this);
    m_progress->setFixedHeight(26);
    m_progress->hide();
    root->addWidget(m_progress);
    auto* body = new QWidget(this);
    body->setObjectName("homeBody");
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(42, 32, 42, 34);
    layout->setSpacing(12);
    layout->addStretch(2);
    auto* hero = new QVBoxLayout();
    hero->setSpacing(18);
    auto* badge = new QLabel(QStringLiteral("NIIJIMA RAILWAYS"), body);
    badge->setStyleSheet("font-size: 12px; letter-spacing: 3px; color: #f5dec2;");
    hero->addWidget(badge, 0, Qt::AlignLeft);
    auto* wordmark = new HomeWordmark(body);
    wordmark->setObjectName("homeWordmark");
    wordmark->setAccessibleName(QStringLiteral("新島鉄道"));
    wordmark->setFixedSize(444, 110);
    hero->addWidget(wordmark, 0, Qt::AlignLeft);
    auto* slogan = new QLabel(QStringLiteral("<span style='font-size:18px'>==</span>しゅっぱつしんこう！！"
                                            "<span style='font-size:18px'>==</span>"), body);
    slogan->setTextFormat(Qt::RichText);
    slogan->setAccessibleName(QStringLiteral("==しゅっぱつしんこう！！=="));
    slogan->setProperty("niijimaLanguage", "ja");
    slogan->setStyleSheet("font-family: 'Zen Maru Gothic'; font-size: 24px; font-weight: 400; color: #d0d0d0;");
    hero->addWidget(slogan, 0, Qt::AlignLeft);
    layout->addLayout(hero);
    layout->addStretch(4);
    m_status = new QLabel(body);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    m_status->setStyleSheet("font-size: 14px;");
    layout->addWidget(m_status);
    m_status->hide();
    auto* actions = new QHBoxLayout();
    actions->setSpacing(12);
    auto* prism = new QPushButton(APPLICATION->prismLogo(), QStringLiteral("进入 Prism"), body);
    prism->setIconSize(QSize(20, 20));
    prism->setObjectName("prism");
    connect(prism, &QPushButton::clicked, this, [] { APPLICATION->showMainWindow(); });
    actions->addWidget(prism, 0, Qt::AlignBottom);
    actions->addStretch();
    m_packVersion = new QLabel(body);
    m_packVersion->setObjectName("packVersion");
    m_packVersion->setTextFormat(Qt::PlainText);
    m_packVersion->setStyleSheet("font-size: 14px; color: #aaaaaa;");
    m_packVersion->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_packVersion->hide();

    m_cancel = new QPushButton(QStringLiteral("取消"), body);
    m_cancel->setObjectName("cancel");
    m_cancel->setFixedHeight(32);
    auto cancelPolicy = m_cancel->sizePolicy();
    cancelPolicy.setRetainSizeWhenHidden(true);
    m_cancel->setSizePolicy(cancelPolicy);
    m_cancel->hide();
    connect(m_cancel, &QPushButton::clicked, this, [this] {
        if (m_task)
            m_task->abort();
    });
    // Share one column so showing Cancel never pushes the version label sideways.
    auto* secondary = new QWidget(body);
    auto* secondaryActions = new QVBoxLayout(secondary);
    secondaryActions->setContentsMargins(0, 0, 0, 0);
    secondaryActions->setSpacing(0);
    secondaryActions->addWidget(m_cancel, 0, Qt::AlignRight | Qt::AlignTop);
    secondaryActions->addStretch();
    secondaryActions->addWidget(m_packVersion, 0, Qt::AlignRight | Qt::AlignBottom);
    actions->addWidget(secondary, 0, Qt::AlignBottom);
    m_primary = new HomePrimaryButton(body);
    m_primary->setObjectName("primary");
    secondary->setFixedHeight(m_primary->sizeHint().height());
    connect(m_primary, &QPushButton::clicked, this, &NiijimaHomeWindow::primaryAction);
    actions->addWidget(m_primary, 0, Qt::AlignBottom);
    m_installStatus = new NiijimaInstallStatus(m_primary);
    layout->addLayout(actions);
    root->addWidget(body, 1);
    connect(APPLICATION->instances(), &InstanceList::instancesChanged, this, &NiijimaHomeWindow::refresh);
    connect(APPLICATION, &Application::instanceLaunchChanged, this, &NiijimaHomeWindow::refreshPrimaryAction);
    connect(APPLICATION->instances(), &InstanceList::instanceCommitted, this, [this](const InstanceTask* task, const QString& id) {
        if (task == m_import) {
            m_committedId = id;
        }
    });
    refresh();
}

namespace {
QUrl packIndexUrl()
{
    const auto setting = APPLICATION->settings()->getSetting("NiijimaPackIndexUrl");
    auto value = setting->get().toString().trimmed();
    return QUrl(value.isEmpty() ? setting->defValue().toString() : value);
}
} // namespace

void NiijimaHomeWindow::setPackChannel(const QString& channel)
{
    if (m_migrationOffer)
        m_migrationOffer->close();
    m_channel = channel;
    APPLICATION->settings()->set("NiijimaPackChannel", channel);
    m_update.release.reset();
    refresh();
    checkForUpdates();
}

void NiijimaHomeWindow::enterChannelCode()
{
    const auto code = requestHomeChannelCode(this);
    if (!code || code->isEmpty())
        return;
    cancelUpdateCheck();
    auto* task = new Niijima::PackIndexTask(packIndexUrl(), {}, this);
    task->lookupCode(*code);
    m_task = task;
    m_primary->setEnabled(false);
    connect(task, &Task::finished, this, [this, task] {
        m_task = nullptr;
        if (task->wasSuccessful())
            setPackChannel(task->channel());
        else {
            refresh();
            showHomeChannelError(this, task->failReason());
        }
        task->deleteLater();
    });
    task->start();
}

void NiijimaHomeWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!event->spontaneous())
        checkForUpdates();
}

void NiijimaHomeWindow::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && isActiveWindow() && isVisible() && !m_checkTask)
        checkForUpdates();
}

void NiijimaHomeWindow::cancelUpdateCheck()
{
    // Invalidate callbacks before abort(), which can emit finished synchronously.
    ++m_update.generation;
    m_update.loading = false;
    if (m_checkTask) {
        auto* task = m_checkTask.data();
        m_checkTask = nullptr;
        task->abort();
    }
}

void NiijimaHomeWindow::checkForUpdates(const QString& launchId)
{
    if (m_task && m_task->isRunning())
        return;
    const auto previousId = updateAvailable() ? m_update.release->id : 0;
    cancelUpdateCheck();
    const auto generation = m_update.begin();
    refreshPrimaryAction();
    const auto continueLaunch = [this, generation, launchId] {
        if (launchId.isEmpty() || generation != m_update.generation)
            return;
        // Invalidate the other completion path before login can open a nested event loop.
        cancelUpdateCheck();
        refreshPrimaryAction();
        if (isVisible() && m_selectedInstance == launchId && !updateAvailable()) {
            auto* inst = APPLICATION->instances()->getInstanceById(launchId);
            if (inst && inst->canLaunch())
                launchInstance(inst);
        }
    };
    QTimer::singleShot(1000, Qt::PreciseTimer, this, [this, generation, continueLaunch] {
        if (generation != m_update.generation)
            return;
        m_update.timeout(generation);
        refreshPrimaryAction();
        continueLaunch();
    });
    const auto url = packIndexUrl();
    auto* task = new Niijima::PackIndexTask(url, APPLICATION->settings()->get("NiijimaPackVersionId").toString(), this, m_channel);
    m_checkTask = task;
    connect(task, &Task::finished, this, [this, task, generation, continueLaunch, previousId] {
        if (m_checkTask == task)
            m_checkTask = nullptr;
        if (generation == m_update.generation) {
            if (task->wasSuccessful()) {
                m_update.complete(generation, task->release());
                if (updateAvailable() && task->release().id != previousId)
                    m_installStatus->append(QStringLiteral("整合包有更新：v%1").arg(Niijima::channelVersion(task->release().version, m_channel)));
            } else {
                // No popup or status copy for a background check. The one-second gate still expires.
                qWarning() << "Niijima pack check failed:" << task->failReason();
            }
            refreshPrimaryAction();
            continueLaunch();
        }
        task->deleteLater();
    });
    task->start();
}

bool NiijimaHomeWindow::updateAvailable() const
{
    return !m_selectedInstance.isEmpty() && m_update.release &&
           Niijima::needsPackUpdate(*m_update.release, m_selectedPackId);
}

Result<> NiijimaHomeWindow::savePackId(quint64 packId)
{
    const auto* instance = APPLICATION->instances()->getInstanceById(m_committedId);
    if (!instance)
        return std::unexpected(Niijima::packError(3001, QStringLiteral("Cannot record pack-id for missing instance: %1").arg(m_committedId)));
    return Niijima::verifyInstalledPackId(instance->instanceRoot(), instance->gameRoot(), packId, m_channel);
}

void NiijimaHomeWindow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);
    p.setClipPath(clip);
    const QRectF body(0, kTitleBarBottom, width(), height() - kTitleBarBottom);
    p.fillRect(rect(), QColor("#26313f"));
    if (!m_frostedBackground.isNull()) {
        // Center-crop to cover the body while preserving the artwork's aspect ratio.
        const qreal scale = qMax(body.width() / m_frostedBackground.width(), body.height() / m_frostedBackground.height());
        const QSizeF visible(body.width() / scale, body.height() / scale);
        const QRectF source((m_frostedBackground.width() - visible.width()) / 2, (m_frostedBackground.height() - visible.height()) / 2,
                            visible.width(), visible.height());
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawPixmap(body, m_frostedBackground, source);
    }

    // Uniform blur only: no tint or regional contrast masks over the background.
    p.fillRect(QRect(0, 0, width(), kTitleBarBottom), QColor("#f39801"));
    p.setClipping(false);
    p.setPen(QPen(QColor("#d39b45"), 1));
    p.drawPath(clip);
}
#ifdef Q_OS_WIN
void NiijimaHomeWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!droppedPackFile(event->mimeData()).isEmpty() && event->possibleActions().testFlag(Qt::CopyAction)) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void NiijimaHomeWindow::dropEvent(QDropEvent* event)
{
    const auto path = droppedPackFile(event->mimeData());
    if (path.isEmpty() || !event->possibleActions().testFlag(Qt::CopyAction))
        return;
    event->setDropAction(Qt::CopyAction);
    event->accept();
    // Return to Explorer before opening Prism's modal import dialog.
    QTimer::singleShot(0, this, [path] { APPLICATION->showMainWindow()->addInstance(path); });
}
#endif

void NiijimaHomeWindow::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && e->position().y() < kTitleBarBottom && windowHandle())
        windowHandle()->startSystemMove();
    QWidget::mousePressEvent(e);
}
void NiijimaHomeWindow::closeEvent(QCloseEvent* e)
{
    if (m_import && m_task && m_task->isRunning()) {
        e->ignore();
        if (m_closeAfterSkip || m_import->discarded())
            return;
        if (m_import->reachedGameFilesDownload()) {
            // abort() may finish synchronously; set the intent before using the ordinary Skip action.
            m_closeAfterSkip = true;
            setEnabled(false);
            if (m_import->isRunning())
                m_task->abort();
            return;
        }
        if (isHidden()) {
            abortInstallAndClose();
            return;
        }
        if (m_installExitPrompt)
            return;
        auto* prompt = makeInstallExitPrompt(this);
        m_installExitPrompt = prompt;
        prompt->setAttribute(Qt::WA_DeleteOnClose);
        connect(prompt, &QDialog::finished, this, [this](int result) {
            if (result == QMessageBox::Yes)
                abortInstallAndClose();
        });
        prompt->open();
        return;
    }
    if (auto* lookup = qobject_cast<Niijima::PackIndexTask*>(m_task.data())) {
        // A cancelled lookup must neither switch channels nor start installing.
        disconnect(lookup, nullptr, this, nullptr);
        m_task = nullptr;
        lookup->abort();
    }
    cancelUpdateCheck();
    e->accept();
    emit isClosing();
}
void NiijimaHomeWindow::abortInstallAndClose()
{
    cancelUpdateCheck();
    auto* task = m_task.data();
    if (!m_import || !task || !task->isRunning()) {
        close();
        return;
    }
    if (m_import->discarded())
        return;
    setEnabled(false);
    // Let the active worker release its files before destroying the home window.
    disconnect(task, nullptr, this, nullptr);
    connect(task, &Task::finished, this, [this] {
        m_task = nullptr;
        close();
    }, Qt::QueuedConnection);
    m_import->discard();
}

static Niijima::InstalledPackSelection installedSourceExcept(const QString& excluded, const QString& channel)
{
    const auto* instances = APPLICATION->instances();
    QList<Niijima::InstalledPackCandidate> candidates;
    for (int i = 0; i < instances->count(); ++i) {
        const auto* instance = instances->at(i);
        if (instance->id() == excluded)
            continue;
        candidates.append({instance->id(), instance->instanceRoot(), instance->gameRoot()});
    }
    return Niijima::selectLatestInstalledPack(candidates, channel);
}

void NiijimaHomeWindow::selectLatestInstance()
{
    const auto selected = installedSourceExcept({}, m_channel);
    m_selectedInstance = selected.instanceId;
    m_selectedPackId = selected.packId;
}

void NiijimaHomeWindow::offerMigration()
{
    auto* target = APPLICATION->instances()->getInstanceById(m_committedId);
    if (!target || Niijima::migrationFiles(target->gameRoot()).isEmpty())
        return;
    const auto targetId = target->id();
    m_migrationOffer = migrationOffer(this, kTitleBarBottom, [this, targetId] { openMigration(targetId); });
}

void NiijimaHomeWindow::openMigration(const QString& targetId)
{
    auto* instances = APPLICATION->instances();
    auto* target = instances->getInstanceById(targetId);
    if (!target)
        return;
    const auto selection = installedSourceExcept(targetId, m_channel);
    Niijima::MigrationSource automatic;
    QString version;
    if (auto* source = instances->getInstanceById(selection.instanceId)) {
        automatic = {source->gameRoot(), QString::number(selection.packId), m_channel};
        version = Niijima::installedPackVersion(source->instanceRoot());
        if (version.isEmpty())
            version = source->name();
    }
    auto* dialog = new MigrationDialog(this, target->gameRoot(), automatic, Niijima::channelVersion(version, m_channel), [targetId] {
        auto* instance = APPLICATION->instances()->getInstanceById(targetId);
        auto* launch = APPLICATION->launchController(targetId);
        return (instance && instance->isRunning()) || (launch && !launch->isFinished());
    });
    connect(dialog, &QDialog::accepted, this, [this] { m_installStatus->transient(QStringLiteral("配置已迁移")); });
    dialog->open();
}

void NiijimaHomeWindow::refresh()
{
    if (m_task && m_task->isRunning())
        return;
    refreshPrimaryAction();
    m_installStatus->clear();
    m_cancel->hide();
    m_progress->hide();
    m_status->clear();
    m_status->hide();
}
void NiijimaHomeWindow::refreshPrimaryAction()
{
    if (m_task && m_task->isRunning())
        return;
    selectLatestInstance();
    refreshPackVersion();
    auto* instance = APPLICATION->instances()->getInstanceById(m_selectedInstance);
    bool launching = false;
    bool running = false;
    if (instance) {
        connect(instance, &BaseInstance::launchTaskChanged, this, &NiijimaHomeWindow::refreshPrimaryAction, Qt::UniqueConnection);
        auto* task = instance->getLaunchTask();
        if (task)
            connect(task, &LaunchTask::readyForLaunch, this, &NiijimaHomeWindow::refreshPrimaryAction, Qt::UniqueConnection);
        auto* controller = APPLICATION->launchController(instance->id());
        launching = m_loginInstance == instance->id() || (controller && !controller->isFinished());
        running = instance->isRunning() && task && task->pid() > 0;
    }
    const bool update = updateAvailable();
    m_primary->setText(m_update.loading ? QStringLiteral("加载中")
                                       : update ? QStringLiteral("更新游戏")
                                                : running ? QStringLiteral("打开控制台")
                                                          : launching ? QStringLiteral("正在启动")
                                                                      : instance ? QStringLiteral("启动游戏") : QStringLiteral("下载游戏"));
    m_primary->setEnabled(!m_update.loading && (update || !launching || running));
    const bool console = running && !update && !m_update.loading;
    if (m_primary->property("running").toBool() != console) {
        m_primary->setProperty("running", console);
        m_primary->style()->unpolish(m_primary);
        m_primary->style()->polish(m_primary);
        m_primary->update();
    }
}

void NiijimaHomeWindow::watchGameWindow(MinecraftInstance* instance)
{
#ifdef Q_OS_WIN
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, instance = QPointer<MinecraftInstance>(instance)] {
        if (!instance || !APPLICATION->launchController(instance->id())) {
            timer->deleteLater();
            return;
        }
        auto* task = instance->getLaunchTask();
        if (!instance->isRunning() || !task || task->pid() <= 0)
            return;
        struct WindowSearch { DWORD pid; HWND window = nullptr; } search{static_cast<DWORD>(task->pid())};
        EnumWindows([](HWND window, LPARAM parameter) -> BOOL {
            auto& target = *reinterpret_cast<WindowSearch*>(parameter);
            DWORD pid = 0;
            GetWindowThreadProcessId(window, &pid);
            if (pid != target.pid || !IsWindowVisible(window) || GetWindow(window, GW_OWNER) ||
                (GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
                return TRUE;
            target.window = window;
            return FALSE;
        }, reinterpret_cast<LPARAM>(&search));
        if (!search.window)
            return;
        AllowSetForegroundWindow(search.pid);
        SetForegroundWindow(search.window);
        if (isVisible())
            showMinimized();
        timer->deleteLater();
    });
    timer->start(500);
#else
    Q_UNUSED(instance);
#endif
}

void NiijimaHomeWindow::refreshPackVersion()
{
    auto* instance = APPLICATION->instances()->getInstanceById(m_selectedInstance);
    if (!instance) {
        m_packVersion->clear();
        m_packVersion->hide();
        return;
    }
    // Direct ZIP imports preserve their versionId here even without managed-project metadata.
    QString version = Niijima::installedPackVersion(instance->instanceRoot());
    if (version.isEmpty())
        version = instance->getManagedPackVersionName().trimmed();
    version = Niijima::channelVersion(version, m_channel);
    if (version.isEmpty()) {
        version = QStringLiteral("读取错误");
    } else if (!version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        version.prepend(QLatin1Char('v'));
    }
    m_packVersion->setText(QStringLiteral("整合包版本：%1").arg(version));
    m_packVersion->show();
}

void NiijimaHomeWindow::showDownloadError(const QString& message)
{
    refresh();
    m_status->setText(message);
    m_status->show();
}

void NiijimaHomeWindow::launchInstance(MinecraftInstance* inst)
{
    MinecraftAccountPtr account;
    const bool starting = inst->canLaunch();
    if (starting) {
        auto* accounts = APPLICATION->accounts();
        bool hasMicrosoftAccount = false;
        for (int i = 0; i < accounts->count(); ++i)
            hasMicrosoftAccount |= accounts->at(i)->accountType() == AccountType::MSA;
        if (!hasMicrosoftAccount) {
            m_loginInstance = inst->id();
            refreshPrimaryAction();
            account = MSALoginDialog::newAccount(this);
            m_loginInstance.clear();
            refreshPrimaryAction();
            if (!account)
                return;
            accounts->addAccount(account);
            if (accounts->count() == 1)
                accounts->setDefaultAccount(account);
        }
    }
    if (APPLICATION->launch(inst, LaunchMode::Normal, nullptr, account) && starting)
        watchGameWindow(inst);
}

void NiijimaHomeWindow::primaryAction()
{
    if ((m_task && m_task->isRunning()) || m_update.loading)
        return;
    selectLatestInstance();
    refreshPackVersion();
    if (updateAvailable()) {
        startImport(m_update.release->packUrl);
        return;
    }
    auto* settings = APPLICATION->settings();
    const auto id = m_selectedInstance;
    if (auto* inst = APPLICATION->instances()->getInstanceById(id)) {
        if (inst->canLaunch())
            checkForUpdates(id);
        else
            launchInstance(inst);
        return;
    }
    if (m_update.release) {
        startImport(m_update.release->packUrl);
        return;
    }
    startReleaseLookup(packIndexUrl(), settings->get("NiijimaPackVersionId").toString().trimmed());
}

void NiijimaHomeWindow::startReleaseLookup(const QUrl& indexUrl, const QString& requestedId)
{
    cancelUpdateCheck();
    auto* task = new Niijima::PackIndexTask(indexUrl, requestedId, this, m_channel);
    m_task = task;
    m_primary->setEnabled(false);
    m_primary->setText(QStringLiteral("加载中"));
    m_status->clear();
    m_status->hide();
    m_progress->setRange(0, 0);
    m_progress->show();
    m_cancel->setText(QStringLiteral("取消"));
    m_cancel->show();
    connect(task, &Task::abortStatusChanged, m_cancel, &QWidget::setEnabled);
    connect(task, &Task::finished, this, [this, task] {
        const bool success = task->wasSuccessful();
        const bool cancelled = task->getState() == Task::State::AbortedByUser;
        const auto reason = task->failReason();
        const auto packUrl = task->release().packUrl;
        m_task = nullptr;
        task->deleteLater();
        if (success) {
            m_update.complete(m_update.generation, task->release());
            startImport(packUrl);
        } else if (cancelled) {
            refresh();
            m_status->setText(QStringLiteral("已取消"));
            m_status->show();
        } else {
            showDownloadError(reason);
        }
    });
    task->start();
    m_cancel->setEnabled(task->canAbort());
}

void NiijimaHomeWindow::startImport(const QUrl& url)
{
    if (m_task && m_task->isRunning())
        return;
    cancelUpdateCheck();
    const auto release = m_update.release && m_update.release->packUrl == url ? m_update.release : std::nullopt;
    // A configurable URL is not automatically a trusted distribution source.
    auto* importer = new InstanceImportTask(url, false, this);
    importer->setCreateNewInstance(true);
    importer->setNiijimaRelease(true);
    importer->setIcon(QStringLiteral("default"));
    importer->setGroup(QStringLiteral("新岛铁道"));
    m_import = importer;
    if (m_migrationOffer)
        m_migrationOffer->close();
    m_committedId.clear();
    auto* task = APPLICATION->instances()->wrapInstanceTask(importer);
    m_task = task;
    task->setParent(this);
    m_primary->setEnabled(false);
    m_primary->setText(QStringLiteral("正在安装"));
    m_transfers.clear();
    m_progress->setRange(0, 1000);
    m_progress->setValue(url.isLocalFile() ? 600 : 0);
    m_progress->setFormat(QStringLiteral("%p%"));
    m_progress->show();
    m_cancel->setText(QStringLiteral("取消"));
    m_cancel->show();
    m_status->hide();
    m_installStatus->append(QStringLiteral("正在下载整合包…"));
    connect(task, &Task::abortStatusChanged, m_cancel, &QWidget::setEnabled);
    connect(task, &Task::abortButtonTextChanged, m_cancel, &QPushButton::setText);
    connect(task, &Task::status, m_installStatus, &NiijimaInstallStatus::append);
    connect(importer, &Task::progressFraction, this, [this](double fraction) {
        m_progress->setValue(qMax(m_progress->value(), qMin(999, int(1000 * fraction))));
    });
    connect(importer, &InstanceImportTask::extractingPack, this, [this] {
        m_transfers.clear();
        m_progress->setValue(600);
        m_progress->setFormat(QStringLiteral("解压中 0/0"));
    });
    connect(importer, &InstanceImportTask::extractionProgress, this, [this](qint64 current, qint64 total) {
        m_progress->setFormat(QStringLiteral("解压中 %1/%2").arg(current).arg(total));
    });
    connect(importer, &InstanceImportTask::extractedPack, this, [this] { m_progress->setFormat(QStringLiteral("%p%")); });
    connect(task, &Task::stepProgress, this, &NiijimaHomeWindow::updateTransferText);
    connect(task, &Task::finished, this, [this, task, release] {
        if (m_installExitPrompt)
            m_installExitPrompt->reject();
        const bool success = task->wasSuccessful();
        const bool cancelled = task->getState() == Task::State::AbortedByUser;
        const auto reason = task->failReason();
        if (success)
            m_progress->setValue(1000);
        m_transfers.clear();
        m_task = nullptr;
        m_import = nullptr;
        const bool closeAfterSkip = m_closeAfterSkip;
        if (closeAfterSkip) {
            m_closeAfterSkip = false;
            setEnabled(true);
        }
        if (success && release) {
            if (const auto saved = savePackId(release->id); !saved) {
                showDownloadError(saved.error());
                task->deleteLater();
                return;
            }
        }
        if (closeAfterSkip && success) {
            task->deleteLater();
            QTimer::singleShot(0, this, &QWidget::close);
            return;
        }
        refresh();
        if (success)
            offerMigration();
        if (!success) {
            m_status->setText(cancelled ? QStringLiteral("已取消安装") : reason);
            if (!cancelled)
                QMessageBox::warning(this, QStringLiteral("安装失败"), reason);
            m_status->show();
        }
        task->deleteLater();
    });
    task->start();
    m_cancel->setEnabled(task->canAbort());
}

void NiijimaHomeWindow::updateTransferText(const TaskStepProgress& progress)
{
    if (progress.isDownload)
        m_transfers.insert(progress.uid, progress);
    else if (!progress.isDone() || !m_transfers.remove(progress.uid))
        return;

    if (m_transfers.isEmpty()) {
        m_progress->setFormat(QStringLiteral("%p%"));
        return;
    }
    double speed = 0;
    double remaining = 0;
    bool known = true;
    for (const auto& transfer : m_transfers) {
        speed += transfer.bytesPerSecond;
        known &= transfer.total > 0;
        remaining += qMax(qint64(0), transfer.total - transfer.current);
    }
    const auto eta = known && speed > 0 ? Time::humanReadableDuration(remaining / speed) : QStringLiteral("--");
    m_progress->setFormat(QStringLiteral("%1/s · ETA %2").arg(StringUtils::humanReadableFileSize(speed), eta));
}
