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

EventManager::EventManager(QObject *parent) : QObject(parent) {

}

EventManager::~EventManager() {
  // pending events won't be delivered anymore, but we do need to delete them
  qDeleteAll(_eventQueue);
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

void EventManager::registerObject(QObject *object, Priority priority, const QString &methodPrefix) {
  for(int i = object->metaObject()->methodOffset(); i < object->metaObject()->methodCount(); i++) {
    QString methodSignature(object->metaObject()->method(i).signature());

    if(!methodSignature.startsWith(methodPrefix))
      continue;

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
          continue;
        }
        eventType += num;
      }
    }

    if(eventType < 0)
      eventType = eventEnum().keyToValue(methodSignature.toAscii());
    if(eventType < 0) {
      qWarning() << Q_FUNC_INFO << "Could not find EventType" << methodSignature;
      continue;
    }
    Handler handler(object, i, priority);
    registeredHandlers()[eventType].append(handler);
    qDebug() << "Registered event handler for" << methodSignature << "in" << object;
  }
}

void EventManager::registerEventHandler(EventType event, QObject *object, const char *slot, Priority priority) {
  registerEventHandler(QList<EventType>() << event, object, slot, priority);
}

void EventManager::registerEventHandler(QList<EventType> events, QObject *object, const char *slot, Priority priority) {
  int methodIndex = object->metaObject()->indexOfMethod(slot);
  if(methodIndex < 0) {
    qWarning() << Q_FUNC_INFO << QString("Slot %1 not found in object %2").arg(slot).arg(object->objectName());
    return;
  }
  Handler handler(object, methodIndex, priority);
  foreach(EventType event, events) {
    registeredHandlers()[event].append(handler);
    qDebug() << "Registered event handler for" << event << "in" << object;
  }
}

// not threadsafe! if we should want that, we need to add a mutexed queue somewhere in this general area.
void EventManager::sendEvent(Event *event) {
  // qDebug() << "Sending" << event;
  _eventQueue.append(event);
  if(_eventQueue.count() == 1) // we're not currently processing another event
    processEvents();
}

void EventManager::customEvent(QEvent *event) {
  if(event->type() == QEvent::User) {
    processEvents();
    event->accept();
  }
}

void EventManager::processEvents() {
  // we only process one event at a time for now, and let Qt's own event processing come in between
  if(_eventQueue.isEmpty())
    return;
  dispatchEvent(_eventQueue.first());
  _eventQueue.removeFirst();
  if(_eventQueue.count())
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

void EventManager::dispatchEvent(Event *event) {
  //qDebug() << "Dispatching" << event;

  // we try handlers from specialized to generic by masking the enum

  // build a list sorted by priorities that contains all eligible handlers
  QList<Handler> handlers;
  uint type = event->type();

  // special handling for numeric IrcEvents
  if((type & ~IrcEventNumericMask) == IrcEventNumeric) {
    ::IrcEventNumeric *numEvent = static_cast< ::IrcEventNumeric *>(event);
    if(!numEvent)
      qWarning() << "Invalid event type for IrcEventNumeric!";
    else {
      int num = numEvent->number();
      if(num > 0)
        insertHandlers(registeredHandlers().value(type + num), handlers);
    }
  }

  // exact type
  insertHandlers(registeredHandlers().value(type), handlers);

  // check if we have a generic handler for the event group
  if((type & EventGroupMask) != type)
    insertHandlers(registeredHandlers().value(type & EventGroupMask), handlers);

  // now dispatch the event
  QList<Handler>::const_iterator it = handlers.begin();
  while(it != handlers.end()) {

    // TODO: check event flags here!

    void *param[] = {0, Q_ARG(Event *, event).data() };
    it->object->qt_metacall(QMetaObject::InvokeMetaMethod, it->methodIndex, param);

    ++it;
  }

  // finally, delete it
  delete event;
}

void EventManager::insertHandlers(const QList<Handler> &newHandlers, QList<Handler> &existing) {
  foreach(Handler handler, newHandlers) {
    if(existing.isEmpty())
      existing.append(handler);
    else {
      // need to insert it at the proper position
      QList<Handler>::iterator it = existing.begin();
      while(it != existing.end()) {
        if(handler.priority > it->priority)
          break;
        ++it;
      }
      existing.insert(it, handler);
    }
  }
}
