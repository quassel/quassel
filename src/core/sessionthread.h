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

#ifndef _SESSIONTHREAD_H_
#define _SESSIONTHREAD_H_

#include <QMutex>
#include <QThread>

#include "types.h"

class CoreSession;
class QIODevice;

class SessionThread : public QThread {
  Q_OBJECT

  public:
    SessionThread(UserId user, QObject *parent = 0);
    ~SessionThread();

    void run();

    CoreSession *session();
    UserId user();

  public slots:
    void addClient(QIODevice *socket);

  private slots:
    void setSessionInitialized();

  signals:
    void initialized();

  private:
    CoreSession *_session;
    UserId _user;
    QList<QIODevice *> clientQueue;
    bool _sessionInitialized;

    bool isSessionInitialized();
    void addClientToSession(QIODevice *socket);
};

#endif
