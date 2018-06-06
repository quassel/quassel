/***************************************************************************
*   Copyright (C) 2005-2016 by the Quassel Project                        *
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
#include "extraicon.h"
#include "quassel.h"
#include <QIcon>
#include <QDirIterator>

QIcon ExtraIcon::load(const QString _name) {
    QString name = _name;
    QString theme = QIcon::themeName();
    if (theme == "breeze-dark") {
        theme = "breezedark";
    }
    else if (theme != "breeze" && theme != "breezedark" && theme != "oxygen") {
        if (QIcon::hasThemeIcon(name)) {
            return QIcon::fromTheme(name);
        }
        theme = "breeze";
    }

    QStringList iconSearchPaths = ExtraIcon::iconSearchPaths();

    QStringList icons = ExtraIcon::getIconList(iconSearchPaths, theme, name);

    while(icons.length() == 0 && name != ""){
        name = ExtraIcon::getNextFallbackName(name);
        if (name == "") {
            return QIcon(":/icons/" + _name + ".png");
        }
        icons = ExtraIcon::getIconList(iconSearchPaths, theme, name);
    }

    QList<QString>::Iterator iconsIt = icons.begin();
    QIcon qicon = QIcon(*iconsIt);
    iconsIt++;
    while (iconsIt!=icons.end()) {
        qicon.addFile(*iconsIt);
        iconsIt++;
    }

    return qicon;
}

QStringList ExtraIcon::iconSearchPaths() {
    QStringList iconSearchPaths;

#ifdef EMBED_DATA
    iconSearchPaths.push_back(":/icons/extra/");
#else
    // add data dir paths only here?
#endif
    for (auto& dataPath : Quassel::dataDirPaths()) {
        iconSearchPaths.push_back(dataPath + "icons/extra/");
    }
    return iconSearchPaths;
}

QStringList ExtraIcon::getIconList(QStringList iconSearchPaths, QString theme, QString name) {
    QStringList icons;

    for (auto& iconSearchPath : iconSearchPaths) {
        QDirIterator it(iconSearchPath + theme, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString icon = it.next();
            if (icon.contains("/" + name + ".")) {
                icons.push_back(icon);
            }
        }
    }
    return icons;
}

QString ExtraIcon::getNextFallbackName(QString name) {
    int index = name.lastIndexOf("-");
    if (index == -1) {
        return "";
    }
    return name.left(index);
}