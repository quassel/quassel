/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef _CORESESSION_H_
#define _CORESESSION_H_

#include <QObject>
#include <QString>
#include <QVariant>

#include "coreproxy.h"

class Server;
class Storage;

class CoreSession : public QObject {
  Q_OBJECT

  public:
    CoreSession(UserId, Storage *);
    ~CoreSession();

    QList<BufferId> buffers() const;
    UserId userId() const;
    QVariant sessionState();

  public slots:
    //! Store a piece session-wide data and distribute it to connected clients.
    void storeSessionData(const QString &key, const QVariant &data);

  public:
    //! Retrieve a piece of session-wide data.
    QVariant retrieveSessionData(const QString &key, const QVariant &def = QVariant());

    CoreProxy *proxy();

  public slots:
    void connectToIrc(QStringList);
    void processSignal(ClientSignal, QVariant, QVariant, QVariant);
    void sendBacklog(BufferId, QVariant, QVariant);
    void msgFromGui(BufferId, QString message);
    void sendServerStates();

  signals:
    void proxySignal(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

    void msgFromGui(QString net, QString buf, QString message);
    void displayMsg(Message message);
    void displayStatusMsg(QString, QString);

    void connectToIrc(QString net);
    void disconnectFromIrc(QString net);
    void serverStateRequested();

    void backlogData(BufferId, QList<QVariant>, bool done);

    void bufferIdUpdated(BufferId);
    void sessionDataChanged(const QString &key);
    void sessionDataChanged(const QString &key, const QVariant &data);

  private slots:
    void recvStatusMsgFromServer(QString msg);
    void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
    void serverConnected(QString net);
    void serverDisconnected(QString net);

  private:
    UserId user;

    CoreProxy *coreProxy;
    Storage *storage;
    QHash<QString, Server *> servers;

    VarMap sessionData;
    QMutex mutex;

};

#endif
