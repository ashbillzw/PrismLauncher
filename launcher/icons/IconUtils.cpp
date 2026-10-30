// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "IconUtils.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDirListing>
#include <QFileInfo>

static const QStringList g_validIconExtensions = { { "svg", "png", "ico", "gif", "jpg", "jpeg", "webp" } };

namespace IconUtils {

QString findBestIconIn(const QString& folder, const QString& iconKey)
{
    for (const auto& entry : QDirListing(folder, QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks)) {
        auto fileInfo = entry.fileInfo();
        if ((fileInfo.completeBaseName() == iconKey || fileInfo.fileName() == iconKey) && isIconSuffix(fileInfo.suffix())) {
            return fileInfo.absoluteFilePath();
        }
    }
    return {};
}

namespace {
QString findPclIcon(const QString& gameRoot)
{
    QFile config(QDir(gameRoot).filePath("PCL/Setup.ini"));
    if (!config.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream input(&config);
    QString logo;
    bool custom = false;
    while (!input.atEnd()) {
        const auto line = input.readLine().trimmed();
        const auto separator = line.indexOf(':');
        if (separator < 0)
            continue;
        const auto key = line.left(separator).trimmed();
        const auto value = line.mid(separator + 1).trimmed();
        if (key == "Logo")
            logo = value;
        else if (key == "LogoCustom")
            custom = value.compare("True", Qt::CaseInsensitive) == 0;
    }
    logo.replace('\\', '/');
    // Imported archives may only refer to their own files, not host paths or URLs.
    if (!custom || logo.isEmpty() || QDir::isAbsolutePath(logo) || logo.contains(':'))
        return {};
    const QFileInfo image(QDir(gameRoot).filePath(logo));
    const auto canonical = image.canonicalFilePath();
    const auto relative = QDir(QFileInfo(gameRoot).canonicalFilePath()).relativeFilePath(canonical);
    if (!image.isFile() || canonical.isEmpty() || relative == ".." || relative.startsWith("../") ||
        QDir::isAbsolutePath(relative) || !isIconSuffix(image.suffix().toLower()))
        return {};
    return canonical;
}
}  // namespace

QString findPackIcon(const QString& root, const QString& iconKey)
{
    auto path = findBestIconIn(root, iconKey);
    if (path.isEmpty())
        path = findBestIconIn(root, "icon.png");
    if (path.isEmpty())
        path = findBestIconIn(QDir(root).filePath("overrides"), "icon.png");
    if (!path.isEmpty())
        return path;
    // Client overrides take precedence over common overrides in a mrpack.
    for (const auto& folder : {QDir(root).filePath("client-overrides"), QDir(root).filePath("overrides"), root}) {
        path = findPclIcon(folder);
        if (!path.isEmpty())
            return path;
    }
    return {};
}

QString getIconFilter()
{
    return "(*." + g_validIconExtensions.join(" *.") + ")";
}

bool isIconSuffix(const QString& suffix)
{
    return g_validIconExtensions.contains(suffix);
}

}  // namespace IconUtils
