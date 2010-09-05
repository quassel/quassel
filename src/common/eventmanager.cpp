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

EventManager::EventManager(QObject *parent) : QObject(parent) {

}

void EventManager::registerObject(QObject *object, RegistrationMode mode, const QString &methodPrefix) {
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
    Handler handler(object, i);
    mode == Prepend ? registeredHandlers()[static_cast<EventType>(eventType)].prepend(handler)
                    : registeredHandlers()[static_cast<EventType>(eventType)].append(handler);

    qDebug() << "Registered event handler for" << methodSignature << "in" << object;
  }
}

void EventManager::registerEventHandler(EventType event, QObject *object, const char *slot, RegistrationMode mode) {
  registerEventHandler(QList<EventType>() << event, object, slot, mode);
}

void EventManager::registerEventHandler(QList<EventType> events, QObject *object, const char *slot, RegistrationMode mode) {
  int methodIndex = object->metaObject()->indexOfMethod(slot);
  if(methodIndex < 0) {
    qWarning() << Q_FUNC_INFO << QString("Slot %1 not found in object %2").arg(slot).arg(object->objectName());
    return;
  }
  Handler handler(object, methodIndex);
  foreach(EventType event, events) {
    mode == Prepend ? registeredHandlers()[event].prepend(handler)
                    : registeredHandlers()[event].append(handler);

    qDebug() << "Registered event handler for" << event << "in" << object;
  }
}
