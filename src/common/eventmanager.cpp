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

#include "eventmanager.h"

#include <QCoreApplication>
#include <QEvent>
#include <QDebug>

#include "event.h"
#include "ircevent.h"

// ============================================================
//  QueuedEvent
// ============================================================
class QueuedQuasselEvent : public QEvent {
public:
  QueuedQuasselEvent(Event *event)
    : QEvent(QEvent::User), event(event) {}
  Event *event;
};

// ============================================================
//  EventManager
// ============================================================
EventManager::EventManager(QObject *parent)
  : QObject(parent) {
}

QMetaEnum EventManager::eventEnum() const {
  if(!_enum.isValid()) {
    int eventEnumIndex = metaObject()->indexOfEnumerator("EventType");
    Q_ASSERT(eventEnumIndex >= 0);
    _enum = metaObject()->enumerator(eventEnumIndex);
    Q_ASSERT(_enum.isValid());
  }
  return _enum;
}

EventManager::EventType EventManager::eventTypeByName(const QString &name) const {
  int val = eventEnum().keyToValue(name.toLatin1());
  return (val == -1) ? Invalid : static_cast<EventType>(val);
}

EventManager::EventType EventManager::eventGroupByName(const QString &name) const {
  EventType type = eventTypeByName(name);
  return type == Invalid? Invalid : static_cast<EventType>(type & EventGroupMask);
}

QString EventManager::enumName(EventType type) const {
  return eventEnum().valueToKey(type);
}

/* NOTE:
   Registering and calling handlers works fine even if they specify a subclass of Event as their parameter.
   However, this most probably is a result from a reinterpret_cast somewhere deep inside Qt, so there is *no*
   type safety. If the event sent is of the wrong class type, you'll get a neat segfault!
   Thus, we need to make sure that events are of the correct class type when sending!

   We might add a registration-time check later, which will require matching the enum base name (e.g. "IrcEvent") with
   the type the handler claims to support. This still won't protect us from someone sending an IrcEvent object
   with an enum type "NetworkIncoming", for example.

   Another way would be to add a check into the various Event subclasses, such that the ctor matches the given event type
   with the actual class. Possibly (optionally) using rtti...
*/

int EventManager::findEventType(const QString &methodSignature_, const QString &methodPrefix) const {
  if(!methodSignature_.startsWith(methodPrefix))
    return -1;

  QString methodSignature = methodSignature_;

  methodSignature = methodSignature.section('(',0,0);  // chop the attribute list
  methodSignature = methodSignature.mid(methodPrefix.length()); // strip prefix

  int eventType = -1;

  // special handling for numeric IrcEvents: IrcEvent042 gets mapped to IrcEventNumeric + 42
  if(methodSignature.length() == 8+3 && methodSignature.startsWith("IrcEvent")) {
    int num = methodSignature.right(3).toUInt();
    if(num > 0) {
      QString numericSig = methodSignature.left(methodSignature.length() - 3) + "Numeric";
      eventType = eventEnum().keyToValue(numericSig.toAscii());
      if(eventType < 0) {
        qWarning() << Q_FUNC_INFO << "Could not find EventType" << numericSig << "for handling" << methodSignature;
        return -1;
      }
      eventType += num;
    }
  }

  if(eventType < 0)
    eventType = eventEnum().keyToValue(methodSignature.toAscii());
  if(eventType < 0) {
    qWarning() << Q_FUNC_INFO << "Could not find EventType" << methodSignature;
    return -1;
  }
  return eventType;
}

void EventManager::registerObject(QObject *object, Priority priority, const QString &methodPrefix, const QString &filterPrefix) {
  for(int i = object->metaObject()->methodOffset(); i < object->metaObject()->methodCount(); i++) {
    QString methodSignature(object->metaObject()->method(i).signature());

    int eventType = findEventType(methodSignature, methodPrefix);
    if(eventType > 0) {
      Handler handler(object, i, priority);
      registeredHandlers()[eventType].append(handler);
      //qDebug() << "Registered event handler for" << methodSignature << "in" << object;
    }
    eventType = findEventType(methodSignature, filterPrefix);
    if(eventType > 0) {
      Handler handler(object, i, priority);
      registeredFilters()[eventType].append(handler);
      //qDebug() << "Registered event filterer for" << methodSignature << "in" << object;
    }
  }
}

void EventManager::registerEventFilter(EventType event, QObject *object, const char *slot) {
  registerEventHandler(QList<EventType>() << event, object, slot, NormalPriority, true);
}

void EventManager::registerEventFilter(QList<EventType> events, QObject *object, const char *slot) {
  registerEventHandler(events, object, slot, NormalPriority, true);
}

void EventManager::registerEventHandler(EventType event, QObject *object, const char *slot, Priority priority, bool isFilter) {
  registerEventHandler(QList<EventType>() << event, object, slot, priority, isFilter);
}

void EventManager::registerEventHandler(QList<EventType> events, QObject *object, const char *slot, Priority priority, bool isFilter) {
  int methodIndex = object->metaObject()->indexOfMethod(slot);
  if(methodIndex < 0) {
    qWarning() << Q_FUNC_INFO << QString("Slot %1 not found in object %2").arg(slot).arg(object->objectName());
    return;
  }
  Handler handler(object, methodIndex, priority);
  foreach(EventType event, events) {
    if(isFilter) {
      registeredFilters()[event].append(handler);
      qDebug() << "Registered event filter for" << event << "in" << object;
    } else {
      registeredHandlers()[event].append(handler);
      qDebug() << "Registered event handler for" << event << "in" << object;
    }
  }
}

void EventManager::postEvent(Event *event) {
  if(sender() && sender()->thread() != this->thread()) {
    QueuedQuasselEvent *queuedEvent = new QueuedQuasselEvent(event);
    QCoreApplication::postEvent(this, queuedEvent);
  } else {
    if(_eventQueue.isEmpty())
      // we're currently not processing events
      processEvent(event);
    else
      _eventQueue.append(event);
  }
}

void EventManager::customEvent(QEvent *event) {
  if(event->type() == QEvent::User) {
    QueuedQuasselEvent *queuedEvent = static_cast<QueuedQuasselEvent *>(event);
    processEvent(queuedEvent->event);
    event->accept();
  }
}

void EventManager::processEvent(Event *event) {
  Q_ASSERT(_eventQueue.isEmpty());
  dispatchEvent(event);
  // dispatching the event might cause new events to be generated. we process those afterwards.
  while(!_eventQueue.isEmpty()) {
    dispatchEvent(_eventQueue.first());
    _eventQueue.removeFirst();
  }
}

void EventManager::dispatchEvent(Event *event) {
  //qDebug() << "Dispatching" << event;

  // we try handlers from specialized to generic by masking the enum

  // build a list sorted by priorities that contains all eligible handlers
  QList<Handler> handlers;
  QHash<QObject *, Handler> filters;
  QSet<QObject *> ignored;
  uint type = event->type();

  bool checkDupes = false;

  // special handling for numeric IrcEvents
  if((type & ~IrcEventNumericMask) == IrcEventNumeric) {
    ::IrcEventNumeric *numEvent = static_cast< ::IrcEventNumeric *>(event);
    if(!numEvent)
      qWarning() << "Invalid event type for IrcEventNumeric!";
    else {
      int num = numEvent->number();
      if(num > 0) {
        insertHandlers(registeredHandlers().value(type + num), handlers, false);
        insertFilters(registeredFilters().value(type + num), filters);
        checkDupes = true;
      }
    }
  }

  // exact type
  insertHandlers(registeredHandlers().value(type), handlers, checkDupes);
  insertFilters(registeredFilters().value(type), filters);

  // check if we have a generic handler for the event group
  if((type & EventGroupMask) != type) {
    insertHandlers(registeredHandlers().value(type & EventGroupMask), handlers, true);
    insertFilters(registeredFilters().value(type & EventGroupMask), filters);
  }

  // now dispatch the event
  QList<Handler>::const_iterator it;
  for(it = handlers.begin(); it != handlers.end() && !event->isStopped(); ++it) {
    QObject *obj = it->object;

    if(ignored.contains(obj)) // object has filtered the event
      continue;

    if(filters.contains(obj)) { // we have a filter, so let's check if we want to deliver the event
      Handler filter = filters.value(obj);
      bool result = false;
      void *param[] = {Q_RETURN_ARG(bool, result).data(), Q_ARG(Event *, event).data() };
      obj->qt_metacall(QMetaObject::InvokeMetaMethod, filter.methodIndex, param);
      if(!result) {
        ignored.insert(obj);
        continue; // mmmh, event filter told us to not accept
      }
    }

    // finally, deliverance!
    void *param[] = {0, Q_ARG(Event *, event).data() };
    obj->qt_metacall(QMetaObject::InvokeMetaMethod, it->methodIndex, param);
  }

  // that's it
  delete event;
}

void EventManager::insertHandlers(const QList<Handler> &newHandlers, QList<Handler> &existing, bool checkDupes) {
  foreach(const Handler &handler, newHandlers) {
    if(existing.isEmpty())
      existing.append(handler);
    else {
      // need to insert it at the proper position, but only if we don't yet have a handler for this event and object!
      bool insert = true;
      QList<Handler>::iterator insertpos = existing.end();
      QList<Handler>::iterator it = existing.begin();
      while(it != existing.end()) {
        if(checkDupes && handler.object == it->object) {
          insert = false;
          break;
        }
        if(insertpos == existing.end() && handler.priority > it->priority)
          insertpos = it;

        ++it;
      }
      if(insert)
        existing.insert(it, handler);
    }
  }
}

// priority is ignored, and only the first (should be most specialized) filter is being used
// fun things could happen if you used the registerEventFilter() methods in the wrong order though
void EventManager::insertFilters(const QList<Handler> &newFilters, QHash<QObject *, Handler> &existing) {
  foreach(const Handler &filter, newFilters) {
    if(!existing.contains(filter.object))
      existing[filter.object] = filter;
  }
}
