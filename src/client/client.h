/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
#include <QPointer>

#include "buffer.h" // needed for activity lvl
class BufferInfo;
class Message;

class NetworkInfo;


class AbstractUi;
class AbstractUiMsg;
class BufferTreeModel;
class SignalProxy;

class QTimer;


class Client : public QObject {
  Q_OBJECT

public:
  static Client *instance();
  static void destroy();
  static void init(AbstractUi *);

  static QList<NetworkInfo *> networkInfos();
  static NetworkInfo *networkInfo(uint networkid);
  
  static QList<BufferInfo> allBufferInfos();
  static QList<Buffer *> buffers();
  static Buffer *buffer(uint bufferUid);
  static Buffer *buffer(BufferInfo);
  static BufferInfo statusBufferInfo(QString net);
  static BufferInfo bufferInfo(QString net, QString buf);

  static BufferTreeModel *bufferModel();
  static SignalProxy *signalProxy();

  static AbstractUiMsg *layoutMsg(const Message &);

  static bool isConnected();

  static void fakeInput(uint bufferUid, QString message);
  static void fakeInput(BufferInfo bufferInfo, QString message);
  
  static void storeSessionData(const QString &key, const QVariant &data);
  static QVariant retrieveSessionData(const QString &key, const QVariant &def = QVariant());
  static QStringList sessionDataKeys();

  enum ClientMode { LocalCore, RemoteCore };

signals:
  void sendInput(BufferInfo, QString message);
  void showBuffer(Buffer *);
  void bufferSelected(Buffer *);
  void bufferUpdated(Buffer *);
  void bufferActivity(Buffer::ActivityLevel, Buffer *);
  void backlogReceived(Buffer *, QList<Message>);
  void requestBacklog(BufferInfo, QVariant, QVariant);
  void requestNetworkStates();

  void recvPartialItem(uint avail, uint size);
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
  void connectToCore(const QVariantMap &);
  void disconnectFromCore();

private slots:
  void recvCoreState(const QVariant &state);
  void recvSessionData(const QString &key, const QVariant &data);

  void coreSocketError(QAbstractSocket::SocketError);
  void coreHasData();
  void coreSocketConnected();
  void coreSocketDisconnected();
  void coreSocketStateChanged(QAbstractSocket::SocketState);

  void userInput(BufferInfo, QString);

  void networkConnected(uint);
  void networkDisconnected(uint);

  void updateCoreConnectionProgress();
  void recvMessage(const Message &message);
  void recvStatusMsg(QString network, QString message);
  void recvBacklogData(BufferInfo, QVariantList, bool);
  void updateBufferInfo(BufferInfo);

  void bufferDestroyed();

  void layoutMsg();

private:
  Client(QObject *parent = 0);
  virtual ~Client();
  void init();
  
  void syncToCore(const QVariant &coreState);
  QVariant connectToLocalCore(QString user, QString passwd);  // defined in main.cpp
  void disconnectFromLocalCore();                             // defined in main.cpp

  static QPointer<Client> instanceptr;
  
  QPointer<QIODevice> socket;
  QPointer<SignalProxy> _signalProxy;
  QPointer<AbstractUi> mainUi;
  QPointer<BufferTreeModel> _bufferModel;

  ClientMode clientMode;

  quint32 blockSize;
  bool connectedToCore;
  
  QVariantMap coreConnectionInfo;
  QHash<uint, Buffer *> _buffers;
  QHash<uint, NetworkInfo*> _networkInfo;

  QTimer *layoutTimer;
  QList<Buffer *> layoutQueue;

  QVariantMap sessionData;


};

#endif
