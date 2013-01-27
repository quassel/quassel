/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef HAVE_KDE

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

#include "iconloader.h"
#include "quassel.h"
#include "util.h"

IconLoader IconLoader::_iconLoader;
int IconLoader::_groupSize[] = { 48, 22, 22, 16, 32, 22 };  // default sizes taken from Oxygen

IconLoader *IconLoader::global()
{
    // Workaround: the static _iconLoader might be initialized before the resources it needs
    // This way, first call to global() will init it by setting the theme
    if (_iconLoader.theme().isEmpty())
        _iconLoader.setTheme("oxygen");
    return &_iconLoader;
}


IconLoader::IconLoader(QObject *parent) : QObject(parent)
{
    // setTheme("oxygen");
}


IconLoader::~IconLoader()
{
}


void IconLoader::setTheme(const QString &theme)
{
    _theme = theme;
    // check which dirs could contain themed icons
    _themedIconDirNames.clear();
    _plainIconDirNames.clear();

    // First, look for a system theme
    // This is supposed to only work on Unix, though other platforms might set $XDG_DATA_DIRS if they please.
    QStringList iconDirNames = QString(qgetenv("XDG_DATA_DIRS")).split(':', QString::SkipEmptyParts);
    if (!iconDirNames.isEmpty()) {
        for (int i = 0; i < iconDirNames.count(); i++)
            iconDirNames[i].append(QString("/icons/"));
    }
#ifdef Q_OS_UNIX
    else {
        // Provide a fallback
        iconDirNames << "/usr/share/icons/";
    }
    // Add our prefix too
    QString appDir = QCoreApplication::applicationDirPath();
    int binpos = appDir.lastIndexOf("/bin");
    if (binpos >= 0) {
        appDir.replace(binpos, 4, "/share");
        appDir.append("/icons/");
        if (!iconDirNames.contains(appDir))
            iconDirNames.append(appDir);
    }
#endif

    // Now look for an icons/ subdir in our data paths
    foreach(const QString &dir, Quassel::dataDirPaths())
    iconDirNames << dir + "icons/";

    // Add our resource path too
    iconDirNames << ":/icons/";

    // Ready do add theme names
    foreach(const QString &dir, iconDirNames) {
        QString path = dir + theme + '/';
        if (QFile::exists(path))
            _themedIconDirNames << path;
    }
    foreach(const QString &dir, iconDirNames) {
        QString path = dir + "hicolor/";
        if (QFile::exists(path))
            _themedIconDirNames << path;
    }

    // We ship some plain (non-themed) icons in $data/pics
    foreach(const QString &dir, Quassel::dataDirPaths()) {
        QString path = dir + "pics/";
        if (QFile::exists(path))
            _plainIconDirNames << path;
    }
    // And of course, our resource path
    if (QFile::exists(":/pics"))
        _plainIconDirNames << ":/pics";
}


// TODO: optionally implement cache (speed/memory tradeoff?)
QPixmap IconLoader::loadIcon(const QString &name, IconLoader::Group group, int size)
{
    if (group < 0 || group >= LastGroup) {
        qWarning() << "Invalid icon group!";
        return QPixmap();
    }
    if (size == 0)
        size = _groupSize[group];

    QString path = findIconPath(name, size);
    if (path.isEmpty()) return QPixmap();

    // load the icon
    return QPixmap(path);
}


QString IconLoader::findIconPath(const QString &name, int size)
{
    QString fname = QString("%1.png").arg(name); // we only support PNG so far
    // First, look for a themed icon... we don't do anything fancy here, only exact match for both name and size
    foreach(QString basedir, _themedIconDirNames) {
        QDir sizedir(QString("%1/%2x%2").arg(basedir).arg(QString::number(size)));
        if (sizedir.exists()) {
            // ignore context, i.e. scan all subdirs
            QStringList contextdirs = sizedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            foreach(QString dir, contextdirs) {
                QString path = QString("%1/%2/%3").arg(sizedir.absolutePath(), dir, fname);
                if (QFile::exists(path)) return path;
            }
        }
    }
    // Now check the plain dirs
    foreach(QString dir, _plainIconDirNames) {
        QString path = QString("%1/%2").arg(dir, name);
        if (QFile::exists(path)) return path;
    }

    qWarning() << "Icon not found:" << name << size;
    return QString();
}


// Convenience constructors

QPixmap DesktopIcon(const QString &name, int force_size)
{
    IconLoader *loader = IconLoader::global();
    return loader->loadIcon(name, IconLoader::Desktop, force_size);
}


QPixmap BarIcon(const QString &name, int force_size)
{
    IconLoader *loader = IconLoader::global();
    return loader->loadIcon(name, IconLoader::Toolbar, force_size);
}


QPixmap MainBarIcon(const QString &name, int force_size)
{
    IconLoader *loader = IconLoader::global();
    return loader->loadIcon(name, IconLoader::MainToolbar, force_size);
}


QPixmap SmallIcon(const QString &name, int force_size)
{
    IconLoader *loader = IconLoader::global();
    return loader->loadIcon(name, IconLoader::Small, force_size);
}


#endif
