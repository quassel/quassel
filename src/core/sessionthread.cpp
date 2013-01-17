/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "core.h"
#include "coresession.h"
#include "internalconnection.h"
#include "remoteconnection.h"
#include "sessionthread.h"
#include "signalproxy.h"

SessionThread::SessionThread(UserId uid, bool restoreState, QObject *parent)
    : QThread(parent),
    _session(0),
    _user(uid),
    _sessionInitialized(false),
    _restoreState(restoreState)
{
    connect(this, SIGNAL(initialized()), this, SLOT(setSessionInitialized()));
}


SessionThread::~SessionThread()
{
    // shut down thread gracefully
    quit();
    wait();
}


CoreSession *SessionThread::session()
{
    return _session;
}


UserId SessionThread::user()
{
    return _user;
}


bool SessionThread::isSessionInitialized()
{
    return _sessionInitialized;
}


void SessionThread::setSessionInitialized()
{
    _sessionInitialized = true;
    foreach(QObject *peer, clientQueue) {
        addClientToSession(peer);
    }
    clientQueue.clear();
}


// this and the following related methods are executed in the Core thread!
void SessionThread::addClient(QObject *peer)
{
    if (isSessionInitialized()) {
        addClientToSession(peer);
    }
    else {
        clientQueue.append(peer);
    }
}


void SessionThread::addClientToSession(QObject *peer)
{
    RemoteConnection *connection = qobject_cast<RemoteConnection *>(peer);
    if (connection) {
        addRemoteClientToSession(connection);
        return;
    }

    InternalConnection *internal = qobject_cast<InternalConnection *>(peer);
    if (internal) {
        addInternalClientToSession(internal);
        return;
    }

    qWarning() << "SessionThread::addClient() received invalid peer!" << peer;
}


void SessionThread::addRemoteClientToSession(RemoteConnection *connection)
{
    connection->setParent(0);
    connection->moveToThread(session()->thread());
    emit addRemoteClient(connection);
}


void SessionThread::addInternalClientToSession(InternalConnection *connection)
{
    connection->setParent(0);
    connection->moveToThread(session()->thread());
    emit addInternalClient(connection);
}


void SessionThread::run()
{
    _session = new CoreSession(user(), _restoreState);
    connect(this, SIGNAL(addRemoteClient(RemoteConnection*)), _session, SLOT(addClient(RemoteConnection*)));
    connect(this, SIGNAL(addInternalClient(InternalConnection*)), _session, SLOT(addClient(InternalConnection*)));
    connect(_session, SIGNAL(sessionState(QVariant)), Core::instance(), SIGNAL(sessionState(QVariant)));
    emit initialized();
    exec();
    delete _session;
}
