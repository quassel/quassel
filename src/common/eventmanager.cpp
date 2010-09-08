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

#include <QDebug>
#include <QMetaEnum>

#include "event.h"

EventManager::EventManager(QObject *parent) : QObject(parent) {

}

void EventManager::registerObject(QObject *object, Priority priority, const QString &methodPrefix) {
  int eventEnumIndex = metaObject()->indexOfEnumerator("EventType");
  Q_ASSERT(eventEnumIndex >= 0);
  QMetaEnum eventEnum = metaObject()->enumerator(eventEnumIndex);

  for(int i = object->metaObject()->methodOffset(); i < object->metaObject()->methodCount(); i++) {
    QString methodSignature(object->metaObject()->method(i).signature());

    if(!methodSignature.startsWith(methodPrefix))
      continue;

    methodSignature = methodSignature.section('(',0,0);  // chop the attribute list
    methodSignature = methodSignature.mid(methodPrefix.length()); // strip prefix

    int eventType = eventEnum.keyToValue(methodSignature.toAscii());
    if(eventType < 0) {
      qWarning() << Q_FUNC_INFO << QString("Could not find EventType %1").arg(methodSignature);
      continue;
    }
    Handler handler(object, i, priority);
    registeredHandlers()[static_cast<EventType>(eventType)].append(handler);
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
  dispatchEvent(event);
}

void EventManager::dispatchEvent(Event *event) {
  // we try handlers from specialized to generic by masking the enum

  // build a list sorted by priorities that contains all eligible handlers
  QList<Handler> handlers;
  EventType type = event->type();
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
