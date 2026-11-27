// SPDX-License-Identifier: GPL-3.0-only
#include "StartupFocusWin.h"

#ifdef Q_OS_WIN
#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QProcess>
#include <QScopeGuard>

namespace {
bool launcherInForeground()
{
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// Both writers share Drippy's per-JVM decision. Once signalled it survives a focus loss
// and launcher exit. Drippy signals it too when the startup stage is over.
bool finishStartupFocus(HWND window, DWORD pid, bool handoff)
{
    if (handoff && !launcherInForeground())
        return false;
    const auto channel = QStringLiteral("Local\\Niijima.StartupFocus.") + QString::number(pid);
    HANDLE gate = CreateMutexW(nullptr, FALSE, (channel + ".gate").toStdWString().c_str());
    if (!gate)
        return false;
    const auto closeGate = qScopeGuard([&] { CloseHandle(gate); });
    DWORD wait = WaitForSingleObject(gate, 0);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
        return false;
    const auto releaseGate = qScopeGuard([&] { ReleaseMutex(gate); });
    HANDLE decision = CreateEventW(nullptr, TRUE, FALSE, (channel + ".closed").toStdWString().c_str());
    if (!decision)
        return false;
    const auto closeDecision = qScopeGuard([&] { CloseHandle(decision); });
    DWORD state = WaitForSingleObject(decision, 0);
    if (state != WAIT_TIMEOUT)
        return !handoff && state == WAIT_OBJECT_0;

    HANDLE game = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
    if (!game)
        return false;
    HANDLE gameDecision = nullptr;
    BOOL pinned = DuplicateHandle(GetCurrentProcess(), decision, game, &gameDecision, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(game);
    if (!pinned)
        return false;
    if (handoff) {
        if (!launcherInForeground())
            return false;
        AllowSetForegroundWindow(pid);
        if (!SetForegroundWindow(window))
            return false;
        if (GetForegroundWindow() != window)
            SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 150, nullptr);
        if (GetForegroundWindow() != window)
            return false;
    }
    if (!SetEvent(decision))
        return false;
    qInfo() << (handoff ? "Confirmed startup focus handoff to game PID" : "Observed game window in foreground for PID") << pid;
    return true;
}

// Installed and dispatched on Qt's main thread. Match the window's owner PID,
// including when another process initiated the foreground change.
struct Watch { DWORD pid; QObject* lifetime; };
QHash<HWINEVENTHOOK, Watch> watches;

void CALLBACK foregroundChanged(HWINEVENTHOOK hook, DWORD, HWND window, LONG, LONG, DWORD, DWORD)
{
    // Events are delivered in order but asynchronously. Re-reading GetForegroundWindow()
    // here would lose a genuine visit if the player had already switched away.
    DWORD pid = 0;
    wchar_t name[64];
    const auto watch = watches.constFind(hook);
    if (watch == watches.cend() || !window || !GetWindowThreadProcessId(window, &pid) || pid != watch->pid ||
        GetAncestor(window, GA_ROOT) != window ||
        GetWindow(window, GW_OWNER) || !GetClassNameW(window, name, 64) || wcscmp(name, L"GLFW30") != 0)
        return;
    if (finishStartupFocus(window, pid, false))
        watch->lifetime->deleteLater();
}
}

bool Niijima::handoffStartupFocus(HWND window, DWORD pid)
{
    return finishStartupFocus(window, pid, true);
}

void Niijima::observeStartupFocus(QProcess& process)
{
    DWORD pid = static_cast<DWORD>(process.processId());
    if (!pid)
        return;
    auto hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, foregroundChanged,
                                0, 0, WINEVENT_OUTOFCONTEXT);
    if (!hook) {
        qWarning() << "Cannot observe startup foreground for PID" << pid << GetLastError();
        return;
    }
    auto* lifetime = new QObject(&process);
    watches.insert(hook, {pid, lifetime});
    QObject::connect(lifetime, &QObject::destroyed, qApp, [hook] {
        watches.remove(hook);
        UnhookWinEvent(hook);
    });
    QObject::connect(&process, &QProcess::finished, lifetime, &QObject::deleteLater);
}
#endif
