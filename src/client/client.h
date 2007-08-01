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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <QAbstractSocket>
#include <QTcpSocket>
#include <QList>

#include "buffer.h"
#include "message.h"
#include "proxy_common.h"

class AbstractUi;
class ClientProxy;
class BufferTreeModel;
class QtGui;

class QTimer;

class Client : public QObject {
  Q_OBJECT

  public:
    static Client *instance();
    static void init(AbstractUi *);
    static void destroy();

    static QList<BufferId> allBufferIds();
    static Buffer *buffer(BufferId);
    static BufferId statusBufferId(QString net);
    static BufferId bufferId(QString net, QString buf);

    static BufferTreeModel *bufferModel();

    static AbstractUiMsg *layoutMsg(const Message &);

    static bool isConnected();

    static void storeSessionData(const QString &key, const QVariant &data);
    static QVariant retrieveSessionData(const QString &key, const QVariant &def = QVariant());
    static QStringList sessionDataKeys();

  signals:
    void sendInput(BufferId, QString message);
    void showBuffer(Buffer *);
    void bufferSelected(Buffer *);
    void bufferUpdated(Buffer *);
    void bufferActivity(Buffer::ActivityLevel, Buffer *);
    void bufferDestroyed(Buffer *);
    void backlogReceived(Buffer *, QList<Message>);
    void requestBacklog(BufferId, QVariant, QVariant);
    void requestNetworkStates();

    void recvPartialItem(quint32 avail, quint32 size);
    void coreConnectionError(QString errorMsg);
    void coreConnectionMsg(const QString &msg);
    void coreConnectionProgress(uint part, uint total);

    void connected();
    void disconnected();

    void sessionDataChanged(const QString &key);
    void sessionDataChanged(const QString &key, const QVariant &data);
    void sendSessionData(const QString &key, const QVariant &data);

  public slots:
    //void selectBuffer(Buffer *);
    //void connectToLocalCore();
    void connectToCore(const VarMap &);
    void disconnectFromCore();

  private slots:
    void recvSessionData(const QString &key, const QVariant &data);
    void recvProxySignal(ClientSignal sig, QVariant arg1, QVariant arg2, QVariant arg3);

    void serverError(QAbstractSocket::SocketError);
    void serverHasData();
    void coreConnected();
    void coreDisconnected();

    void userInput(BufferId, QString);
    void networkConnected(QString);
    void networkDisconnected(QString);
    void recvNetworkState(QString, QVariant);
    void recvMessage(const Message &message);
    void recvStatusMsg(QString network, QString message);
    void setTopic(QString net, QString buf, QString);
    void addNick(QString net, QString nick, VarMap props);
    void removeNick(QString net, QString nick);
    void renameNick(QString net, QString oldnick, QString newnick);
    void updateNick(QString net, QString nick, VarMap props);
    void setOwnNick(QString net, QString nick);
    void recvBacklogData(BufferId, const QList<QVariant> &, bool);
    void updateBufferId(BufferId);

    void layoutMsg();

  private:
    Client();
    ~Client();
    void init();
    static Client *instanceptr;

    void syncToCore();
    QVariant connectToLocalCore(QString user, QString passwd);  // defined in main.cpp
    void disconnectFromLocalCore();                             // defined in main.cpp

    enum ClientMode { LocalCore, RemoteCore };
    static ClientMode clientMode;

    AbstractUi *mainUi;
    ClientProxy *clientProxy;
    BufferTreeModel *_bufferModel;

    QTcpSocket socket;
    quint32 blockSize;

    static bool connectedToCore;
    static VarMap coreConnectionInfo;
    static QHash<BufferId, Buffer *> buffers;
    static QHash<uint, BufferId> bufferIds;
    static QHash<QString, QHash<QString, VarMap> > nicks;
    static QHash<QString, bool> netConnected;
    static QStringList netsAwaitingInit;
    static QHash<QString, QString> ownNick;

    QTimer *layoutTimer;
    QList<Buffer *> layoutQueue;

    VarMap sessionData;
};

#endif
