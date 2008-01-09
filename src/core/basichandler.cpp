/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "networkconnection.h"

BasicHandler::BasicHandler(NetworkConnection *parent)
  : QObject(parent),
    server(parent) {
  
  connect(this, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)),
	  server, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)));

  connect(this, SIGNAL(putCmd(QString, QStringList, QString)),
	  server, SLOT(putCmd(QString, QStringList, QString)));

  connect(this, SIGNAL(putRawLine(QString)),
	  server, SLOT(putRawLine(QString)));
}

QStringList BasicHandler::providesHandlers() const {
  QStringList handlers;
  for(int i=0; i < metaObject()->methodCount(); i++) {
    QString methodSignature(metaObject()->method(i).signature());
    if(!methodSignature.startsWith("handle"))
      continue;

    methodSignature = methodSignature.section('(',0,0);  // chop the attribute list
    methodSignature = methodSignature.mid(6); // strip "handle"
    handlers << methodSignature;
  }
  return handlers;
}


void BasicHandler::handle(const QString &member, const QGenericArgument &val0,
 			  const QGenericArgument &val1, const QGenericArgument &val2,
 			  const QGenericArgument &val3, const QGenericArgument &val4,
 			  const QGenericArgument &val5, const QGenericArgument &val6,
 			  const QGenericArgument &val7, const QGenericArgument &val8) {

  // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
  QString handler = member.toLower();
  handler[0] = handler[0].toUpper();
  handler = "handle" + handler;

  if(!QMetaObject::invokeMethod(this, handler.toAscii(), val0, val1, val2, val3, val4, val5, val6, val7, val8))
    // Ok. Default handler it is.
    QMetaObject::invokeMethod(this, "defaultHandler", Q_ARG(QString, member), val0, val1, val2, val3, val4, val5, val6, val7, val8);
     
}

// ====================
//  protected:
// ====================
Network *BasicHandler::network() const {
  return server->network();
}
