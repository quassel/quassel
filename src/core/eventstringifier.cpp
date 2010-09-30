/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#include "eventstringifier.h"

#include "coresession.h"
#include "messageevent.h"

EventStringifier::EventStringifier(CoreSession *parent) : QObject(parent),
  _coreSession(parent),
  _whois(false)
{

}

void EventStringifier::displayMsg(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
                                  const QString &target, Message::Flags msgFlags) {
  MessageEvent *msgEvent = createMessageEvent(event, msgType, msg, sender, target, msgFlags);
  sendMessageEvent(msgEvent);
}

MessageEvent *EventStringifier::createMessageEvent(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
                        const QString &target, Message::Flags msgFlags) {
  MessageEvent *msgEvent = new MessageEvent(msgType, event->network(), msg, sender, target, msgFlags);
  return msgEvent;
}

void EventStringifier::sendMessageEvent(MessageEvent *event) { qDebug() << event->text();
  coreSession()->eventManager()->sendEvent(event);
}
