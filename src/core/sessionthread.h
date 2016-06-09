/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef SESSIONTHREAD_H
#define SESSIONTHREAD_H

#include <QMutex>
#include <QThread>

#include "types.h"

class CoreSession;
class InternalPeer;
class RemotePeer;
class QIODevice;

class SessionThread : public QThread
{
    Q_OBJECT

public:
    SessionThread(UserId user, bool restoreState, QObject *parent = 0);
    ~SessionThread();

    void run();

    CoreSession *session();
    UserId user();

    /**
     * Request the session disconnect and clean-up.
     *
     * This handles any tasks that require the event-loop.  Non-blocking call; connect to the
     * SessionThread::threadedSessionFinished() signal to receive notice when cleanup is finished.
     */
    void finishThreadedSession();

public slots:
    void addClient(QObject *peer);

private slots:
    void setSessionInitialized();

    /**
     * Signifies that the session hosted by this thread instance has finished cleaning up.
     *
     * Generic event used internally to be able to include UserId in the public signal.
     */
    void sessionFinished();

signals:
    void initialized();
    void shutdown();

    void addRemoteClient(RemotePeer *peer);
    void addInternalClient(InternalPeer *peer);

    /**
     * Request the session disconnect and clean-up.
     *
     * Generic event used internally for easier cross-thread safety.
     */
    void finishSession();

    /**
     * Signifies that the session hosted by this thread instance has finished cleaning up.
     *
     * @param[out] user UserId of the finished session
     */
    void threadedSessionFinished(const UserId user);

private:
    CoreSession *_session;
    UserId _user;
    QList<QObject *> clientQueue;
    bool _sessionInitialized;
    bool _restoreState;

    bool isSessionInitialized();
    void addClientToSession(QObject *peer);
    void addRemoteClientToSession(RemotePeer *remotePeer);
    void addInternalClientToSession(InternalPeer *internalPeer);
};


#endif
