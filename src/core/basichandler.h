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

#ifndef _BASICHANDLER_H_
#define _BASICHANDLER_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QGenericArgument>

#include "message.h"

#include "networkconnection.h"

class CoreSession;

class BasicHandler : public QObject {
  Q_OBJECT

public:
  BasicHandler(NetworkConnection *parent = 0);

  QStringList providesHandlers();

  QString serverDecode(const QByteArray &string);
  QStringList serverDecode(const QList<QByteArray> &stringlist);
  QString channelDecode(const QString &bufferName, const QByteArray &string);
  QStringList channelDecode(const QString &bufferName, const QList<QByteArray> &stringlist);
  QString userDecode(const QString &userNick, const QByteArray &string);
  QStringList userDecode(const QString &userNick, const QList<QByteArray> &stringlist);

  QByteArray serverEncode(const QString &string);
  QList<QByteArray> serverEncode(const QStringList &stringlist);
  QByteArray channelEncode(const QString &bufferName, const QString &string);
  QList<QByteArray> channelEncode(const QString &bufferName, const QStringList &stringlist);
  QByteArray userEncode(const QString &userNick, const QString &string);
  QList<QByteArray> userEncode(const QString &userNick, const QStringList &stringlist);

signals:
  void displayMsg(Message::Type, BufferInfo::Type, QString target, QString text, QString sender = "", Message::Flags flags = Message::None);
  void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray());
  void putRawLine(const QByteArray &msg);

protected:
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", Message::Flags flags = Message::None);
  void putCmd(const QString &cmd, const QByteArray &param, const QByteArray &prefix = QByteArray());

  virtual void handle(const QString &member, QGenericArgument val0 = QGenericArgument(0),
                      QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
                      QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
                      QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
                      QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument());


  inline Network *network() const { return _networkConnection->network(); }
  inline NetworkConnection *networkConnection() const { return _networkConnection; }
  inline CoreSession *coreSession() const { return _networkConnection->coreSession(); }

  BufferInfo::Type typeByTarget(const QString &target) const;

private:
  const QHash<QString, int> &handlerHash();
  QHash<QString, int> _handlerHash;
  int defaultHandler;
  NetworkConnection *_networkConnection;
  bool initDone;
};
#endif
