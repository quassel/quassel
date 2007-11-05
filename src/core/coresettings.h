/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef CORESETTINGS_H_
#define CORESETTINGS_H_

#include "settings.h"
#include "global.h"

class CoreSettings : public Settings {
  Q_OBJECT
  
  public:
    virtual ~CoreSettings();
    CoreSettings();
    
    void setDatabaseSettings(const QVariant &data);
    QVariant databaseSettings(const QVariant &def = QVariant());
    
    void setPort(const uint &port);
    uint port(const uint &def = DEFAULT_PORT);
        
  private:
    //virtual QStringList allSessionKeys() = 0;
    virtual QStringList sessionKeys();
    
    virtual void setSessionValue(const QString &key, const QVariant &data);
    virtual QVariant sessionValue(const QString &key, const QVariant &def = QVariant());
};

#endif /*CORESETTINGS_H_*/
