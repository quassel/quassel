/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
#include "internalpeer.h"
#include "remotepeer.h"
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
    RemotePeer *remote = qobject_cast<RemotePeer *>(peer);
    if (remote) {
        addRemoteClientToSession(remote);
        return;
    }

    InternalPeer *internal = qobject_cast<InternalPeer *>(peer);
    if (internal) {
        addInternalClientToSession(internal);
        return;
    }

    qWarning() << "SessionThread::addClient() received invalid peer!" << peer;
}


void SessionThread::addRemoteClientToSession(RemotePeer *remotePeer)
{
    remotePeer->setParent(0);
    remotePeer->moveToThread(session()->thread());
    emit addRemoteClient(remotePeer);
}


void SessionThread::addInternalClientToSession(InternalPeer *internalPeer)
{
    internalPeer->setParent(0);
    internalPeer->moveToThread(session()->thread());
    emit addInternalClient(internalPeer);
}


void SessionThread::run()
{
    _session = new CoreSession(user(), _restoreState);
    connect(this, SIGNAL(addRemoteClient(RemotePeer*)), _session, SLOT(addClient(RemotePeer*)));
    connect(this, SIGNAL(addInternalClient(InternalPeer*)), _session, SLOT(addClient(InternalPeer*)));
    connect(_session, SIGNAL(sessionState(QVariant)), Core::instance(), SIGNAL(sessionState(QVariant)));
    emit initialized();
    exec();
    delete _session;
}
