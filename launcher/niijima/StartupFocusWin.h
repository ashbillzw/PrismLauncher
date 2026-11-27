// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <qt_windows.h>
class QProcess;
namespace Niijima {
void observeStartupFocus(QProcess& process);
bool handoffStartupFocus(HWND window, DWORD pid);
}
#endif
