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

#include "basichandler.h"
#include "ircevent.h"
#include "message.h"

class CoreSession;
class CtcpEvent;
class MessageEvent;

//! Generates user-visible MessageEvents from incoming IrcEvents

/* replaces the string-generating parts of the old IrcServerHandler */
class EventStringifier : public BasicHandler {
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

  // legacy handlers
  Q_INVOKABLE void processNetworkSplitJoin(NetworkSplitEvent *event);
  Q_INVOKABLE void processNetworkSplitQuit(NetworkSplitEvent *event);

  //! Handle generic numeric events
  Q_INVOKABLE void processIrcEventNumeric(IrcEventNumeric *event);

  Q_INVOKABLE void processIrcEventInvite(IrcEvent *event);
  Q_INVOKABLE void processIrcEventJoin(IrcEvent *event);
  Q_INVOKABLE void processIrcEventKick(IrcEvent *event);
  Q_INVOKABLE void processIrcEventMode(IrcEvent *event);
  Q_INVOKABLE void processIrcEventNick(IrcEvent *event);
  Q_INVOKABLE void processIrcEventPart(IrcEvent *event);
  Q_INVOKABLE void processIrcEventPong(IrcEvent *event);
  Q_INVOKABLE void processIrcEventQuit(IrcEvent *event);
  Q_INVOKABLE void processIrcEventTopic(IrcEvent *event);

  Q_INVOKABLE void processIrcEvent005(IrcEvent *event);      // RPL_ISUPPORT
  Q_INVOKABLE void processIrcEvent301(IrcEvent *event);      // RPL_AWAY
  Q_INVOKABLE void processIrcEvent305(IrcEvent *event);      // RPL_UNAWAY
  Q_INVOKABLE void processIrcEvent306(IrcEvent *event);      // RPL_NOWAWAY
  Q_INVOKABLE void processIrcEvent311(IrcEvent *event);      // RPL_WHOISUSER
  Q_INVOKABLE void processIrcEvent312(IrcEvent *event);      // RPL_WHOISSERVER
  Q_INVOKABLE void processIrcEvent314(IrcEvent *event);      // RPL_WHOWASUSER
  Q_INVOKABLE void processIrcEvent315(IrcEvent *event);      // RPL_ENDOFWHO
  Q_INVOKABLE void processIrcEvent317(IrcEvent *event);      // RPL_WHOISIDLE
  Q_INVOKABLE void processIrcEvent318(IrcEvent *event);      // RPL_ENDOFWHOIS
  Q_INVOKABLE void processIrcEvent319(IrcEvent *event);      // RPL_WHOISCHANNELS
  Q_INVOKABLE void processIrcEvent322(IrcEvent *event);      // RPL_LIST
  Q_INVOKABLE void processIrcEvent323(IrcEvent *event);      // RPL_LISTEND
  Q_INVOKABLE void processIrcEvent324(IrcEvent *event);      // RPL_CHANNELMODEIS
  Q_INVOKABLE void processIrcEvent328(IrcEvent *event);      // RPL_??? (channel creation time)
  Q_INVOKABLE void processIrcEvent329(IrcEvent *event);      // RPL_??? (channel homepage)
  Q_INVOKABLE void processIrcEvent330(IrcEvent *event);      // RPL_WHOISACCOUNT (quakenet/snircd/undernet)
  Q_INVOKABLE void processIrcEvent331(IrcEvent *event);      // RPL_NOTOPIC
  Q_INVOKABLE void processIrcEvent332(IrcEvent *event);      // RPL_TOPIC
  Q_INVOKABLE void processIrcEvent333(IrcEvent *event);      // RPL_??? (topic set by)
  Q_INVOKABLE void processIrcEvent341(IrcEvent *event);      // RPL_INVITING
  Q_INVOKABLE void processIrcEvent352(IrcEvent *event);      // RPL_WHOREPLY
  Q_INVOKABLE void processIrcEvent369(IrcEvent *event);      // RPL_ENDOFWHOWAS
  Q_INVOKABLE void processIrcEvent432(IrcEvent *event);      // ERR_ERRONEUSNICKNAME
  Q_INVOKABLE void processIrcEvent433(IrcEvent *event);      // ERR_NICKNAMEINUSE
  Q_INVOKABLE void processIrcEvent437(IrcEvent *event);      // ERR_UNAVAILRESOURCE

  // Q_INVOKABLE void processIrcEvent(IrcEvent *event);

  /* CTCP handlers */
  Q_INVOKABLE void processCtcpEvent(CtcpEvent *event);

  Q_INVOKABLE void handleCtcpAction(CtcpEvent *event);
  Q_INVOKABLE void handleCtcpPing(CtcpEvent *event);
  Q_INVOKABLE void defaultHandler(const QString &cmd, CtcpEvent *event);

public slots:
  //! Creates and sends a MessageEvent
  void displayMsg(NetworkEvent *event,
                  Message::Type msgType,
                  const QString &msg,
                  const QString &sender = QString(),
                  const QString &target = QString(),
                  Message::Flags msgFlags = Message::None);

signals:
  void newMessageEvent(Event *event);

private:
  bool checkParamCount(IrcEvent *event, int minParams);

  CoreSession *_coreSession;
  bool _whois;

};

#endif
