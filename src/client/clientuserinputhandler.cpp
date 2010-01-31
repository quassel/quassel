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

#include "buffermodel.h"
#include "client.h"
#include "clientaliasmanager.h"
#include "clientuserinputhandler.h"
#include "clientsettings.h"
#include "execwrapper.h"
#include "ircuser.h"
#include "network.h"

ClientUserInputHandler::ClientUserInputHandler(QObject *parent)
: QObject(parent)
{
  TabCompletionSettings s;
  s.notify("CompletionSuffix", this, SLOT(completionSuffixChanged(QVariant)));
  completionSuffixChanged(s.completionSuffix());
}

void ClientUserInputHandler::completionSuffixChanged(const QVariant &v) {
  QString suffix = v.toString();
  QString letter = "A-Za-z";
  QString special = "\x5b-\x60\x7b-\x7d";
  _nickRx = QRegExp(QString("^([%1%2][%1%2\\d-]*)%3").arg(letter, special, suffix).trimmed());
}

// this would be the place for a client-side hook
void ClientUserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg) {

  if(!msg.startsWith('/')) {
    if(_nickRx.indexIn(msg) == 0) {
      const Network *net = Client::network(bufferInfo.networkId());
      IrcUser *user = net ? net->ircUser(_nickRx.cap(1)) : 0;
      if(user)
        user->setLastSpokenTo(bufferInfo.bufferId(), QDateTime::currentDateTime().toUTC());
    }
  }

  AliasManager::CommandList clist = Client::aliasManager()->processInput(bufferInfo, msg);

  for(int i = 0; i < clist.count(); i++) {
    QString cmd = clist.at(i).second.section(' ', 0, 0).remove(0, 1).toUpper();
    QString args = clist.at(i).second.section(' ', 1);
    if(cmd == "EXEC")
      handleExec(clist.at(i).first, args);
    else {
      if(cmd == "JOIN" || cmd == "QUERY") {
        BufferId newBufId = Client::networkModel()->bufferId(bufferInfo.networkId(), args.section(' ', 0, 0));
        if(!newBufId.isValid()) {
          Client::bufferModel()->switchToBufferAfterCreation(bufferInfo.networkId(), args.section(' ', 0, 0));
        }
        else {
          Client::bufferModel()->switchToBuffer(newBufId);
        }
      }
      emit sendInput(clist.at(i).first, clist.at(i).second);
    }
  }
}

void ClientUserInputHandler::handleExec(const BufferInfo &bufferInfo, const QString &execString) {
  ExecWrapper *exec = new ExecWrapper(this); // gets suicidal when it's done
  exec->start(bufferInfo, execString);
}
