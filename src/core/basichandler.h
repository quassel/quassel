/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#include <QGenericArgument>

#include "message.h"

class Server;
class NetworkInfo;

class BasicHandler : public QObject {
  Q_OBJECT

public:
  BasicHandler(Server *parent = 0);

  QStringList providesHandlers() const;

signals:
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void putCmd(QString cmd, QStringList params, QString prefix = 0);
  void putRawLine(QString msg);
  
protected:
  virtual void handle(const QString &member, const QGenericArgument &val0 = QGenericArgument(0),
		      const QGenericArgument &val1 = QGenericArgument(), const QGenericArgument &val2 = QGenericArgument(),
		      const QGenericArgument &val3 = QGenericArgument(), const QGenericArgument &val4 = QGenericArgument(),
		      const QGenericArgument &val5 = QGenericArgument(), const QGenericArgument &val6 = QGenericArgument(),
		      const QGenericArgument &val7 = QGenericArgument(), const QGenericArgument &val8 = QGenericArgument());
	    
  Server *server;
  

protected:
  NetworkInfo *networkInfo() const;

};
#endif
