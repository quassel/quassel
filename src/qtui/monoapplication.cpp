/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "monoapplication.h"
#include "coreapplication.h"
#include "client.h"
#include "clientsyncer.h"
#include "core.h"
#include "qtui.h"

MonolithicApplication::MonolithicApplication(int &argc, char **argv)
  : QtUiApplication(argc, argv),
    _internalInitDone(false)
{
  _internal = new CoreApplicationInternal(); // needed for parser options
  setRunMode(Quassel::Monolithic);
}

bool MonolithicApplication::init() {
  if(!Quassel::init()) // parse args
    return false;

  if(isOptionSet("port")) {
    _internal->init();
    _internalInitDone = true;
  }

  connect(Client::instance(), SIGNAL(newClientSyncer(ClientSyncer *)), this, SLOT(newClientSyncer(ClientSyncer *)));
  return QtUiApplication::init();
}

MonolithicApplication::~MonolithicApplication() {
  // Client needs to be destroyed first
  Client::destroy();
  delete _internal;
}

void MonolithicApplication::newClientSyncer(ClientSyncer *syncer) {
  connect(syncer, SIGNAL(startInternalCore(ClientSyncer *)), this, SLOT(startInternalCore(ClientSyncer *)));
}

void MonolithicApplication::startInternalCore(ClientSyncer *syncer) {
  if(!_internalInitDone) {
    _internal->init();
    _internalInitDone = true;
  }
  Core *core = Core::instance();
  connect(syncer, SIGNAL(connectToInternalCore(SignalProxy *)), core, SLOT(setupInternalClientSession(SignalProxy *)));
  connect(core, SIGNAL(sessionState(const QVariant &)), syncer, SLOT(internalSessionStateReceived(const QVariant &)));
}
