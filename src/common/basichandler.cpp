/***************************************************************************
 *   Copyright (C) 2005-10 by the Quassel Project                          *
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
#include "basichandler.h"

#include <QMetaMethod>

#include "logger.h"

BasicHandler::BasicHandler(QObject *parent)
  : QObject(parent),
    defaultHandler(-1),
    initDone(false)
{
}

QStringList BasicHandler::providesHandlers() {
  return handlerHash().keys();
}

const QHash<QString, int> &BasicHandler::handlerHash() {
  if(!initDone) {
    for(int i = metaObject()->methodOffset(); i < metaObject()->methodCount(); i++) {
      QString methodSignature(metaObject()->method(i).signature());
      if(methodSignature.startsWith("defaultHandler")) {
	defaultHandler = i;
	continue;
      }
      
      if(!methodSignature.startsWith("handle"))
	continue;
      
      methodSignature = methodSignature.section('(',0,0);  // chop the attribute list
      methodSignature = methodSignature.mid(6); // strip "handle"
      _handlerHash[methodSignature] = i;
    }
    initDone = true;
  }
  return _handlerHash;
}

void BasicHandler::handle(const QString &member, QGenericArgument val0,
 			  QGenericArgument val1, QGenericArgument val2,
 			  QGenericArgument val3, QGenericArgument val4,
 			  QGenericArgument val5, QGenericArgument val6,
 			  QGenericArgument val7, QGenericArgument val8) {
  // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
  // and now we even have a fast lookup! Thanks thiago!

  QString handler = member.toLower();
  handler[0] = handler[0].toUpper();

  if(!handlerHash().contains(handler)) {
    if(defaultHandler == -1) {
      qWarning() << QString("No such Handler: %1::handle%2").arg(metaObject()->className(), handler);
      return;
    } else {
      void *param[] = {0, Q_ARG(QString, member).data(), val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
		       val5.data(), val6.data(), val7.data(), val8.data(), val8.data()};
      qt_metacall(QMetaObject::InvokeMetaMethod, defaultHandler, param);
      return;
    }
  }

  void *param[] = {0, val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
		   val5.data(), val6.data(), val7.data(), val8.data(), val8.data(), 0};
  qt_metacall(QMetaObject::InvokeMetaMethod, handlerHash()[handler], param);
}
