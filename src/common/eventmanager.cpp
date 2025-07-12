/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "eventmanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEvent>
#include <QMetaObject>

#include "event.h"
#include "ircevent.h"
#include "messageevent.h"
#include "ctcpevent.h"
#include "networkevent.h"

// Register meta-types at startup
namespace {
struct MetaTypeRegistrar
{
    MetaTypeRegistrar()
    {
        qRegisterMetaType<NetworkDataEvent*>("NetworkDataEvent*");
        qRegisterMetaType<IrcEventRawMessage*>("IrcEventRawMessage*");
        qRegisterMetaType<::MessageEvent*>("MessageEvent*");
        qRegisterMetaType<::IrcEvent*>("IrcEvent*");
        qRegisterMetaType<::IrcEventNumeric*>("IrcEventNumeric*");
        qRegisterMetaType<::CtcpEvent*>("CtcpEvent*");
    }
};
static MetaTypeRegistrar metaTypeRegistrar;
}  // namespace

// EventTypeRegistry implementation
EventTypeRegistry::EventTypeRegistry()
{
    initializeKnownTypes();
}

void EventTypeRegistry::initializeKnownTypes()
{
    // Register all known event types
    registerEventType<NetworkDataEvent>("NetworkDataEvent*");
    registerEventType<IrcEventRawMessage>("IrcEventRawMessage*");
    registerEventType<MessageEvent>("MessageEvent*");
    registerEventType<IrcEvent>("IrcEvent*");
    registerEventType<IrcEventNumeric>("IrcEventNumeric*");
    registerEventType<CtcpEvent>("CtcpEvent*");
}

bool EventTypeRegistry::dispatchEvent(const QString& expectedEventType, Event* event, const QMetaMethod& method, QObject* obj) const
{
    // Handle base Event type
    if (expectedEventType.isEmpty() || expectedEventType == "Event*") {
        return method.invoke(obj, Qt::DirectConnection, Q_ARG(Event*, event));
    }
    
    // Look up the type handler
    auto it = _typeHandlers.find(expectedEventType);
    if (it == _typeHandlers.end()) {
        qWarning() << Q_FUNC_INFO << "Unknown event type" << expectedEventType << "for handler" << method.methodSignature() << "in" << obj;
        return false;
    }
    
    const TypeHandler& handler = it.value();
    
    // Cast the event
    void* castedEvent = handler.castFunction(event);
    if (!castedEvent) {
        qWarning() << Q_FUNC_INFO << "Event is not a" << expectedEventType << ", actual type:" << EventManager::enumName(event->type()) 
                   << "for handler" << method.methodSignature() << "in" << obj;
        return false;
    }
    
    // Invoke the method with the cast event
    if (!handler.invokeFunction(method, obj, castedEvent)) {
        qWarning() << Q_FUNC_INFO << "Failed to invoke handler" << method.methodSignature() << "in" << obj;
        return false;
    }
    
    return true;
}

// QueuedEvent
class QueuedQuasselEvent : public QEvent
{
public:
    QueuedQuasselEvent(Event* event)
        : QEvent(QEvent::User)
        , event(event)
    {
    }
    Event* event;
};

// EventManager
EventManager::EventManager(QObject* parent)
    : QObject(parent)
{
}

QMetaEnum EventManager::eventEnum()
{
    if (!_enum.isValid()) {
        int eventEnumIndex = staticMetaObject.indexOfEnumerator("EventType");
        Q_ASSERT(eventEnumIndex >= 0);
        _enum = staticMetaObject.enumerator(eventEnumIndex);
        Q_ASSERT(_enum.isValid());
    }
    return _enum;
}

EventManager::EventType EventManager::eventTypeByName(const QString& name)
{
    int val = eventEnum().keyToValue(name.toLatin1());
    if (name == "CtcpEvent") {
        qDebug() << "[DEBUG] eventTypeByName: Looking up" << name << "keyToValue result:" << val << "eventEnum.isValid():" << eventEnum().isValid();
        for (int i = 0; i < eventEnum().keyCount(); ++i) {
            qDebug() << "[DEBUG] eventTypeByName: enum key" << i << ":" << eventEnum().key(i) << "value:" << eventEnum().value(i);
        }
    }
    return (val == -1) ? Invalid : static_cast<EventType>(val);
}

EventManager::EventType EventManager::eventGroupByName(const QString& name)
{
    EventType type = eventTypeByName(name);
    return type == Invalid ? Invalid : static_cast<EventType>(type & EventGroupMask);
}

QString EventManager::enumName(EventType type)
{
    return eventEnum().valueToKey(type);
}

QString EventManager::enumName(int type)
{
    return eventEnum().valueToKey(type);
}

Event* EventManager::createEvent(const QVariantMap& map)
{
    QVariantMap m = map;
    Network* net = networkById(m.take("network").toInt());
    return Event::fromVariantMap(m, net);
}

int EventManager::findEventType(const QString& methodSignature_, const QString& methodPrefix) const
{
    if (!methodSignature_.startsWith(methodPrefix))
        return -1;

    QString methodSignature = methodSignature_;
    methodSignature = methodSignature.section('(', 0, 0);          // chop the attribute list
    methodSignature = methodSignature.mid(methodPrefix.length());  // strip prefix

    int eventType = -1;

    // Special handling for numeric IrcEvents: IrcEvent042 gets mapped to IrcEventNumeric + 42
    if (methodSignature.length() == 8 + 3 && methodSignature.startsWith("IrcEvent")) {
        int num = methodSignature.right(3).toUInt();
        if (num > 0) {
            QString numericSig = methodSignature.left(methodSignature.length() - 3) + "Numeric";
            eventType = eventEnum().keyToValue(numericSig.toLatin1());
            if (eventType < 0) {
                qWarning() << Q_FUNC_INFO << "Could not find EventType" << numericSig << "for handling" << methodSignature;
                return -1;
            }
            eventType += num;
        }
    }

    if (eventType < 0)
        eventType = eventEnum().keyToValue(methodSignature.toLatin1());
    if (eventType < 0) {
        if (methodSignature == "CtcpEvent") {
            qDebug() << "[DEBUG] registerHandler: Failed to find EventType for" << methodSignature;
            qDebug() << "[DEBUG] registerHandler: eventEnum.isValid():" << eventEnum().isValid() << "keyCount:" << eventEnum().keyCount();
            for (int i = 0; i < eventEnum().keyCount(); ++i) {
                qDebug() << "[DEBUG] registerHandler: enum key" << i << ":" << eventEnum().key(i) << "value:" << eventEnum().value(i);
            }
        }
        qWarning() << Q_FUNC_INFO << "Could not find EventType" << methodSignature;
        return -1;
    }
    return eventType;
}

void EventManager::registerObject(QObject* object, Priority priority, const QString& methodPrefix, const QString& filterPrefix)
{
    for (int i = object->metaObject()->methodOffset(); i < object->metaObject()->methodCount(); i++) {
        const QMetaMethod method = object->metaObject()->method(i);
        QString methodSignature = method.methodSignature();
        int eventType = findEventType(methodSignature, methodPrefix);
        if (eventType > 0) {
            Handler handler{object, i, priority, method.parameterTypes().value(0)};
            registeredHandlers()[eventType].append(handler);
            qDebug() << "Registered event handler for" << methodSignature << "in" << object << "expecting" << handler.expectedEventType;
        }
        eventType = findEventType(methodSignature, filterPrefix);
        if (eventType > 0) {
            Handler handler{object, i, priority, method.parameterTypes().value(0)};
            registeredFilters()[eventType].append(handler);
            qDebug() << "Registered event filterer for" << methodSignature << "in" << object << "expecting" << handler.expectedEventType;
        }
    }
}

void EventManager::registerEventFilter(EventType event, QObject* object, const char* slot)
{
    registerEventHandler(QList<EventType>() << event, object, slot, NormalPriority, true);
}

void EventManager::registerEventFilter(QList<EventType> events, QObject* object, const char* slot)
{
    registerEventHandler(events, object, slot, NormalPriority, true);
}

void EventManager::registerEventHandler(EventType event, QObject* object, const char* slot, Priority priority, bool isFilter)
{
    registerEventHandler(QList<EventType>() << event, object, slot, priority, isFilter);
}

void EventManager::registerEventHandler(QList<EventType> events, QObject* object, const char* slot, Priority priority, bool isFilter)
{
    int methodIndex = object->metaObject()->indexOfMethod(slot);
    if (methodIndex < 0) {
        qWarning() << Q_FUNC_INFO << QString("Slot %1 not found in object %2").arg(slot).arg(object->objectName());
        return;
    }
    const QMetaMethod method = object->metaObject()->method(methodIndex);
    Handler handler{object, methodIndex, priority, method.parameterTypes().value(0)};
    for (EventType event : events) {
        if (isFilter) {
            registeredFilters()[event].append(handler);
            qDebug() << "Registered event filter for" << event << "in" << object << "expecting" << handler.expectedEventType;
        }
        else {
            registeredHandlers()[event].append(handler);
            qDebug() << "Registered event handler for" << event << "in" << object << "expecting" << handler.expectedEventType;
        }
    }
}

void EventManager::postEvent(Event* event)
{
    if (!event) {
        qDebug() << Q_FUNC_INFO << "Clearing event queue";
        _eventQueue.clear();
        return;
    }
    qDebug() << Q_FUNC_INFO << "Posting event of type:" << enumName(event->type()) << "at address:" << event;
    if (sender() && sender()->thread() != this->thread()) {
        auto* queuedEvent = new QueuedQuasselEvent(event);
        qDebug() << Q_FUNC_INFO << "Queuing event for cross-thread delivery:" << event;
        QCoreApplication::postEvent(this, queuedEvent);
    }
    else {
        if (_eventQueue.isEmpty())
            processEvent(event);
        else
            _eventQueue.append(event);
    }
}

void EventManager::customEvent(QEvent* event)
{
    if (event->type() == QEvent::User) {
        auto* queuedEvent = static_cast<QueuedQuasselEvent*>(event);
        processEvent(queuedEvent->event);
        event->accept();
    }
}

void EventManager::processEvent(Event* event)
{
    Q_ASSERT(_eventQueue.isEmpty());
    dispatchEvent(event);
    while (!_eventQueue.isEmpty()) {
        dispatchEvent(_eventQueue.first());
        _eventQueue.removeFirst();
    }
}

void EventManager::dispatchEvent(Event* event)
{
    QList<Handler> handlers;
    QHash<QObject*, Handler> filters;
    QSet<QObject*> ignored;
    uint type = event->type();

    bool checkDupes = false;

    if ((type & ~IrcEventNumericMask) == IrcEventNumeric) {
        auto* numEvent = static_cast<::IrcEventNumeric*>(event);
        if (!numEvent)
            qWarning() << "Invalid event type for IrcEventNumeric!";
        else {
            int num = numEvent->number();
            if (num > 0) {
                insertHandlers(registeredHandlers().value(type + num), handlers, false);
                insertFilters(registeredFilters().value(type + num), filters);
                checkDupes = true;
            }
        }
    }

    insertHandlers(registeredHandlers().value(type), handlers, checkDupes);
    insertFilters(registeredFilters().value(type), filters);

    if ((type & EventGroupMask) != type) {
        insertHandlers(registeredHandlers().value(type & EventGroupMask), handlers, true);
        insertFilters(registeredFilters().value(type & EventGroupMask), filters);
    }

    for (const Handler& handler : handlers) {
        if (event->isStopped())
            break;

        QObject* obj = handler.object;
        if (!obj || ignored.contains(obj))
            continue;

        const QMetaMethod method = obj->metaObject()->method(handler.methodIndex);
        if (!method.isValid()) {
            qWarning() << Q_FUNC_INFO << "Invalid method for handler in" << obj;
            continue;
        }

        if (filters.contains(obj)) {
            const Handler& filter = filters.value(obj);
            const QMetaMethod filterMethod = obj->metaObject()->method(filter.methodIndex);
            if (!filterMethod.isValid()) {
                qWarning() << Q_FUNC_INFO << "Invalid filter method in" << obj;
                continue;
            }

            bool result = false;
            if (!filterMethod.invoke(obj, Qt::DirectConnection, Q_RETURN_ARG(bool, result), Q_ARG(Event*, event))) {
                qWarning() << Q_FUNC_INFO << "Failed to invoke filter" << filterMethod.methodSignature() << "in" << obj;
                continue;
            }
            if (!result) {
                ignored.insert(obj);
                continue;
            }
        }

        // Use the registry for dynamic event dispatch
        if (!_eventTypeRegistry.dispatchEvent(handler.expectedEventType, event, method, obj)) {
            continue;
        }
    }

    delete event;
}

void EventManager::insertHandlers(const QList<Handler>& newHandlers, QList<Handler>& existing, bool checkDupes)
{
    for (const Handler& handler : newHandlers) {
        if (existing.isEmpty()) {
            existing.append(handler);
        }
        else {
            bool insert = true;
            QList<Handler>::iterator insertpos = existing.end();
            for (auto it = existing.begin(); it != existing.end(); ++it) {
                if (checkDupes && handler.object == it->object) {
                    insert = false;
                    break;
                }
                if (insertpos == existing.end() && handler.priority > it->priority)
                    insertpos = it;
            }
            if (insert)
                existing.insert(insertpos, handler);
        }
    }
}

void EventManager::insertFilters(const QList<Handler>& newFilters, QHash<QObject*, Handler>& existing)
{
    for (const Handler& filter : newFilters) {
        if (!existing.contains(filter.object))
            existing[filter.object] = filter;
    }
}

QMetaEnum EventManager::_enum;
EventTypeRegistry EventManager::_eventTypeRegistry;

