/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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
#include "qtui.h"

class InternalPeer;

MonolithicApplication::MonolithicApplication(int &argc, char **argv)
    : QtUiApplication(argc, argv),
    _internalInitDone(false)
{
    _internal = new CoreApplicationInternal(); // needed for parser options
#if defined(HAVE_KDE4) || defined(Q_OS_MAC)
    disableCrashhandler();
#endif /* HAVE_KDE4 || Q_OS_MAC */
    setRunMode(Quassel::Monolithic);
}


bool MonolithicApplication::init()
{
    if (!Quassel::init()) // parse args
        return false;

    connect(Client::coreConnection(), SIGNAL(startInternalCore()), SLOT(startInternalCore()));

    // FIXME what's this for?
    if (isOptionSet("port")) {
        startInternalCore();
    }

    return QtUiApplication::init();
}


MonolithicApplication::~MonolithicApplication()
{
    // Client needs to be destroyed first
    Client::destroy();
    delete _internal;
}


void MonolithicApplication::startInternalCore()
{
    if (!_internalInitDone) {
        _internal->init();
        _internalInitDone = true;
    }
    Core *core = Core::instance();
    CoreConnection *connection = Client::coreConnection();
    connect(connection, SIGNAL(connectToInternalCore(InternalPeer*)), core, SLOT(setupInternalClientSession(InternalPeer*)));
    connect(core, SIGNAL(sessionState(Protocol::SessionState)), connection, SLOT(internalSessionStateReceived(Protocol::SessionState)));
}


bool MonolithicApplication::reloadConfig()
{
    if (_internal) {
        return _internal->reloadConfig();
    } else {
        return false;
    }
}
