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

#ifndef _CORESESSION_H_
#define _CORESESSION_H_

#include <QObject>
#include <QString>
#include <QVariant>

#include "message.h"

class Server;
class SignalProxy;
class Storage;

class CoreSession : public QObject {
  Q_OBJECT

public:
  CoreSession(UserId, Storage *, QObject *parent = 0);
  virtual ~CoreSession();

  NetworkId getNetworkId(const QString &network) const;
  QList<BufferInfo> buffers() const;
  UserId userId() const;
  QVariant sessionState();

  //! Retrieve a piece of session-wide data.
  QVariant retrieveSessionData(const QString &key, const QVariant &def = QVariant());

  SignalProxy *signalProxy() const;

  void attachServer(Server *server);

  //! Return necessary data for restoring the session after restarting the core
  QVariant state() const;
  void restoreState(const QVariant &previousState);

public slots:
  //! Store a piece session-wide data and distribute it to connected clients.
  void storeSessionData(const QString &key, const QVariant &data);

  void serverStateRequested();

  void addClient(QIODevice *connection);

  void connectToNetwork(QString, const QVariant &previousState = QVariant());
  //void connectToNetwork(NetworkId);

  //void processSignal(ClientSignal, QVariant, QVariant, QVariant);
  void sendBacklog(BufferInfo, QVariant, QVariant);
  void msgFromGui(BufferInfo, QString message);

signals:
  void msgFromGui(uint netid, QString buf, QString message);
  void displayMsg(Message message);
  void displayStatusMsg(QString, QString);

  void connectToIrc(QString net);
  void disconnectFromIrc(QString net);

  void backlogData(BufferInfo, QVariantList, bool done);

  void bufferInfoUpdated(BufferInfo);
  void sessionDataChanged(const QString &key);
  void sessionDataChanged(const QString &key, const QVariant &data);

private slots:
  void recvStatusMsgFromServer(QString msg);
  void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void serverConnected(uint networkid);
  void serverDisconnected(uint networkid);

private:
  UserId user;
  
  SignalProxy *_signalProxy;
  Storage *storage;
  QHash<NetworkId, Server *> servers;
  
  QVariantMap sessionData;
  QMutex mutex;
};

#endif
