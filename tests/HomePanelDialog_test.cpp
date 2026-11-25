// SPDX-License-Identifier: GPL-3.0-only
#include <QAbstractButton>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QPainter>
#include <QPointer>
#include <QPauseAnimation>
#include <QTimer>
#include <QTest>
#include "ui/dialogs/MigrationDialog.h"

class HomePanelDialogTest : public QObject {
    Q_OBJECT
    class RoundedHome : public QWidget {
        void paintEvent(QPaintEvent*) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::white);
            painter.drawRoundedRect(rect(), 14, 14);
        }
    };

private slots:
    void migrationOfferIdleTimeout()
    {
        QWidget home;
        home.resize(640, 400);
        QPushButton outside(&home);
        outside.move(400, 300);
        home.show();
        home.activateWindow();
        outside.setFocus();
        bool confirmed = false;
        QPointer<QFrame> offer = migrationOffer(&home, 47, [&] { confirmed = true; });
        QTest::mouseMove(&outside);
        auto* delay = offer->findChild<QPauseAnimation*>();
        QVERIFY(delay);
        QCOMPARE(delay->duration(), 10000);
        QTRY_COMPARE(delay->state(), QAbstractAnimation::Running);

        QTest::mouseMove(offer, QPoint(3, 3));
        QTRY_COMPARE(delay->state(), QAbstractAnimation::Paused);
        auto* cancel = offer->findChild<QPushButton*>("dismiss");
        QVERIFY(cancel);
        cancel->setFocus();
        QTest::mouseMove(&outside);
        QTRY_VERIFY(!offer->underMouse());
        QCOMPARE(delay->state(), QAbstractAnimation::Paused);
        delay->setCurrentTime(9600);
        outside.setFocus();
        QTRY_COMPARE(delay->state(), QAbstractAnimation::Running);
        QVERIFY(delay->currentTime() >= 9600);
        QTRY_VERIFY_WITH_TIMEOUT(offer.isNull(), 1500);
        QVERIFY(!confirmed);
    }

    void confirmInstallExit()
    {
        QWidget home;
        auto* prompt = makeInstallExitPrompt(&home);
        QCOMPARE(prompt->windowTitle(), QStringLiteral("正在安装整合包"));
        QCOMPARE(prompt->text(), QStringLiteral("是否中止安装并退出启动器"));
        QCOMPARE(prompt->defaultButton()->text(), QStringLiteral("否"));
        QCOMPARE(prompt->escapeButton(), prompt->button(QMessageBox::No));
        QCOMPARE(prompt->button(QMessageBox::Yes)->text(), QStringLiteral("中止安装并退出启动器"));

        prompt->open();
        QTest::keyClick(prompt, Qt::Key_Return);
        QCOMPARE(prompt->result(), int(QMessageBox::No));
        prompt->open();
        QTest::keyClick(prompt, Qt::Key_Escape);
        QCOMPARE(prompt->result(), int(QMessageBox::No));
        prompt->open();
        prompt->button(QMessageBox::Yes)->click();
        QCOMPARE(prompt->result(), int(QMessageBox::Yes));
    }

    void channelCodeRequiresNonblankInput()
    {
        QWidget home;
        bool emptyDisabled = false, spacesDisabled = false, stayedOpen = false, clearedDisabled = false;
        QTimer::singleShot(0, &home, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto* input = dialog->findChild<QLineEdit*>();
            auto* box = dialog->findChild<QDialogButtonBox*>();
            if (input && box) {
                for (auto* button : box->buttons()) {
                    if (box->buttonRole(button) != QDialogButtonBox::AcceptRole)
                        continue;
                    emptyDisabled = !button->isEnabled();
                    input->setText("  ");
                    spacesDisabled = !button->isEnabled();
                    QTest::keyClick(input, Qt::Key_Return);
                    stayedOpen = dialog->isVisible();
                    input->setText("x");
                    input->clear();
                    clearedDisabled = !button->isEnabled();
                    input->setText(" abc ");
                    if (button->isEnabled()) {
                        button->click();
                        return;
                    }
                }
            }
            dialog->reject();
        });
        auto result = requestHomeChannelCode(&home);
        QVERIFY(emptyDisabled && spacesDisabled && stayedOpen && clearedDisabled);
        QCOMPARE(result.value_or(QString{}), QStringLiteral("abc"));
    }

    void backdropPreservesHomeCorners()
    {
        RoundedHome home;
        home.setAttribute(Qt::WA_TranslucentBackground);
        home.resize(480, 300);
        const auto original = home.grab().toImage();
        HomePanelDialog dialog(&home, {}, 200);
        QVERIFY(dialog.testAttribute(Qt::WA_TranslucentBackground));
        const auto modal = dialog.grab().toImage();
        QCOMPARE(modal.size(), original.size());
        QCOMPARE(original.pixelColor(0, 0).alpha(), 0);
        const int radius = qRound(14 * modal.devicePixelRatio());
        for (int y = 0; y < radius; ++y) {
            for (int x = 0; x < radius; ++x) {
                for (const int px : {x, modal.width() - 1 - x}) {
                    for (const int py : {y, modal.height() - 1 - y}) {
                        const auto alpha = original.pixelColor(px, py).alpha();
                        QVERIFY(modal.pixelColor(px, py).alpha() <= alpha + 1);
                    }
                }
            }
        }
        const auto interior = modal.pixelColor(modal.width() / 2, 2 * radius);
        QVERIFY(interior.alpha() >= 254); // The eight-bit blur can round a solid pixel down by one.
        QVERIFY(interior.red() < 240);
    }
};

QTEST_MAIN(HomePanelDialogTest)
#include "HomePanelDialog_test.moc"
