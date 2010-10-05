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

#ifndef EVENTSTRINGIFIER_H
#define EVENTSTRINGIFIER_H

#include <QObject>

#include "ircevent.h"
#include "message.h"

class CoreSession;
class MessageEvent;

//! Generates user-visible MessageEvents from incoming IrcEvents

/* replaces the string-generating parts of the old IrcServerHandler */
class EventStringifier : public QObject {
  Q_OBJECT

public:
  explicit EventStringifier(CoreSession *parent);

  inline CoreSession *coreSession() const { return _coreSession; }

  MessageEvent *createMessageEvent(NetworkEvent *event,
                                   Message::Type msgType,
                                   const QString &msg,
                                   const QString &sender = QString(),
                                   const QString &target = QString(),
                                   Message::Flags msgFlags = Message::None);

  //! Handle generic numeric events
  Q_INVOKABLE void processIrcEventNumeric(IrcEventNumeric *event);

  Q_INVOKABLE void processIrcEventInvite(IrcEvent *event);
  Q_INVOKABLE void earlyProcessIrcEventKick(IrcEvent *event);
  Q_INVOKABLE void earlyProcessIrcEventNick(IrcEvent *event);
  Q_INVOKABLE void earlyProcessIrcEventPart(IrcEvent *event);
  Q_INVOKABLE void processIrcEventPong(IrcEvent *event);
  Q_INVOKABLE void processIrcEventTopic(IrcEvent *event);

  // Q_INVOKABLE void processIrcEvent(IrcEvent *event);

public slots:
  //! Creates and sends a MessageEvent
  void displayMsg(NetworkEvent *event,
                  Message::Type msgType,
                  const QString &msg,
                  const QString &sender = QString(),
                  const QString &target = QString(),
                  Message::Flags msgFlags = Message::None);
private:
  void sendMessageEvent(MessageEvent *event);

  CoreSession *_coreSession;
  bool _whois;

};

#endif
