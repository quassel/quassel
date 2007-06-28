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

class Settings {

  public:
    //Settings();
    //~Settings();
    static void init();
    static void setProfile(const QString &string);
    static QString profile();

    static void setGuiValue(const QString &key, const QVariant &value);
    static QVariant guiValue (const QString &key, const QVariant &defaultValue = QVariant());
    static void setCoreValue(const QString &user, const QString &key, const QVariant &value);
    static QVariant coreValue (const QString &user, const QString& key, const QVariant &defaultValue = QVariant());

  private:
    static QString curProfile;

};

//extern Settings *settings;

#endif
