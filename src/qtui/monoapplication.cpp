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


bool MonolithicApplication::init()
{
    if (!QtUiApplication::init())
        return false;

    connect(Client::coreConnection(), SIGNAL(startInternalCore()), SLOT(startInternalCore()));

    return true;
}


MonolithicApplication::~MonolithicApplication()
{
    // Client needs to be destroyed first
    Client::destroy();
    _core.reset();
    Quassel::destroy();
}


void MonolithicApplication::startInternalCore()
{
    if (!_core) {
        _core.reset(new Core{});  // FIXME C++14: std::make_unique
        Core::instance()->init();
    }
    connect(Client::coreConnection(), SIGNAL(connectToInternalCore(InternalPeer*)), Core::instance(), SLOT(setupInternalClientSession(InternalPeer*)));
    connect(Core::instance(), SIGNAL(sessionState(Protocol::SessionState)), Client::coreConnection(), SLOT(internalSessionStateReceived(Protocol::SessionState)));
}
