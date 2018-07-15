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

#include "monoapplication.h"
#include "coreapplication.h"
#include "client.h"
#include "core.h"
#include "internalpeer.h"
#include "qtui.h"

class InternalPeer;

MonolithicApplication::MonolithicApplication(int &argc, char **argv)
    : QtUiApplication(argc, argv)
{
#if defined(HAVE_KDE4) || defined(Q_OS_MAC)
    Quassel::disableCrashHandler();
#endif /* HAVE_KDE4 || Q_OS_MAC */

    Quassel::setRunMode(Quassel::Monolithic);
}


void MonolithicApplication::init()
{
    QtUiApplication::init();

    connect(Client::coreConnection(), SIGNAL(connectToInternalCore(QPointer<InternalPeer>)), this, SLOT(onConnectionRequest(QPointer<InternalPeer>)));

    // If port is set, start internal core directly so external clients can connect
    // This is useful in case the mono client re-gains remote connection capability,
    // in which case the internal core would not have to be started by default.
    if (Quassel::isOptionSet("port")) {
        startInternalCore();
    }
}


MonolithicApplication::~MonolithicApplication()
{
    // Client needs to be destroyed first
    Client::destroy();
    _coreThread.quit();
    _coreThread.wait();
    Quassel::destroy();
}


void MonolithicApplication::startInternalCore()
{
    if (_core) {
        // Already started
        return;
    }

    // Start internal core in a separate thread, so it doesn't block the UI
    _core = new Core{};
    _core->moveToThread(&_coreThread);
    connect(&_coreThread, SIGNAL(started()), _core, SLOT(initAsync()));
    connect(&_coreThread, SIGNAL(finished()), _core, SLOT(deleteLater()));

    connect(this, SIGNAL(connectInternalPeer(QPointer<InternalPeer>)), _core, SLOT(connectInternalPeer(QPointer<InternalPeer>)));
    connect(_core, SIGNAL(sessionState(Protocol::SessionState)), Client::coreConnection(), SLOT(internalSessionStateReceived(Protocol::SessionState)));

    connect(_core, SIGNAL(dbUpgradeInProgress(bool)), Client::instance(), SLOT(onDbUpgradeInProgress(bool)));
    connect(_core, SIGNAL(exitRequested(int,QString)), Client::instance(), SLOT(onExitRequested(int,QString)));

    _coreThread.start();
}


void MonolithicApplication::onConnectionRequest(QPointer<InternalPeer> peer)
{
    if (!_core) {
        startInternalCore();
    }

    // While starting the core may take a while, the object itself is instantiated synchronously and the connections
    // established, so it's safe to emit this immediately. The core will take care of queueing the request until
    // initialization is complete.
    emit connectInternalPeer(peer);
}
