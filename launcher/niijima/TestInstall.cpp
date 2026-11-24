// SPDX-License-Identifier: GPL-3.0-only
#include "TestInstall.h"
#include "Application.h"
#include "LaunchController.h"
#include "launch/LaunchTask.h"
#include "launch/LogModel.h"
#include <QUuid>
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "niijima/InstalledPacks.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

void Niijima::observeTestLaunch(LaunchController* controller)
{
    QDir directory(QDir(controller->instance()->gameRoot()).filePath(".niijima-test"));
    QFile request(directory.filePath("request.json"));
    if (!request.open(QIODevice::ReadOnly))
        return;
    const auto plan = QJsonDocument::fromJson(request.readAll()).object();
    auto id = QUuid(plan.value("run").toString());
    if (id.isNull())
        return;
    controller->instance()->setProperty("niijimaTestHeapCap", plan.value("heapMiB").toInt());
    const auto session = id.toString(QUuid::WithoutBraces);
    QObject::connect(controller, &LaunchController::finished, controller, [directory, session, controller] {
        controller->instance()->setProperty("niijimaTestHeapCap", QVariant());
        QStringList errors;
        if (!directory.exists(session + "/result.json")) {
            if (auto* task = controller->instance()->getLaunchTask()) {
                const auto lines = task->getLogModel()->toPlainText().split('\n');
                for (const auto& line : lines) {
                    if (line.startsWith("Caused by:") || line.startsWith("Exception in thread"))
                        errors.append(line);
                }
            }
        }
        QSaveFile file(directory.filePath(session + ".launch.json"));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(QJsonObject{{"error", controller->failReason()}, {"exception", errors.join('\n')}}).toJson());
            file.commit();
        }
    });
}

void Niijima::installTest(const QString& packId, const QString& directory, bool exitWhenDone)
{
    const QString output = QDir(directory).filePath("installed.json");
    auto finish = [output, exitWhenDone](QJsonObject result) {
        QSaveFile file(output);
        auto bytes = QJsonDocument(result).toJson();
        bool written = file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
        if (!written)
            qWarning() << "Cannot write test installation result:" << file.errorString();
        if (exitWhenDone)
            QTimer::singleShot(0, qApp, [ok = written && !result.contains("error")] { QCoreApplication::exit(ok ? 0 : 1); });
    };
    const auto& local = APPLICATION->localTest();
    QFile index(QDir(local.packs).filePath("latest.json"));
    if (!local.enabled || !local.error.isEmpty() || !index.open(QIODevice::ReadOnly)) {
        finish({{"error", "Local test distribution unavailable"}});
        return;
    }
    auto release = parsePackIndex(index.readAll(), QUrl(local.root + "packs/latest.json"), packId);
    if (!release || !release->version.contains(".test")) {
        finish({{"error", release ? QStringLiteral("Expected a test release") : release.error()}});
        return;
    }
    const QUrl zip = local.resolve(release->packUrl);
    if (!zip.isLocalFile() || !QFileInfo(zip.toLocalFile()).isFile()) {
        finish({{"error", "Local release ZIP missing"}});
        return;
    }
    auto* importer = new InstanceImportTask(zip, false);
    importer->setCreateNewInstance(true);
    importer->setNiijimaRelease(true);
    importer->setGroup(QStringLiteral("新岛铁道-测试"));
    auto* task = APPLICATION->instances()->wrapInstanceTask(importer);
    auto installed = std::make_shared<QString>();
    QObject::connect(APPLICATION->instances(), &InstanceList::instanceCommitted, task,
                     [importer, installed](const InstanceTask* source, const QString& id) {
                         if (source == importer)
                             *installed = id;
                     });
    QObject::connect(task, &Task::finished, task, [task, installed, release = *release, finish] {
        task->deleteLater();
        auto* instance = APPLICATION->instances()->getInstanceById(*installed);
        if (!task->wasSuccessful() || !instance) {
            finish({{"error", task->wasSuccessful() ? QStringLiteral("Missing committed instance") : task->failReason()}});
            return;
        }
        auto verified = verifyInstalledPackId(instance->instanceRoot(), instance->gameRoot(), release.id);
        if (!verified) {
            finish({{"error", verified.error()}});
            return;
        }
        finish({{"instance", *installed}, {"gameRoot", QFileInfo(instance->gameRoot()).absoluteFilePath()},
                {"packId", QString::number(release.id)}, {"version", release.version}});
    });
    task->start();
}
