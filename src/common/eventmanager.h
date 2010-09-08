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

  enum Priority {
    VeryLowPriority,
    LowPriority,
    NormalPriority,
    HighPriority,
    HighestPriority
  };

  /*

  */
  /* These values make sense! Don't change without knowing what you do! */
  enum EventType {
    Invalid                     = 0xffffffff,
    GenericEvent                = 0x00000000,

    // for event group handlers (handleIrcEvent() will handle all IrcEvent* enums)
    // event groups are specified by bits 20-24
    EventGroupMask              = 0x00ff0000,

    NetworkEvent                = 0x00010000,
    NetworkConnecting,
    NetworkInitializing,
    NetworkInitialized,
    NetworkReconnecting,
    NetworkDisconnecting,
    NetworkDisconnected,
    NetworkIncoming,

    IrcServerEvent              = 0x00020000,
    IrcServerIncoming,

    IrcEvent                    = 0x00030000,
    IrcEventCap,
    IrcEventCapAuthenticate,
    IrcEventInvite,
    IrcEventJoin,
    IrcEventKick,
    IrcEventMode,
    IrcEventNick,
    IrcEventNotice,
    IrcEventPart,
    IrcEventPing,
    IrcEventPong,
    IrcEventPrivmsg,
    IrcEventQuit,
    IrcEventTopic,

    IrcEventNumeric             = 0x00031000, /* needs 1000 (0x03e8) consecutive free values! */
  };

  EventManager(QObject *parent = 0);
  //virtual ~EventManager();

  QStringList providesEnums();

public slots:
  void registerObject(QObject *object, Priority priority = NormalPriority, const QString &methodPrefix = "handle");
  void registerEventHandler(EventType event, QObject *object, const char *slot, Priority priority = NormalPriority);
  void registerEventHandler(QList<EventType> events, QObject *object, const char *slot, Priority priority = NormalPriority);

  //! Send an event to the registered handlers
  /**
    The EventManager takes ownership of the event and will delete it once it's processed.
    NOTE: This method is not threadsafe!
    @param event The event to be dispatched
   */
  void sendEvent(Event *event);

private:
  struct Handler {
    QObject *object;
    int methodIndex;
    Priority priority;

    explicit Handler(QObject *obj = 0, int method = 0, Priority prio = NormalPriority) {
      object = obj;
      methodIndex = method;
      priority = prio;
    }
  };

  typedef QHash<uint, QList<Handler> > HandlerHash;

  inline const HandlerHash &registeredHandlers() const { return _registeredHandlers; }
  inline HandlerHash &registeredHandlers() { return _registeredHandlers; }

  //! Add handlers to an existing sorted (by priority) handler list
  void insertHandlers(const QList<Handler> &newHandlers, QList<Handler> &existing);

  void dispatchEvent(Event *event);

  HandlerHash _registeredHandlers;

};

#endif
