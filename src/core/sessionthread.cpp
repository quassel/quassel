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

#include <QMutexLocker>

#include "sessionthread.h"
#include "signalproxy.h"
#include "coresession.h"
#include "core.h"

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
    QIODevice *socket = qobject_cast<QIODevice *>(peer);
    if (socket) {
        addRemoteClientToSession(socket);
        return;
    }

    SignalProxy *proxy = qobject_cast<SignalProxy *>(peer);
    if (proxy) {
        addInternalClientToSession(proxy);
        return;
    }

    qWarning() << "SessionThread::addClient() received neither QIODevice nor SignalProxy as peer!" << peer;
}


void SessionThread::addRemoteClientToSession(QIODevice *socket)
{
    socket->setParent(0);
    socket->moveToThread(session()->thread());
    emit addRemoteClient(socket);
}


void SessionThread::addInternalClientToSession(SignalProxy *proxy)
{
    emit addInternalClient(proxy);
}


void SessionThread::run()
{
    _session = new CoreSession(user(), _restoreState);
    connect(this, SIGNAL(addRemoteClient(QIODevice *)), _session, SLOT(addClient(QIODevice *)));
    connect(this, SIGNAL(addInternalClient(SignalProxy *)), _session, SLOT(addClient(SignalProxy *)));
    connect(_session, SIGNAL(sessionState(const QVariant &)), Core::instance(), SIGNAL(sessionState(const QVariant &)));
    emit initialized();
    exec();
    delete _session;
}
