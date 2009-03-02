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
  _aliasManager = AliasManager();
  Client::signalProxy()->synchronize(&_aliasManager);
  connect(&_aliasManager, SIGNAL(initDone()), SLOT(initDone()));
}

void ClientUserInputHandler::clientDisconnected() {
  // clear alias manager
  _aliasManager = AliasManager();
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
void ClientUserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg_) {
  QString msg = msg_;

  if(!_initialized) { // aliases not yet synced
    _inputBuffer.append(qMakePair(bufferInfo, msg));
    return;
  }

  // leading slashes indicate there's a command to call unless there is another one in the first section (like a path /proc/cpuinfo)
  int secondSlashPos = msg.indexOf('/', 1);
  int firstSpacePos = msg.indexOf(' ');
  if(!msg.startsWith('/') || (secondSlashPos != -1 && (secondSlashPos < firstSpacePos || firstSpacePos == -1))) {
    if(msg.startsWith("//"))
      msg.remove(0, 1); // //asdf is transformed to /asdf

    // check if we addressed a user and update its timestamp in that case
    if(bufferInfo.type() == BufferInfo::ChannelBuffer) {
      if(!msg.startsWith('/')) {
        if(_nickRx.indexIn(msg) == 0) {
          const Network *net = Client::network(bufferInfo.networkId());
          IrcUser *user = net ? net->ircUser(_nickRx.cap(1)) : 0;
          if(user)
            user->setLastSpokenTo(bufferInfo.bufferId(), QDateTime::currentDateTime().toUTC());
        }
      }
    }
    msg.prepend("/SAY ");  // make sure we only send proper commands to the core

  } else {
    // check for aliases
    QString cmd = msg.section(' ', 0, 0).remove(0, 1).toUpper();
    for(int i = 0; i < _aliasManager.count(); i++) {
      if(_aliasManager[i].name.toLower() == cmd.toLower()) {
        expand(_aliasManager[i].expansion, bufferInfo, msg.section(' ', 1));
        return;
      }
    }
  }

  // all clear, send off to core.
  emit sendInput(bufferInfo, msg);
}

void ClientUserInputHandler::expand(const QString &alias, const BufferInfo &bufferInfo, const QString &msg) {
  const Network *network = Client::network(bufferInfo.networkId());
  if(!network) {
    // FIXME send error as soon as we have a method for that!
    return;
  }

  QRegExp paramRangeR("\\$(\\d+)\\.\\.(\\d*)");
  QStringList commands = alias.split(QRegExp("; ?"));
  QStringList params = msg.split(' ');
  QStringList expandedCommands;
  for(int i = 0; i < commands.count(); i++) {
    QString command = commands[i];

    // replace ranges like $1..3
    if(!params.isEmpty()) {
      int pos;
      while((pos = paramRangeR.indexIn(command)) != -1) {
        int start = paramRangeR.cap(1).toInt();
        bool ok;
        int end = paramRangeR.cap(2).toInt(&ok);
        if(!ok) {
          end = params.count();
        }
        if(end < start)
          command = command.replace(pos, paramRangeR.matchedLength(), QString());
        else {
          command = command.replace(pos, paramRangeR.matchedLength(), QStringList(params.mid(start - 1, end - start + 1)).join(" "));
        }
      }
    }

    for(int j = params.count(); j > 0; j--) {
      IrcUser *ircUser = network->ircUser(params[j - 1]);
      command = command.replace(QString("$%1:hostname").arg(j), ircUser ? ircUser->host() : QString("*"));
      command = command.replace(QString("$%1").arg(j), params[j - 1]);
    }
    command = command.replace("$0", msg);
    command = command.replace("$channelname", bufferInfo.bufferName());
    command = command.replace("$currentnick", network->myNick());
    expandedCommands << command;
  }

  while(!expandedCommands.isEmpty()) {
    QString command;
    if(expandedCommands[0].trimmed().toLower().startsWith("/wait")) {
      command = expandedCommands.join("; ");
      expandedCommands.clear();
    } else {
      command = expandedCommands.takeFirst();
    }
    handleUserInput(bufferInfo, command);
  }
}
