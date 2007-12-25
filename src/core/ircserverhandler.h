/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _IRCSERVERHANDLER_H_
#define _IRCSERVERHANDLER_H_

#include "basichandler.h"

class IrcServerHandler : public BasicHandler {
  Q_OBJECT

public:
  IrcServerHandler(Server *parent = 0);
  ~IrcServerHandler();

  void handleServerMsg(QByteArray rawMsg);

  QString serverDecode(const QByteArray &string);
  QStringList serverDecode(const QList<QByteArray> &stringlist);
  QString bufferDecode(const QString &bufferName, const QByteArray &string);
  QStringList bufferDecode(const QString &bufferName, const QList<QByteArray> &stringlist);
  QString userDecode(const QString &userNick, const QByteArray &string);
  QStringList userDecode(const QString &userNick, const QList<QByteArray> &stringlist);


public slots:
  void handleJoin(QString, QList<QByteArray>);
  void handleKick(QString, QList<QByteArray>);
  void handleMode(QString, QList<QByteArray>);
  void handleNick(QString, QList<QByteArray>);
  void handleNotice(QString, QList<QByteArray>);
  void handlePart(QString, QList<QByteArray>);
  void handlePing(QString, QList<QByteArray>);
  void handlePrivmsg(QString, QList<QByteArray>);
  void handleQuit(QString, QList<QByteArray>);
  void handleTopic(QString, QList<QByteArray>);

  void handle001(QString, QList<QByteArray>);   // RPL_WELCOME
  void handle005(QString, QList<QByteArray>);   // RPL_ISUPPORT
  void handle331(QString, QList<QByteArray>);   // RPL_NOTOPIC
  void handle332(QString, QList<QByteArray>);   // RPL_TOPIC
  void handle333(QString, QList<QByteArray>);   // Topic set by...
  void handle353(QString, QList<QByteArray>);   // RPL_NAMREPLY
  void handle432(QString, QList<QByteArray>);   // ERR_ERRONEUSNICKNAME
  void handle433(QString, QList<QByteArray>);   // ERR_NICKNAMEINUSE

  void defaultHandler(QString cmd, QString prefix, QList<QByteArray> params);

  private:
    Server *server;
};


#endif
