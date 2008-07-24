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

#include "util.h"
#include "logger.h"

BasicHandler::BasicHandler(NetworkConnection *parent)
  : QObject(parent),
    defaultHandler(-1),
    _networkConnection(parent),
    initDone(false)
{
  connect(this, SIGNAL(displayMsg(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)),
         networkConnection(), SIGNAL(displayMsg(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)));

  connect(this, SIGNAL(putCmd(QString, const QList<QByteArray> &, const QByteArray &)),
	  networkConnection(), SLOT(putCmd(QString, const QList<QByteArray> &, const QByteArray &)));

  connect(this, SIGNAL(putRawLine(const QByteArray &)),
          networkConnection(), SLOT(putRawLine(const QByteArray &)));
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
      quWarning() << QString("No such Handler: %1::handle%2").arg(metaObject()->className(), handler);
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

QString BasicHandler::serverDecode(const QByteArray &string) {
  return networkConnection()->serverDecode(string);
}

QStringList BasicHandler::serverDecode(const QList<QByteArray> &stringlist) {
  QStringList list;
  foreach(QByteArray s, stringlist) list << networkConnection()->serverDecode(s);
  return list;
}

QString BasicHandler::channelDecode(const QString &bufferName, const QByteArray &string) {
  return networkConnection()->channelDecode(bufferName, string);
}

QStringList BasicHandler::channelDecode(const QString &bufferName, const QList<QByteArray> &stringlist) {
  QStringList list;
  foreach(QByteArray s, stringlist) list << networkConnection()->channelDecode(bufferName, s);
  return list;
}

QString BasicHandler::userDecode(const QString &userNick, const QByteArray &string) {
  return networkConnection()->userDecode(userNick, string);
}

QStringList BasicHandler::userDecode(const QString &userNick, const QList<QByteArray> &stringlist) {
  QStringList list;
  foreach(QByteArray s, stringlist) list << networkConnection()->userDecode(userNick, s);
  return list;
}

/*** ***/

QByteArray BasicHandler::serverEncode(const QString &string) {
  return networkConnection()->serverEncode(string);
}

QList<QByteArray> BasicHandler::serverEncode(const QStringList &stringlist) {
  QList<QByteArray> list;
  foreach(QString s, stringlist) list << networkConnection()->serverEncode(s);
  return list;
}

QByteArray BasicHandler::channelEncode(const QString &bufferName, const QString &string) {
  return networkConnection()->channelEncode(bufferName, string);
}

QList<QByteArray> BasicHandler::channelEncode(const QString &bufferName, const QStringList &stringlist) {
  QList<QByteArray> list;
  foreach(QString s, stringlist) list << networkConnection()->channelEncode(bufferName, s);
  return list;
}

QByteArray BasicHandler::userEncode(const QString &userNick, const QString &string) {
  return networkConnection()->userEncode(userNick, string);
}

QList<QByteArray> BasicHandler::userEncode(const QString &userNick, const QStringList &stringlist) {
  QList<QByteArray> list;
  foreach(QString s, stringlist) list << networkConnection()->userEncode(userNick, s);
  return list;
}

// ====================
//  protected:
// ====================
BufferInfo::Type BasicHandler::typeByTarget(const QString &target) const {
  if(target.isEmpty())
    return BufferInfo::StatusBuffer;

  if(network()->isChannelName(target))
    return BufferInfo::ChannelBuffer;

  return BufferInfo::QueryBuffer;
}

void BasicHandler::putCmd(const QString &cmd, const QByteArray &param, const QByteArray &prefix) {
  QList<QByteArray> list;
  list << param;
  emit putCmd(cmd, list, prefix);
}

void BasicHandler::displayMsg(Message::Type msgType, QString target, QString text, QString sender, Message::Flags flags) {
  IrcChannel *channel = network()->ircChannel(target);
  if(!channel && (target.startsWith('$') || target.startsWith('#')))
    target = nickFromMask(sender);

  emit displayMsg(msgType, typeByTarget(target), target, text, sender, flags);
}
