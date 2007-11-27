/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
    virtual ~Settings();

    static void setGuiValue(QString, QVariant) {};
    static QVariant guiValue(QString, QVariant = QVariant()) { return QVariant(); }
  protected:
    Settings(QString group = "General");

    void setGroup(QString group);

    virtual QStringList allLocalKeys();
    virtual QStringList localChildKeys();
    virtual QStringList localChildGroups();
    //virtual QStringList allSessionKeys() = 0;
    virtual QStringList sessionKeys() = 0;

    virtual void setLocalValue(const QString &key, const QVariant &data);
    virtual QVariant localValue(const QString &key, const QVariant &def = QVariant());

    virtual void setSessionValue(const QString &key, const QVariant &data) = 0;
    virtual QVariant sessionValue(const QString &key, const QVariant &def = QVariant()) = 0;

    virtual void removeLocalKey(const QString &key);

    QString group;

};



#endif
