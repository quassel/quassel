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

#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <QHash>
#include <QObject>

class Event;

class EventManager : public QObject {
  Q_OBJECT
  Q_ENUMS(EventType)

public:

  enum RegistrationMode {
    Prepend,
    Append
  };

  /*

  */
  enum EventType {
    Invalid                     = 0xffffffff,
    GenericEvent                  = 0x000000,

    IrcServerEvent                = 0x010000,
    IrcServerLooking              = 0x010001,
    IrcServerConnecting           = 0x010002,
    IrcServerConnected            = 0x010003,
    IrcServerConnectionFailure    = 0x010004,
    IrcServerDisconnected         = 0x010005,
    IrcServerQuit                 = 0x010006,

    IrcServerIncoming             = 0x010007,

    IrcEvent                      = 0x020000,
    IrcEventCap                   = 0x020001,
    IrcEventCapAuthenticate       = 0x020002,
    IrcEventInvite                = 0x020003,
    IrcEventJoin                  = 0x020004,
    IrcEventKick                  = 0x020005,
    IrcEventMode                  = 0x020006,
    IrcEventNick                  = 0x020007,
    IrcEventNotice                = 0x020008,
    IrcEventPart                  = 0x020009,
    IrcEventPing                  = 0x02000a,
    IrcEventPong                  = 0x02000b,
    IrcEventPrivmsg               = 0x02000c,
    IrcEventQuit                  = 0x02000d,
    IrcEventTopic                 = 0x02000e,

    IrcEventNumeric               = 0x021000 /* needs 1000 (0x03e8) consecutive free values! */
  };

  EventManager(QObject *parent = 0);
  //virtual ~EventManager();

  QStringList providesEnums();

public slots:
  void registerObject(QObject *object, RegistrationMode mode = Append, const QString &methodPrefix = "handle");
  void registerEventHandler(EventType event, QObject *object, const char *slot, RegistrationMode mode = Append);
  void registerEventHandler(QList<EventType> events, QObject *object, const char *slot, RegistrationMode mode = Append);

  //void sendEvent(Event *event);

private:
  struct Handler {
    QObject *object;
    int methodIndex;

    explicit Handler(QObject *obj = 0, int method = 0) {
      object = obj;
      methodIndex = method;
    }
  };

  typedef QHash<EventType, QList<Handler> > HandlerHash;

  inline const HandlerHash &registeredHandlers() const { return _registeredHandlers; }
  inline HandlerHash &registeredHandlers() { return _registeredHandlers; }
  HandlerHash _registeredHandlers;

};

#endif
