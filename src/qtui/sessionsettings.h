/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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


#ifndef _SESSIONSETTINGS_H_
#define _SESSIONSETTINGS_H_

#include <QVariant>

#include "clientsettings.h"

class SessionSettings : public ClientSettings {

  public: 
    SessionSettings(const QString &sessionId, const QString &group = "Session");
   
    void setValue(const QString &key, const QVariant &data);
    QVariant value(const QString &key, const QVariant &def = QVariant());
    
    void removeKey(const QString &key);
    void removeSession();
    
    void cleanup();
    void sessionAging();
    
    int sessionAge();
    void setSessionAge(int age);
    inline const QString sessionId() { return _sessionId; };
    inline void setSessionId(const QString &sessionId) { _sessionId = sessionId; }
  private:
    QString _sessionId;
}; 

#endif
