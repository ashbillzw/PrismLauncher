// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QString>

class LaunchController;
namespace Niijima {
void observeTestLaunch(LaunchController* controller);
void installTest(const QString& packId, const QString& directory, bool exitWhenDone);
}
