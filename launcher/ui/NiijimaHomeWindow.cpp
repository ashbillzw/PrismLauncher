// SPDX-License-Identifier: GPL-3.0-only
#include "NiijimaHomeWindow.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QDir>
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
#include "ui/MainWindow.h"
#include "ui/dialogs/MSALoginDialog.h"

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
constexpr int kTitleBarHeight = 46;

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
    setObjectName("niijimaHome");
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
        QPushButton#minimize, QPushButton#close { background: transparent; border: 0; border-radius: 0; font-size: 20px; padding: 0; color: #ffffff; }
        QPushButton#minimize:hover { background: rgba(255, 255, 255, 35); }
        QPushButton#minimize:pressed { background: rgba(255, 255, 255, 60); }
        QPushButton#close:hover { background: #c42b1c; color: white; }
        QProgressBar { border: 0; background: #f9dca8; color: #452800; text-align: center; font-size: 12px; }
        QProgressBar::chunk { background: #ecb354; }
        QWidget#homeBody QLabel { color: #fff9ee; }
        QWidget#homeBody QPushButton { background: rgba(30, 36, 47, 155); border-color: rgba(255, 249, 238, 80); color: #fff9ee; }
        QWidget#homeBody QPushButton:hover { background: rgba(65, 73, 86, 190); border-color: rgba(255, 249, 238, 145); }
        QWidget#homeBody QPushButton:pressed { background: rgba(19, 26, 37, 210); }
        QWidget#homeBody QPushButton#primary { background: #f39801; border-color: #ffbd52; color: #ffffff; font-size: 36px; font-weight: 700; letter-spacing: 11px; padding: 0; }
        QWidget#homeBody QPushButton#primary:hover { background: #ffad28; }
        QWidget#homeBody QPushButton#primary:pressed { background: #de8600; }
        QWidget#homeBody QPushButton#prism { background: rgba(84, 75, 70, 205); border-color: rgba(194, 181, 166, 130); border-radius: 8px; font-size: 13px; padding: 8px 14px; }
        QWidget#homeBody QPushButton#prism:hover { background: rgba(105, 94, 86, 230); border-color: rgba(225, 213, 198, 170); }
        QWidget#homeBody QPushButton#prism:pressed { background: rgba(66, 59, 53, 235); }
        QWidget#homeBody QPushButton:disabled, QWidget#homeBody QPushButton#primary:disabled { color: #aab0ba; border-color: rgba(255, 255, 255, 35); background: rgba(37, 43, 53, 145); }
    )"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(1, 1, 1, 1);
    root->setSpacing(0);
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(kTitleBarHeight);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(24, 0, 0, 0);
    auto* title = new QLabel(launcherTitle, titleBar);
    QFont heading("Noto Sans SC", 14, QFont::Bold);
    heading.setFamilies({ "Noto Sans SC", "Microsoft YaHei UI" });
    title->setObjectName("homeTitle");
    title->setFont(heading);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    auto* minimize = new QPushButton(QString::fromUtf8("−"), titleBar);
    minimize->setObjectName("minimize");
    minimize->setAccessibleName(QStringLiteral("最小化"));
    minimize->setFixedSize(62, kTitleBarHeight);
    connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minimize);
    auto* close = new QPushButton(QString::fromUtf8("×"), titleBar);
    close->setObjectName("close");
    close->setAccessibleName(QStringLiteral("关闭"));
    close->setFixedSize(62, kTitleBarHeight);
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
    m_details = new QLabel(body);
    m_details->setTextFormat(Qt::PlainText);
    m_details->setStyleSheet("font-size: 12px; color: #e1e6ed;");
    m_details->setWordWrap(true);
    layout->addWidget(m_details);
    m_details->hide();
    auto* actions = new QHBoxLayout();
    actions->setSpacing(12);
    auto* prism = new QPushButton(APPLICATION->prismLogo(), QStringLiteral("进入 Prism"), body);
    prism->setIconSize(QSize(20, 20));
    prism->setObjectName("prism");
    connect(prism, &QPushButton::clicked, this, [this] {
        auto* mainWindow = APPLICATION->showMainWindow();
        connect(mainWindow, &MainWindow::isClosing, this, &NiijimaHomeWindow::onPrismClosing, Qt::UniqueConnection);
        mainWindow->show();
        hide();
    });
    actions->addWidget(prism, 0, Qt::AlignBottom);
    actions->addStretch();
    m_packVersion = new QLabel(body);
    m_packVersion->setObjectName("packVersion");
    m_packVersion->setTextFormat(Qt::PlainText);
    m_packVersion->setStyleSheet("font-size: 14px; color: #aaaaaa;");
    m_packVersion->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    m_packVersion->hide();
    actions->addWidget(m_packVersion, 0, Qt::AlignBottom);
    m_cancel = new QPushButton(QStringLiteral("取消"), body);
    m_cancel->hide();
    connect(m_cancel, &QPushButton::clicked, this, [this] {
        if (m_task)
            m_task->abort();
    });
    actions->addWidget(m_cancel, 0, Qt::AlignBottom);
    m_primary = new HomePrimaryButton(body);
    m_primary->setObjectName("primary");
    connect(m_primary, &QPushButton::clicked, this, &NiijimaHomeWindow::primaryAction);
    actions->addWidget(m_primary, 0, Qt::AlignBottom);
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

void NiijimaHomeWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!event->spontaneous())
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

void NiijimaHomeWindow::checkForUpdates()
{
    if (m_task && m_task->isRunning())
        return;
    cancelUpdateCheck();
    const auto generation = m_update.begin();
    refreshPrimaryAction();
    QTimer::singleShot(1000, Qt::PreciseTimer, this, [this, generation] {
        m_update.timeout(generation);
        refreshPrimaryAction();
    });
    const auto url = packIndexUrl();
    auto* task = new Niijima::PackIndexTask(url, APPLICATION->settings()->get("NiijimaPackVersionId").toString(), this);
    m_checkTask = task;
    connect(task, &Task::finished, this, [this, task, generation] {
        if (m_checkTask == task)
            m_checkTask = nullptr;
        if (generation == m_update.generation) {
            if (task->wasSuccessful()) {
                m_update.complete(generation, task->release());
            } else {
                // No popup or status copy for a background check. The one-second gate still expires.
                qWarning() << "Niijima pack check failed:" << task->failReason();
            }
            refreshPrimaryAction();
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
        return std::unexpected(QStringLiteral("Cannot record pack-id: installed instance was not found."));
    return Niijima::writeInstalledPackId(instance->instanceRoot(), packId);
}

void NiijimaHomeWindow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);
    p.setClipPath(clip);
    const QRectF body(0, kTitleBarHeight, width(), height() - kTitleBarHeight);
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
    p.fillRect(QRect(0, 0, width(), kTitleBarHeight), QColor("#f39801"));
    p.setClipping(false);
    p.setPen(QPen(QColor("#d39b45"), 1));
    p.drawPath(clip);
}
void NiijimaHomeWindow::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && e->position().y() < kTitleBarHeight && windowHandle())
        windowHandle()->startSystemMove();
    QWidget::mousePressEvent(e);
}
void NiijimaHomeWindow::closeEvent(QCloseEvent* e)
{
    if (m_task && m_task->isRunning()) {
        QMessageBox::information(this, QStringLiteral("任务进行中"), QStringLiteral("请等待任务结束，或先取消当前任务。"));
        e->ignore();
        return;
    }
    cancelUpdateCheck();
    e->accept();
    emit isClosing();
}
void NiijimaHomeWindow::onPrismClosing()
{
    if (!isHidden())
        return;
    if (m_task && m_task->isRunning()) {
        // Keep an ongoing import accessible after its Prism window closes.
        show();
        raise();
        activateWindow();
    } else {
        // Release the hidden home window so it cannot keep the application alive.
        close();
    }
}
void NiijimaHomeWindow::selectLatestInstance()
{
    const auto* instances = APPLICATION->instances();
    QList<Niijima::InstalledPackCandidate> candidates;
    for (int i = 0; i < instances->count(); ++i) {
        const auto* instance = instances->at(i);
        candidates.append({instance->id(), instance->instanceRoot()});
    }
    const auto selected = Niijima::selectLatestInstalledPack(candidates);
    m_selectedInstance = selected.instanceId;
    m_selectedPackId = selected.packId;
}

void NiijimaHomeWindow::refresh()
{
    if (m_task && m_task->isRunning())
        return;
    refreshPrimaryAction();
    m_cancel->hide();
    m_progress->hide();
    m_details->clear();
    m_details->hide();
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
    if (version.isEmpty()) {
        version = QStringLiteral("未知");
    } else if (!version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        version.prepend(QLatin1Char('v'));
    }
    m_packVersion->setText(QStringLiteral("整合包版本：%1").arg(version));
    m_packVersion->show();
}

void NiijimaHomeWindow::showDownloadError(const QString& message, const QString& details)
{
    refresh();
    m_status->setText(message);
    m_status->show();
    m_details->setText(details);
    m_details->setVisible(!details.isEmpty());
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
        MinecraftAccountPtr account;
        const bool starting = inst->canLaunch();
        if (starting) {
            auto* accounts = APPLICATION->accounts();
            bool hasMicrosoftAccount = false;
            for (int i = 0; i < accounts->count(); ++i)
                hasMicrosoftAccount |= accounts->at(i)->accountType() == AccountType::MSA;
            if (!hasMicrosoftAccount) {
                m_loginInstance = id;
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
        return;
    }
    if (m_update.release) {
        startImport(m_update.release->packUrl);
        return;
    }
    const auto validDownloadUrl = [](const QUrl& url) {
        const bool local = url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
        return url.isValid() && !url.host().isEmpty() && (url.scheme() == "https" || (local && url.scheme() == "http")) &&
               url.userInfo().isEmpty() && !url.hasFragment();
    };
    const auto indexSetting = settings->getSetting("NiijimaPackIndexUrl");
    auto indexUrl = indexSetting->get().toString().trimmed();
    if (indexUrl.isEmpty())
        indexUrl = indexSetting->defValue().toString();
    if (!validDownloadUrl(QUrl(indexUrl))) {
        showDownloadError(QStringLiteral("整合包索引地址无效"), QStringLiteral("请检查配置文件中的 NiijimaPackIndexUrl。"));
        return;
    }
    startReleaseLookup(QUrl(indexUrl), settings->get("NiijimaPackVersionId").toString().trimmed());
}

void NiijimaHomeWindow::startReleaseLookup(const QUrl& indexUrl, const QString& requestedId)
{
    cancelUpdateCheck();
    auto* task = new Niijima::PackIndexTask(indexUrl, requestedId, this);
    m_task = task;
    m_primary->setEnabled(false);
    m_primary->setText(QStringLiteral("加载中"));
    m_details->clear();
    m_details->hide();
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
            showDownloadError(QStringLiteral("获取整合包版本失败，可重试。"), reason);
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
    importer->setIcon(QStringLiteral("default"));
    importer->setGroup(QStringLiteral("新岛铁道"));
    m_import = importer;
    m_committedId.clear();
    auto* task = APPLICATION->instances()->wrapInstanceTask(importer);
    m_task = task;
    task->setParent(this);
    m_primary->setEnabled(false);
    m_primary->setText(QStringLiteral("正在安装"));
    m_transfers.clear();
    m_details->clear();
    m_details->hide();
    m_progress->setRange(0, 1000);
    m_progress->setValue(url.isLocalFile() ? 600 : 0);
    m_progress->setFormat(QStringLiteral("%p%"));
    m_progress->show();
    m_cancel->setText(QStringLiteral("取消"));
    m_cancel->show();
    m_status->setText(QStringLiteral("正在下载整合包…"));
    m_status->show();
    connect(task, &Task::abortStatusChanged, m_cancel, &QWidget::setEnabled);
    connect(task, &Task::abortButtonTextChanged, m_cancel, &QPushButton::setText);
    connect(task, &Task::status, this, [this](const QString& status) { m_status->setText(status.section('\n', 0, 0)); });
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
        const bool success = task->wasSuccessful();
        const bool cancelled = task->getState() == Task::State::AbortedByUser;
        const auto reason = task->failReason();
        if (success)
            m_progress->setValue(1000);
        m_transfers.clear();
        m_task = nullptr;
        m_import = nullptr;
        if (success && release) {
            if (const auto saved = savePackId(release->id); !saved) {
                showDownloadError(QStringLiteral("安装失败，可重试。"), saved.error());
                task->deleteLater();
                return;
            }
        }
        refresh();
        if (!success) {
            m_status->setText(cancelled ? QStringLiteral("已取消安装") : QStringLiteral("安装失败，可重试。"));
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
