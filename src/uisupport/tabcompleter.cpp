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

TabCompleter::TabCompleter(InputLine *inputLine_)
  : QObject(inputLine_),
    inputLine(inputLine_),
    enabled(false),
    nickSuffix(": ")
{
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

  Network *network = Client::network(networkId);
  if(!network)
    return;

  IrcChannel *channel = network->ircChannel(channelName);
  if(!channel)
    return;

  disconnect(this, SLOT(ircUserJoinedOrParted(IrcUser *)));
  connect(channel, SIGNAL(ircUserJoined(IrcUser *)),
	  this, SLOT(ircUserJoinedOrParted(IrcUser *)));
  connect(channel, SIGNAL(ircUserParted(IrcUser *)),
	  this, SLOT(ircUserJoinedOrParted(IrcUser *)));
	     
  completionList.clear();
  QString tabAbbrev = inputLine->text().left(inputLine->cursorPosition()).section(' ',-1,-1);
  completionList.clear();
  foreach(IrcUser *ircUser, channel->ircUsers()) {
    if(ircUser->nick().toLower().startsWith(tabAbbrev.toLower())) {
      completionList << ircUser->nick();
    }
  }
  completionList.sort();
  nextCompletion = completionList.begin();
  lastCompletionLength = tabAbbrev.length();
}

void TabCompleter::ircUserJoinedOrParted(IrcUser *ircUser) {
  Q_UNUSED(ircUser)
  buildCompletionList();
}

void TabCompleter::complete() {
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

