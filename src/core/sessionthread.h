/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#pragma once

#include <memory>

#include <QThread>

#include "types.h"

class CoreSession;
class Peer;
class InternalPeer;
class RemotePeer;

class SessionThread : public QObject
{
    Q_OBJECT

public:
    SessionThread(UserId user, bool restoreState, bool strictIdentEnabled, QObject *parent = nullptr);
    ~SessionThread() override;

public slots:
    void addClient(Peer *peer);
    void shutdown();

private slots:
    void onSessionInitialized();
    void onSessionDestroyed();

signals:
    void initialized();
    void shutdownSession();
    void shutdownComplete(SessionThread *);

    void addClientToWorker(Peer *peer);

private:
    QThread _sessionThread;
    bool _sessionInitialized{false};

    std::vector<Peer *> _clientQueue;
};
