// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <cmath>
#include <QElapsedTimer>
#include <QEvent>
#include <QList>
#include <QPainter>
#include <QTimer>
#include <QWidget>

// An overlay anchored to the primary button, independent of the footer layout.
class NiijimaInstallStatus final : public QWidget {
public:
    explicit NiijimaInstallStatus(QWidget* button) : QWidget(button->parentWidget()), m_button(button)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        auto labelFont = button->parentWidget()->font();
        labelFont.setPixelSize(14);
        setFont(labelFont);
        m_button->installEventFilter(this);
        m_frame.setTimerType(Qt::PreciseTimer);
        m_frame.setInterval(16);
        connect(&m_frame, &QTimer::timeout, this, &NiijimaInstallStatus::advance);
        hide();
    }

    void append(const QString& message)
    {
        const QString line = message.section('\n', 0, 0);
        if (line.isEmpty() || (!m_lines.isEmpty() && m_lines.first().text == line))
            return;
        if (m_frame.isActive())
            advance();
        if (!m_lines.isEmpty())
            m_rowsToScroll += 1;
        m_lines.prepend({ line, 1.0 });
        setAccessibleName(line);
        if (isHidden()) {
            followButton();
            show();
        }
        if (m_rowsToScroll > 0 && !m_frame.isActive()) {
            m_clock.start();
            m_frame.start();
        }
        update();
    }

    void clear()
    {
        hide();
        m_frame.stop();
        m_lines.clear();
        m_rowsToScroll = 0;
        setAccessibleName({});
    }

protected:
    bool eventFilter(QObject* target, QEvent* event) override
    {
        if (target == m_button && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
            followButton();
        return QWidget::eventFilter(target, event);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(QColor("#fff9ee"));
        const auto metrics = fontMetrics();
        const int pitch = metrics.lineSpacing() + 2;
        for (qsizetype i = 0; i < m_lines.size(); ++i) {
            const qreal top = height() - (i + 1 - m_rowsToScroll) * pitch;
            if (top + pitch <= 0)
                break;
            if (top >= height())
                continue;
            painter.setOpacity(m_lines[i].opacity);
            painter.drawText(QRectF(0, top, width(), pitch), Qt::AlignCenter,
                             metrics.elidedText(m_lines[i].text, Qt::ElideRight, width()));
        }
    }

private:
    void followButton()
    {
        const int bottom = qMax(0, m_button->y() - 6);
        setGeometry(m_button->x(), 0, m_button->width(), bottom);
    }

    void advance()
    {
        const qreal seconds = m_clock.restart() / 1000.0;
        const qreal previous = m_rowsToScroll;
        m_rowsToScroll = qMax(0.0, previous - seconds / .18);
        for (qsizetype i = 3; i < m_lines.size(); ++i) {
            const qreal row = i + 1 - m_rowsToScroll;
            if (row < 4)
                continue;
            // Count only time spent at or above row four, including its arrival frame.
            const qreal fading = qMax(0.0, seconds - qMax(0.0, previous - (i - 3)) * .18);
            const qreal duration = 3.0 * std::pow(1.0 / 6.0, (row - 4) / 4);
            m_lines[i].opacity = qMax(0.0, m_lines[i].opacity - fading / duration);
        }
        while (!m_lines.isEmpty() && m_lines.last().opacity <= 0)
            m_lines.removeLast();
        if (m_rowsToScroll == 0 && m_lines.size() <= 3)
            m_frame.stop();
        update();
    }

    struct Line {
        QString text;
        qreal opacity;
    };
    QWidget* m_button;
    QList<Line> m_lines;
    qreal m_rowsToScroll = 0;
    QElapsedTimer m_clock;
    QTimer m_frame;
};
