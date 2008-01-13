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

class NetworkConnection;
class Network;

class BasicHandler : public QObject {
  Q_OBJECT

public:
  BasicHandler(NetworkConnection *parent = 0);

  QStringList providesHandlers();

signals:
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void putCmd(QString cmd, QStringList params, QString prefix = 0);
  void putRawLine(QString msg);
  
protected:
  virtual void handle(const QString &member, QGenericArgument val0 = QGenericArgument(0),
		      QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
		      QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
		      QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
		      QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument());
	    
  NetworkConnection *server;

  Network *network() const;

private:
  const QHash<QString, int> &handlerHash();
  QHash<QString, int> _handlerHash;
  int defaultHandler;
  bool initDone;
};
#endif
