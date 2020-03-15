/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "sessionthread.h"

#include <QPointer>
#include <QTimer>

#include "core.h"
#include "coresession.h"
#include "internalpeer.h"
#include "remotepeer.h"
#include "signalproxy.h"

namespace {

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker(UserId userId, bool restoreState, bool strictIdentEnabled)
        : _userId{userId}
        , _restoreState{restoreState}
        , _strictIdentEnabled{strictIdentEnabled}
    {}

public slots:
    void initialize()
    {
        _session = new CoreSession{_userId, _restoreState, _strictIdentEnabled, this};
        connect(_session, &QObject::destroyed, QThread::currentThread(), &QThread::quit);
        connect(_session, &CoreSession::sessionStateReceived, Core::instance(), &Core::sessionStateReceived);
        emit initialized();
    }

    void shutdown()
    {
        if (_session) {
            _session->shutdown();
        }
    }

    void addClient(Peer* peer)
    {
        if (!_session) {
            qWarning() << "Session not initialized!";
            return;
        }

        auto remotePeer = qobject_cast<RemotePeer*>(peer);
        if (remotePeer) {
            _session->addClient(remotePeer);
            return;
        }
        auto internalPeer = qobject_cast<InternalPeer*>(peer);
        if (internalPeer) {
            _session->addClient(internalPeer);
            return;
        }

        qWarning() << "SessionThread::addClient() received invalid peer!" << peer;
    }

signals:
    void initialized();

private:
    UserId _userId;
    bool _restoreState;
    bool _strictIdentEnabled;  ///< Whether or not strict ident mode is enabled, locking users' idents to Quassel username
    QPointer<CoreSession> _session;
};

}  // namespace

SessionThread::SessionThread(UserId uid, bool restoreState, bool strictIdentEnabled, QObject* parent)
    : QObject(parent)
{
    auto worker = new Worker(uid, restoreState, strictIdentEnabled);
    worker->moveToThread(&_sessionThread);
    connect(&_sessionThread, &QThread::started, worker, &Worker::initialize);
    connect(&_sessionThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &Worker::initialized, this, &SessionThread::onSessionInitialized);
    connect(worker, &QObject::destroyed, this, &SessionThread::onSessionDestroyed);

    connect(this, &SessionThread::addClientToWorker, worker, &Worker::addClient);
    connect(this, &SessionThread::shutdownSession, worker, &Worker::shutdown);

    // Defer thread start through the event loop, so the SessionThread instance is fully constructed before
    QTimer::singleShot(0, &_sessionThread, SLOT(start()));
}

SessionThread::~SessionThread()
{
    // shut down thread gracefully
    _sessionThread.quit();
    _sessionThread.wait(30000);
}

void SessionThread::shutdown()
{
    emit shutdownSession();
}

void SessionThread::onSessionInitialized()
{
    _sessionInitialized = true;
    for (auto&& peer : _clientQueue) {
        peer->setParent(nullptr);
        peer->moveToThread(&_sessionThread);
        emit addClientToWorker(peer);
    }
    _clientQueue.clear();
}

void SessionThread::onSessionDestroyed()
{
    emit shutdownComplete(this);
}

void SessionThread::addClient(Peer* peer)
{
    if (_sessionInitialized) {
        peer->setParent(nullptr);
        peer->moveToThread(&_sessionThread);
        emit addClientToWorker(peer);
    }
    else {
        _clientQueue.push_back(peer);
    }
}

#include "sessionthread.moc"
