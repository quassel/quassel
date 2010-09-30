/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#ifndef CORESESSIONEVENTPROCESSOR_H
#define CORESESSIONEVENTPROCESSOR_H

#include <QObject>

class CoreSession;
class IrcEventNumeric;

class CoreSessionEventProcessor : public QObject {
  Q_OBJECT

public:
  CoreSessionEventProcessor(CoreSession *session);

  inline CoreSession *coreSession() const { return _coreSession; }

  Q_INVOKABLE void processIrcEventNumeric(IrcEventNumeric *event);

protected:

private:
  CoreSession *_coreSession;
};

#endif
