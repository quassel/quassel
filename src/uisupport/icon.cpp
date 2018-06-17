/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "icon.h"

#include <set>

#include <QDebug>

namespace icon {

namespace {

void printWarning(const QString &iconName, const QString &extra = {})
{
    static std::set<QString> warnedAbout;
    if (warnedAbout.insert(iconName).second) {
        qWarning() << "Missing icon:" << iconName << qPrintable(extra);
    }
}

}


QIcon get(const QString &iconName, const QString &fallbackPath)
{
    return get(std::vector<QString>{iconName}, fallbackPath);
}


QIcon get(const std::vector<QString> &iconNames, const QString &fallbackPath)
{
    for (auto &&iconName : iconNames) {
        // Exact match
        if (QIcon::hasThemeIcon(iconName)) {
            return QIcon::fromTheme(iconName);
        }
    }

    for (auto &&iconName : iconNames) {
        // Try to get something from the theme anyway (i.e. a more generic fallback)
        QIcon fallback = QIcon::fromTheme(iconName);
        if (!fallback.availableSizes().isEmpty()) {
            printWarning(iconName, QString{"(using fallback: \"%1\")"}.arg(fallback.name()));
            return fallback;
        }
    }

    // Build error string
    QStringList requested;
    for (auto &&iconName : iconNames) {
        requested << iconName;
    }
    QString missing = "{" + requested.join(", ") + "}";

    // Nothing from the theme, so try to load from path if given
    if (!fallbackPath.isEmpty()) {
        QIcon fallback{fallbackPath};
        if (!fallback.availableSizes().isEmpty()) {
            printWarning(missing, QString{"(using fallback: \"%1\")"}.arg(fallbackPath));
            return fallback;
        }
    }

    // Meh.
    printWarning(missing);
    return {};
}

}
