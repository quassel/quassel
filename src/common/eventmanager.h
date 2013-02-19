/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <QMetaEnum>

#include "types.h"

class Event;
class Network;

class EventManager : public QObject
{
    Q_OBJECT
    Q_FLAGS(EventFlag EventFlags)
    Q_ENUMS(EventType)

public :

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

    enum EventFlag {
        Self     = 0x01, ///< Self-generated (user input) event
        Fake     = 0x08, ///< Ignore this in CoreSessionEventProcessor
        Netsplit = 0x10, ///< Netsplit join/part, ignore on display
        Backlog  = 0x20,
        Silent   = 0x40, ///< Don't generate a MessageEvent
        Stopped  = 0x80
    };
    Q_DECLARE_FLAGS(EventFlags, EventFlag)

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
        NetworkSplitJoin,
        NetworkSplitQuit,
        NetworkIncoming,

        IrcServerEvent              = 0x00020000,
        IrcServerIncoming,
        IrcServerParseError,

        IrcEvent                    = 0x00030000,
        IrcEventAuthenticate,
        IrcEventCap,
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
        IrcEventWallops,
        IrcEventRawPrivmsg, ///< Undecoded privmsg (still needs CTCP parsing)
        IrcEventRawNotice, ///< Undecoded notice (still needs CTCP parsing)
        IrcEventUnknown, ///< Unknown non-numeric cmd

        IrcEventNumeric             = 0x00031000, /* needs 1000 (0x03e8) consecutive free values! */
        IrcEventNumericMask         = 0x00000fff, /* for checking if an event is numeric */

        MessageEvent                = 0x00040000, ///< Stringified event suitable for converting to Message

        CtcpEvent                   = 0x00050000,
        CtcpEventFlush

#ifdef HAVE_QCA2
        ,KeyEvent                    = 0x00060000
#endif
    };

    EventManager(QObject *parent = 0);

    static EventType eventTypeByName(const QString &name);
    static EventType eventGroupByName(const QString &name);
    static QString enumName(EventType type);
    static QString enumName(int type); // for sanity tests

    Event *createEvent(const QVariantMap &map);

public slots:
    void registerObject(QObject *object, Priority priority = NormalPriority,
        const QString &methodPrefix = "process",
        const QString &filterPrefix = "filter");
    void registerEventHandler(EventType event, QObject *object, const char *slot,
        Priority priority = NormalPriority, bool isFilter = false);
    void registerEventHandler(QList<EventType> events, QObject *object, const char *slot,
        Priority priority = NormalPriority, bool isFilter = false);

    void registerEventFilter(EventType event, QObject *object, const char *slot);
    void registerEventFilter(QList<EventType> events, QObject *object, const char *slot);

    //! Send an event to the registered handlers
    /**
      The EventManager takes ownership of the event and will delete it once it's processed.
      @param event The event to be dispatched
     */
    void postEvent(Event *event);

protected:
    virtual Network *networkById(NetworkId id) const = 0;
    virtual void customEvent(QEvent *event);

private:
    struct Handler {
        QObject *object;
        int methodIndex;
        Priority priority;

        explicit Handler(QObject *obj = 0, int method = 0, Priority prio = NormalPriority)
        {
            object = obj;
            methodIndex = method;
            priority = prio;
        }
    };

    typedef QHash<uint, QList<Handler> > HandlerHash;

    inline const HandlerHash &registeredHandlers() const { return _registeredHandlers; }
    inline HandlerHash &registeredHandlers() { return _registeredHandlers; }

    inline const HandlerHash &registeredFilters() const { return _registeredFilters; }
    inline HandlerHash &registeredFilters() { return _registeredFilters; }

    //! Add handlers to an existing sorted (by priority) handler list
    void insertHandlers(const QList<Handler> &newHandlers, QList<Handler> &existing, bool checkDupes = false);
    //! Add filters to an existing filter hash
    void insertFilters(const QList<Handler> &newFilters, QHash<QObject *, Handler> &existing);

    int findEventType(const QString &methodSignature, const QString &methodPrefix) const;

    void processEvent(Event *event);
    void dispatchEvent(Event *event);

    //! @return the EventType enum
    static QMetaEnum eventEnum();

    HandlerHash _registeredHandlers;
    HandlerHash _registeredFilters;
    QList<Event *> _eventQueue;
    static QMetaEnum _enum;
};


Q_DECLARE_OPERATORS_FOR_FLAGS(EventManager::EventFlags);

#endif
