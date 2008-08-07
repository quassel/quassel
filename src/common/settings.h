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

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QString>
#include <QVariant>
#include <QSettings>

class Settings : private QSettings {
public:
  enum Mode { Default, Custom };
  
protected:
  Settings(QString group, QString applicationName);
  
  inline void setGroup(const QString &group_) { group = group_; }
  
  virtual QStringList allLocalKeys();
  virtual QStringList localChildKeys(const QString &rootkey = QString());
  virtual QStringList localChildGroups(const QString &rootkey = QString());
  
  virtual void setLocalValue(const QString &key, const QVariant &data);
  virtual const QVariant &localValue(const QString &key, const QVariant &def = QVariant());
  
  virtual void removeLocalKey(const QString &key);
  
  QString group;

private:
  void setCacheValue(const QString &group, const QString &key, const QVariant &data);
  const QVariant &cacheValue(const QString &group, const QString &key);
  bool isCached(const QString &group, const QString &key);
};



#endif
