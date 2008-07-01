/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel Project                          *
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

#include "tabcompleter.h"

#include "inputline.h"
#include "client.h"
#include "buffermodel.h"
#include "networkmodel.h"
#include "network.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "uisettings.h"

#include <QRegExp>

TabCompleter::TabCompleter(InputLine *inputLine_)
  : QObject(inputLine_),
    inputLine(inputLine_),
    enabled(false),
    nickSuffix(": ")
{
  inputLine->installEventFilter(this);
}

void TabCompleter::buildCompletionList() {
  completionList.clear();
  nextCompletion = completionList.begin();
  // this is the first time tab is pressed -> build up the completion list and it's iterator
  QModelIndex currentIndex = Client::bufferModel()->currentIndex();
  if(!currentIndex.data(NetworkModel::BufferIdRole).isValid())
    return;
  
  NetworkId networkId = currentIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  QString channelName = currentIndex.sibling(currentIndex.row(), 0).data().toString();

  const Network *network = Client::network(networkId);
  if(!network)
    return;

  IrcChannel *channel = network->ircChannel(channelName);
  if(!channel)
    return;

  // FIXME commented for debugging
  /*
  disconnect(this, SLOT(ircUserJoinedOrParted(IrcUser *)));
  connect(channel, SIGNAL(ircUserJoined(IrcUser *)),
	  this, SLOT(ircUserJoinedOrParted(IrcUser *)));
  connect(channel, SIGNAL(ircUserParted(IrcUser *)),
	  this, SLOT(ircUserJoinedOrParted(IrcUser *)));
  */

  completionList.clear();
  QString tabAbbrev = inputLine->text().left(inputLine->cursorPosition()).section(' ',-1,-1);
  completionList.clear();
  QRegExp regex(QString("^[^a-zA-Z]*").append(tabAbbrev), Qt::CaseInsensitive);
  QMap<QString, QString> sortMap;

  foreach(IrcUser *ircUser, channel->ircUsers()) {
    if(regex.indexIn(ircUser->nick()) > -1) {
      sortMap[ircUser->nick().toLower()] = ircUser->nick();
    }
  }
  foreach (QString str, sortMap)
    completionList << str;

  nextCompletion = completionList.begin();
  lastCompletionLength = tabAbbrev.length();
}

void TabCompleter::ircUserJoinedOrParted(IrcUser *ircUser) {
  Q_UNUSED(ircUser)
  buildCompletionList();
}

void TabCompleter::complete() {
  UiSettings uiSettings;
  nickSuffix = uiSettings.value("CompletionSuffix", QString(": ")).toString();
  
  if(!enabled) {
    buildCompletionList();
    enabled = true;
  }
  
  if (nextCompletion != completionList.end()) {
    // clear previous completion
    for (int i = 0; i < lastCompletionLength; i++) {
      inputLine->backspace();
    }
    
    // insert completion
    inputLine->insert(*nextCompletion);
    
    // remember charcount to delete next time and advance to next completion
    lastCompletionLength = nextCompletion->length();
    nextCompletion++;
    
    // we're completing the first word of the line
    if(inputLine->text().length() == lastCompletionLength) {
      inputLine->insert(nickSuffix);
      lastCompletionLength += nickSuffix.length();
    }

  // we're at the end of the list -> start over again
  } else {
    nextCompletion = completionList.begin();
  }
  
}

void TabCompleter::reset() {
  enabled = false;
}

bool TabCompleter::eventFilter(QObject *obj, QEvent *event) {
  if(obj != inputLine || event->type() != QEvent::KeyPress)
    return QObject::eventFilter(obj, event);

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
  
  if(keyEvent->key() == Qt::Key_Tab) {
    complete();
    return true;
  } else {
    reset();
    return false;
  }
}

