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

#include <QDateTime>

#include "client.h"
#include "clientuserinputhandler.h"
#include "clientsettings.h"
#include "ircuser.h"
#include "network.h"

ClientUserInputHandler::ClientUserInputHandler(QObject *parent)
: QObject(parent),
  _initialized(false)
{
  TabCompletionSettings s;
  s.notify("CompletionSuffix", this, SLOT(completionSuffixChanged(QVariant)));
  completionSuffixChanged(s.completionSuffix());

  // we need this signal for future connects to reset the data;
  connect(Client::instance(), SIGNAL(connected()), SLOT(clientConnected()));
  connect(Client::instance(), SIGNAL(disconnected()), SLOT(clientDisconnected()));
  if(Client::isConnected())
    clientConnected();
}

void ClientUserInputHandler::clientConnected() {
  _aliasManager = ClientAliasManager();
  Client::signalProxy()->synchronize(&_aliasManager);
  connect(&_aliasManager, SIGNAL(initDone()), SLOT(initDone()));
}

void ClientUserInputHandler::clientDisconnected() {
  // clear alias manager
  _aliasManager = ClientAliasManager();
  _initialized = false;
}

void ClientUserInputHandler::initDone() {
  _initialized = true;
  for(int i = 0; i < _inputBuffer.count(); i++)
    handleUserInput(_inputBuffer.at(i).first, _inputBuffer.at(i).second);
  _inputBuffer.clear();
}

void ClientUserInputHandler::completionSuffixChanged(const QVariant &v) {
  QString suffix = v.toString();
  QString letter = "A-Za-z";
  QString special = "\x5b-\x60\x7b-\x7d";
  _nickRx = QRegExp(QString("^([%1%2][%1%2\\d-]*)%3").arg(letter, special, suffix).trimmed());
}

// this would be the place for a client-side hook
void ClientUserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg) {
  if(!_initialized) { // aliases not yet synced
    _inputBuffer.append(qMakePair(bufferInfo, msg));
    return;
  }

  if(!msg.startsWith('/')) {
    if(_nickRx.indexIn(msg) == 0) {
      const Network *net = Client::network(bufferInfo.networkId());
      IrcUser *user = net ? net->ircUser(_nickRx.cap(1)) : 0;
      if(user)
        user->setLastSpokenTo(bufferInfo.bufferId(), QDateTime::currentDateTime().toUTC());
    }
  }

  AliasManager::CommandList clist = _aliasManager.processInput(bufferInfo, msg);

  for(int i = 0; i < clist.count(); i++)
    emit sendInput(clist.at(i).first, clist.at(i).second);
}
