// SPDX-License-Identifier: GPL-3.0-only
#include "NiijimaHomeWindow.h"
#include <QCloseEvent>
#include <QComboBox>
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
#include <QSignalBlocker>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QtMath>
#include <QVBoxLayout>
#include <QWindow>
#include "Application.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "niijima/PackIndex.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"

namespace {
constexpr int kTitleBarHeight = 46;

// Measure the visible glyphs, so the font's trailing letter spacing does not add uneven padding.
class HomePrimaryButton : public QPushButton {
public:
    explicit HomePrimaryButton(QWidget* parent) : QPushButton(parent) { setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); }

    QSize sizeHint() const override
    {
        ensurePolished();
        const auto bounds = QFontMetricsF(font()).tightBoundingRect(text());
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
        painter.setFont(font());
        painter.setPen(isEnabled() ? QColor(Qt::white) : QColor("#aab0ba"));
        const auto bounds = QFontMetricsF(font()).tightBoundingRect(label);
        painter.drawText(QRectF(rect()).center() - bounds.center(), label);
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
        QPushButton#close { background: transparent; border: 0; border-radius: 0; font-size: 20px; padding: 0; color: #ffffff; }
        QPushButton#close:hover { background: #c42b1c; color: white; }
        QComboBox { border: 1px solid #cba169; border-radius: 6px; padding: 7px; background: #fff8eb; color: #452800; }
        QProgressBar { border: 0; background: #f9dca8; color: #452800; text-align: center; font-size: 12px; }
        QProgressBar::chunk { background: #ecb354; }
        QWidget#homeBody QLabel { color: #fff9ee; }
        QWidget#homeBody QPushButton { background: rgba(30, 36, 47, 155); border-color: rgba(255, 249, 238, 80); color: #fff9ee; }
        QWidget#homeBody QPushButton:hover { background: rgba(65, 73, 86, 190); border-color: rgba(255, 249, 238, 145); }
        QWidget#homeBody QPushButton:pressed { background: rgba(19, 26, 37, 210); }
        QWidget#homeBody QPushButton#primary { background: #f39801; border-color: #ffbd52; color: #ffffff; font-size: 36px; font-weight: 700; letter-spacing: 18px; padding: 0; }
        QWidget#homeBody QPushButton#primary:hover { background: #ffad28; }
        QWidget#homeBody QPushButton#primary:pressed { background: #de8600; }
        QWidget#homeBody QPushButton#prism { background: rgba(84, 75, 70, 205); border-color: rgba(194, 181, 166, 130); border-radius: 8px; font-size: 13px; padding: 8px 14px; }
        QWidget#homeBody QPushButton#prism:hover { background: rgba(105, 94, 86, 230); border-color: rgba(225, 213, 198, 170); }
        QWidget#homeBody QPushButton#prism:pressed { background: rgba(66, 59, 53, 235); }
        QWidget#homeBody QPushButton:disabled, QWidget#homeBody QPushButton#primary:disabled { color: #aab0ba; border-color: rgba(255, 255, 255, 35); background: rgba(37, 43, 53, 145); }
        QWidget#homeBody QComboBox { background: rgba(24, 31, 43, 195); color: #fff9ee; border-color: rgba(255, 249, 238, 80); }
        QWidget#homeBody QComboBox QAbstractItemView { background: #252e3b; color: #fff9ee; selection-background-color: #f39801; selection-color: #392100; }
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
    m_instances = new QComboBox(body);
    layout->addWidget(m_instances);
    connect(m_instances, &QComboBox::currentIndexChanged, this, [this] {
        if (m_instances->currentIndex() >= 0)
            APPLICATION->settings()->set("NiijimaPreferredInstance", m_instances->currentData().toString());
        refreshPackVersion();
    });
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
    m_packVersion->setStyleSheet("font-size: 18px;");
    m_packVersion->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_packVersion->hide();
    actions->addWidget(m_packVersion, 0, Qt::AlignVCenter);
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
    connect(APPLICATION->instances(), &InstanceList::instanceCommitted, this, [this](const InstanceTask* task, const QString& id) {
        if (task == m_import) {
            m_committedId = id;
            remember(id);
        }
    });
    refresh();
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
void NiijimaHomeWindow::remember(const QString& id)
{
    auto* s = APPLICATION->settings();
    auto ids = s->get("NiijimaInstanceIds").toStringList();
    if (!ids.contains(id))
        ids.append(id);
    s->set("NiijimaInstanceIds", ids);
    s->set("NiijimaPreferredInstance", id);
}
void NiijimaHomeWindow::refresh()
{
    if (m_task && m_task->isRunning())
        return;
    auto* s = APPLICATION->settings();
    auto ids = s->get("NiijimaInstanceIds").toStringList();
    const auto selected = s->get("NiijimaPreferredInstance").toString();
    const QSignalBlocker blocker(m_instances);
    m_instances->clear();
    for (const auto& id : ids)
        if (auto* inst = APPLICATION->instances()->getInstanceById(id))
            m_instances->addItem(inst->name(), id);
    int index = m_instances->findData(selected);
    if (index < 0 && m_instances->count())
        index = 0;
    m_instances->setCurrentIndex(index);
    m_instances->setVisible(m_instances->count() > 1);
    if (index >= 0)
        s->set("NiijimaPreferredInstance", m_instances->currentData().toString());
    refreshPackVersion();
    m_primary->setText(index >= 0 ? QStringLiteral("启动游戏") : QStringLiteral("下载游戏"));
    m_primary->setEnabled(true);
    m_cancel->hide();
    m_progress->hide();
    m_details->clear();
    m_details->hide();
    m_status->clear();
    m_status->hide();
}
void NiijimaHomeWindow::refreshPackVersion()
{
    auto* instance = APPLICATION->instances()->getInstanceById(m_instances->currentData().toString());
    if (!instance) {
        m_packVersion->clear();
        m_packVersion->hide();
        return;
    }
    // Direct ZIP imports preserve their versionId here even without managed-project metadata.
    QString version;
    QFile index(QDir(instance->instanceRoot()).filePath(QStringLiteral("mrpack/modrinth.index.json")));
    if (index.open(QIODevice::ReadOnly))
        version = QJsonDocument::fromJson(index.readAll()).object().value(QStringLiteral("versionId")).toString().trimmed();
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
    if (m_task && m_task->isRunning())
        return;
    auto* settings = APPLICATION->settings();
    const auto id = settings->get("NiijimaPreferredInstance").toString();
    if (auto* inst = APPLICATION->instances()->getInstanceById(id)) {
        APPLICATION->launch(inst);
        return;
    }
    const auto& local = APPLICATION->localTest();
    if (local.enabled) {
        if (!local.error.isEmpty())
            showDownloadError(QStringLiteral("本地测试配置无效"), local.error);
        else
            startImport(QUrl::fromLocalFile(local.pack));
        return;
    }
    const auto validDownloadUrl = [](const QUrl& url) {
        const bool local = url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
        return url.isValid() && !url.host().isEmpty() && (url.scheme() == "https" || (local && url.scheme() == "http")) &&
               url.userInfo().isEmpty() && !url.hasFragment();
    };
    // Existing explicit ZIP overrides remain available through the configuration file.
    const auto directUrl = settings->get("NiijimaPackUrl").toString().trimmed();
    if (!directUrl.isEmpty()) {
        const QUrl url(directUrl);
        if (!validDownloadUrl(url)) {
            showDownloadError(QStringLiteral("整合包地址无效"), QStringLiteral("请检查配置文件中的 NiijimaPackUrl。"));
            return;
        }
        startImport(url);
        return;
    }

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
    auto* task = new Niijima::PackIndexTask(indexUrl, requestedId, this);
    m_task = task;
    m_primary->setEnabled(false);
    m_primary->setText(QStringLiteral("正在检查"));
    m_instances->hide();
    m_details->clear();
    m_details->hide();
    m_status->setText(QStringLiteral("正在获取整合包版本…"));
    m_status->show();
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
    // A configurable URL is not automatically a trusted distribution source.
    auto* importer = new InstanceImportTask(url, false, this);
    importer->setName(QStringLiteral("新岛铁道"));
    importer->setGroup(QStringLiteral("新岛铁道"));
    m_import = importer;
    m_committedId.clear();
    auto* task = APPLICATION->instances()->wrapInstanceTask(importer);
    m_task = task;
    task->setParent(this);
    m_primary->setEnabled(false);
    m_instances->hide();
    m_primary->setText(QStringLiteral("正在安装"));
    m_details->clear();
    m_details->hide();
    m_progress->setRange(0, 0);
    m_progress->setFormat(QStringLiteral("%p%"));
    m_progress->show();
    m_cancel->setText(QStringLiteral("取消"));
    m_cancel->show();
    m_status->setText(QStringLiteral("正在下载整合包…"));
    m_status->show();
    connect(task, &Task::abortStatusChanged, m_cancel, &QWidget::setEnabled);
    connect(task, &Task::abortButtonTextChanged, m_cancel, &QPushButton::setText);
    connect(task, &Task::status, this, [this](const QString& status) { m_status->setText(status.section('\n', 0, 0)); });
    connect(task, &Task::progress, this, [this](qint64 cur, qint64 total) {
        if (total <= 0) {
            m_progress->setRange(0, 0);
            return;
        }
        m_progress->setRange(0, 1000);
        m_progress->setValue(int(1000.0 * cur / total));
    });
    connect(task, &Task::stepProgress, this, [this](const TaskStepProgress& p) {
        if (p.state == TaskStepState::Running && !p.details.isEmpty()) {
            auto detail = p.details;
            detail.replace('\n', QStringLiteral(" · "));
            // These values belong to the current file, not the whole installation.
            m_details->setText(QStringLiteral("当前文件 · %1").arg(detail));
            m_details->show();
            m_progress->setFormat(p.details.contains('\n') ? p.details.section('\n', -1) : QStringLiteral("%p%"));
        } else if (p.isDone()) {
            m_progress->setFormat(QStringLiteral("%p%"));
            m_details->clear();
            m_details->hide();
        }
    });
    connect(task, &Task::finished, this, [this, task] {
        const bool success = task->wasSuccessful();
        const bool cancelled = task->getState() == Task::State::AbortedByUser;
        const auto reason = task->failReason();
        m_task = nullptr;
        m_import = nullptr;
        refresh();
        if (success) {
            m_status->setText(m_committedId.isEmpty() ? QStringLiteral("实例已导入，请进入 Prism 查看。")
                                                      : QStringLiteral("实例已安装，启动时会检查并补齐运行文件。"));
        } else {
            m_status->setText(cancelled ? QStringLiteral("已取消安装") : QStringLiteral("安装失败，可重试。"));
            if (!cancelled)
                QMessageBox::warning(this, QStringLiteral("安装失败"), reason);
        }
        m_status->show();
        task->deleteLater();
    });
    task->start();
    m_cancel->setEnabled(task->canAbort());
}
