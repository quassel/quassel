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

#ifndef _CORE_H_
#define _CORE_H_

#include <QString>
#include <QVariant>
#include <QSqlDatabase>

#include "server.h"
#include "storage.h"
#include "global.h"
#include "coreproxy.h"

class CoreSession;

class Core : public QObject {
  Q_OBJECT

  public:
    static Core * instance();
    static void destroy();

    static CoreSession * session(UserId);
    static CoreSession * guiSession();
    static CoreSession * createSession(UserId);

  private slots:
    void recvProxySignal(CoreSignal, QVariant, QVariant, QVariant);
    bool startListening(uint port = 4242);
    void stopListening();
    void incomingConnection();
    void clientHasData();
    void clientDisconnected();
    void updateGlobalData(UserId, QString);

  private:
    Core();
    ~Core();
    void init();
    static Core *instanceptr;

    void processClientInit(QTcpSocket *socket, const QVariant &v);
    void processClientUpdate(QTcpSocket *socket, QString key, const QVariant &data);

    UserId guiUser;
    QHash<UserId, CoreSession *> sessions;
    Storage *storage;

    QTcpServer server; // TODO: implement SSL
    QHash<QTcpSocket *, UserId> validClients;
    QHash<QTcpSocket *, quint32> blockSizes;
};

class CoreSession : public QObject {
  Q_OBJECT

  public:
    CoreSession(UserId, Storage *);
    ~CoreSession();

    QList<BufferId> buffers() const;
    inline UserId userId();
    QVariant sessionState();
    CoreProxy *proxy();

  public slots:
    void connectToIrc(QStringList);
    void processSignal(GUISignal, QVariant, QVariant, QVariant);
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

  private slots:
    //void recvProxySignal(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());
    void globalDataUpdated(UserId, QString);
    void recvStatusMsgFromServer(QString msg);
    void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
    void serverConnected(QString net);
    void serverDisconnected(QString net);

  private:
    CoreProxy *coreProxy;
    Storage *storage;
    QHash<QString, Server *> servers;
    UserId user;

};

/*
class Core : public QObject {
  Q_OBJECT

  public:

    Core();
    ~Core();
    QList<BufferId> getBuffers();

  public slots:
    void connectToIrc(QStringList);
    void sendBacklog(BufferId, QVariant, QVariant);
    void msgFromGUI(BufferId, QString message);

  signals:
    void msgFromGUI(QString net, QString buf, QString message);
    void displayMsg(Message message);
    void displayStatusMsg(QString, QString);

    void connectToIrc(QString net);
    void disconnectFromIrc(QString net);
    void serverStateRequested();

    void backlogData(BufferId, QList<QVariant>, bool done);

    void bufferIdUpdated(BufferId);

  private slots:
    void globalDataUpdated(QString);
    void recvStatusMsgFromServer(QString msg);
    void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
    void serverDisconnected(QString net);

  private:
    Storage *storage;
    QHash<QString, Server *> servers;
    UserId user;

};

*/
//extern Core *core;


#endif
