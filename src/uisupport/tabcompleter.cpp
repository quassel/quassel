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
  // ensure a safe state in case we return early.
  completionMap.clear();
  nextCompletion = completionMap.begin();

  // this is the first time tab is pressed -> build up the completion list and it's iterator
  QModelIndex currentIndex = Client::bufferModel()->currentIndex();
  if(!currentIndex.data(NetworkModel::BufferIdRole).isValid())
    return;
  
  NetworkId networkId = currentIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  QString bufferName = currentIndex.sibling(currentIndex.row(), 0).data().toString();

  const Network *network = Client::network(networkId);
  if(!network)
    return;


  QString tabAbbrev = inputLine->text().left(inputLine->cursorPosition()).section(' ',-1,-1);
  QRegExp regex(QString("^[^a-zA-Z]*").append(QRegExp::escape(tabAbbrev)), Qt::CaseInsensitive);

  switch(static_cast<BufferInfo::Type>(currentIndex.data(NetworkModel::BufferTypeRole).toInt())) {
  case BufferInfo::ChannelBuffer:
    { // scope is needed for local var declaration
      IrcChannel *channel = network->ircChannel(bufferName);
      if(!channel)
	return;
      foreach(IrcUser *ircUser, channel->ircUsers()) {
	if(regex.indexIn(ircUser->nick()) > -1)
	  completionMap[ircUser->nick().toLower()] = ircUser->nick();
      }
    }
    break;
  case BufferInfo::QueryBuffer:
    if(regex.indexIn(bufferName) > -1)
      completionMap[bufferName.toLower()] = bufferName;
  case BufferInfo::StatusBuffer:
    if(!network->myNick().isEmpty() && regex.indexIn(network->myNick()) > -1)
      completionMap[network->myNick().toLower()] = network->myNick();
    break;
  default:
    return;
  }
  
  nextCompletion = completionMap.begin();
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
  
  if (nextCompletion != completionMap.end()) {
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
    if(inputLine->cursorPosition() == lastCompletionLength) {
      inputLine->insert(nickSuffix);
      lastCompletionLength += nickSuffix.length();
    }

  // we're at the end of the list -> start over again
  } else {
    if(!completionMap.isEmpty()) {
      nextCompletion = completionMap.begin();
      complete();
    }
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

