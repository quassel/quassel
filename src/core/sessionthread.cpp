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

#include <QMutexLocker>

#include "sessionthread.h"

#include "coresession.h"

SessionThread::SessionThread(UserId uid, bool restoreState, QObject *parent) : QThread(parent) {
    _user = uid;
    _session = 0;
    _sessionInitialized = false;
    _restoreState = restoreState,
    connect(this, SIGNAL(initialized()), this, SLOT(setSessionInitialized()));
}

SessionThread::~SessionThread() {
  // shut down thread gracefully
  quit();
  wait();
}

CoreSession *SessionThread::session() {
  return _session;
}

UserId SessionThread::user() {
  return _user;
}

bool SessionThread::isSessionInitialized() {
  return _sessionInitialized;
}

void SessionThread::setSessionInitialized() {
  _sessionInitialized = true;
  foreach(QIODevice *socket, clientQueue) {
    addClientToSession(socket);
  }
  clientQueue.clear();
}

void SessionThread::addClient(QIODevice *socket) {
  if(isSessionInitialized()) {
    addClientToSession(socket);
  } else {
    clientQueue.append(socket);
  }
}

void SessionThread::addClientToSession(QIODevice *socket) {
  socket->setParent(0);
  socket->moveToThread(session()->thread());
  if(!QMetaObject::invokeMethod(session(), "addClient", Q_ARG(QObject *, socket))) {
    qWarning() << qPrintable(tr("Could not initialize session!"));
    socket->close();
  }
}

void SessionThread::run() {
  _session = new CoreSession(user(), _restoreState);
  emit initialized();
  exec();
  delete _session;
}

