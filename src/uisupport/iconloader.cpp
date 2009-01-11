/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef HAVE_KDE

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

#include "iconloader.h"
#include "util.h"

IconLoader IconLoader::_iconLoader;
int IconLoader::_groupSize[] = { 48, 22, 22, 16, 32, 22 };  // default sizes taken from Oxygen

IconLoader *IconLoader::global() {
  // Workaround: the static _iconLoader might be initialized before the resources it needs
  // This way, first call to global() will init it by setting the theme
  if(_iconLoader.theme().isEmpty())
    _iconLoader.setTheme("oxygen");
  return &_iconLoader;
}

IconLoader::IconLoader(QObject *parent) : QObject(parent) {

  // setTheme("oxygen");
}

IconLoader::~IconLoader() {

}

void IconLoader::setTheme(const QString &theme) {
  _theme = theme;
  // check which dirs could contain themed icons
  _themedIconDirNames.clear();
  _plainIconDirNames.clear();
  QString path;
  QStringList dataDirNames = dataDirPaths();

  // System theme in $data/icons/$theme
  foreach(QString dir, dataDirNames) {
    path = QString("%1/icons/%2").arg(dir, theme);
    if(QFile::exists(path))
      _themedIconDirNames.append(path);
  }
  // Resource for system theme :/icons/$theme
  path = QString(":/icons/%1").arg(theme);
  if(QFile::exists(path))
    _themedIconDirNames.append(path);

  // Own icons in $data/apps/quassel/icons/hicolor
  // Also, plain icon dirs $data/apps/quassel/pics
  foreach(QString dir, dataDirNames) {
    path = QString("%1/icons/hicolor").arg(dir);
    if(QFile::exists(path))
      _themedIconDirNames.append(path);
    path = QString("%1/apps/quassel/pics").arg(dir);
    if(QFile::exists(path))
      _plainIconDirNames.append(path);
  }

  // Same for :/icons/hicolor and :/pics
  path = QString(":/icons/hicolor");
  if(QFile::exists(path))
    _themedIconDirNames.append(path);

  path = QString(":/pics");
  if(QFile::exists(path))
    _plainIconDirNames.append(path);
}

// TODO: optionally implement cache (speed/memory tradeoff?)
QPixmap IconLoader::loadIcon(const QString &name, IconLoader::Group group, int size) {
  if(group < 0 || group >= LastGroup) {
    qWarning() << "Invalid icon group!";
    return QPixmap();
  }
  if(size == 0)
    size = _groupSize[group];

  QString path = findIconPath(name, size);
  if(path.isEmpty()) return QPixmap();

  // load the icon
  return QPixmap(path);
}

QString IconLoader::findIconPath(const QString &name, int size) {
  QString fname = QString("%1.png").arg(name);  // we only support PNG so far
  // First, look for a themed icon... we don't do anything fancy here, only exact match for both name and size
  foreach(QString basedir, _themedIconDirNames) {
    QDir sizedir(QString("%1/%2x%2").arg(basedir).arg(QString::number(size)));
    if(sizedir.exists()) {
      // ignore context, i.e. scan all subdirs
      QStringList contextdirs = sizedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
      foreach(QString dir, contextdirs) {
        QString path = QString("%1/%2/%3").arg(sizedir.absolutePath(), dir, fname);
        if(QFile::exists(path)) return path;
      }
    }
  }
  // Now check the plain dirs
  foreach(QString dir, _plainIconDirNames) {
    QString path = QString("%1/%2").arg(dir, name);
    if(QFile::exists(path)) return path;
  }

  qWarning() << "Icon not found:" << name << size;
  return QString();
}

// Convenience constructors

QPixmap DesktopIcon(const QString& name, int force_size) {
  IconLoader *loader = IconLoader::global();
  return loader->loadIcon(name, IconLoader::Desktop, force_size);
}

QPixmap BarIcon(const QString& name, int force_size) {
  IconLoader *loader = IconLoader::global();
  return loader->loadIcon(name, IconLoader::Toolbar, force_size);
}

QPixmap MainBarIcon(const QString& name, int force_size) {
  IconLoader *loader = IconLoader::global();
  return loader->loadIcon(name, IconLoader::MainToolbar, force_size);
}

QPixmap SmallIcon(const QString& name, int force_size) {
  IconLoader *loader = IconLoader::global();
  return loader->loadIcon(name, IconLoader::Small, force_size);
}

#endif
