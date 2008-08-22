/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCoreApplication>
#include <QHash>
#include <QString>
#include <QVariant>

class SettingsChangeNotifier : public QObject {
  Q_OBJECT

signals:
  void valueChanged(const QVariant &newValue);

private:
  friend class Settings;
};



class Settings {
public:
  enum Mode { Default, Custom };

public:
  void notify(const QString &key, QObject *receiver, const char *slot);

protected:
  inline Settings(QString group_, QString appName_) : group(group_), appName(appName_) {}
  inline virtual ~Settings() {}
  
  inline void setGroup(const QString &group_) { group = group_; }
  
  virtual QStringList allLocalKeys();
  virtual QStringList localChildKeys(const QString &rootkey = QString());
  virtual QStringList localChildGroups(const QString &rootkey = QString());
  
  virtual void setLocalValue(const QString &key, const QVariant &data);
  virtual const QVariant &localValue(const QString &key, const QVariant &def = QVariant());
  
  virtual void removeLocalKey(const QString &key);

  QString group;
  QString appName;

private:
  inline QString org() {
#ifdef Q_WS_MAC
    return QCoreApplication::organizationDomain();
#else
    return QCoreApplication::organizationName();
#endif
  }

  static QHash<QString, QHash<QString, QVariant> > settingsCache;
  static QHash<QString, QHash<QString, SettingsChangeNotifier *> > settingsChangeNotifier;

  inline void setCacheValue(const QString &group, const QString &key, const QVariant &data) {
    settingsCache[group][key] = data;
  }
  inline const QVariant &cacheValue(const QString &group, const QString &key) {
    return settingsCache[group][key];
  }
  inline bool isCached(const QString &group, const QString &key) {
    return settingsCache.contains(group) && settingsCache[group].contains(key);
  }

  inline SettingsChangeNotifier *notifier(const QString &group, const QString &key) {
    if(!hasNotifier(group, key))
      settingsChangeNotifier[group][key] = new SettingsChangeNotifier();
    return settingsChangeNotifier[group][key];
  }

  inline bool hasNotifier(const QString &group, const QString &key) {
    return settingsChangeNotifier.contains(group) && settingsChangeNotifier[group].contains(key);
  }
};

#endif
