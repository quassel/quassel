/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef CLIENTSYNCER_H_
#define CLIENTSYNCER_H_

#include <QPointer>
#include <QString>
#include <QVariantMap>

#ifdef HAVE_SSL
#  include <QSslSocket>
#else
#  include <QTcpSocket>
#endif

#include "types.h"

class IrcUser;
class IrcChannel;
class SignalProxy;

class ClientSyncer : public QObject {
  Q_OBJECT

public:
  ClientSyncer(QObject *parent = 0);
  ~ClientSyncer();

signals:
  void recvPartialItem(quint32 avail, quint32 size);
  void connectionError(const QString &errorMsg);
  void connectionMsg(const QString &msg);
  void sessionProgress(quint32 part, quint32 total);
  void networksProgress(quint32 part, quint32 total);
  void socketStateChanged(QAbstractSocket::SocketState);
  void socketDisconnected();

  void startLogin();
  void loginFailed(const QString &error);
  void loginSuccess();
  void syncFinished();
  void startCoreSetup(const QVariantList &);
  void coreSetupSuccess();
  void coreSetupFailed(const QString &error);

  void encrypted(bool);

  void startInternalCore();
  void connectToInternalCore(SignalProxy *proxy);

public slots:
  void connectToCore(const QVariantMap &);
  void loginToCore(const QString &user, const QString &passwd);
  void disconnectFromCore();
  void useInternalCore(AccountId internalAccountId);

private slots:
  void coreSocketError(QAbstractSocket::SocketError);
  void coreHasData();
  void coreSocketConnected();
  void coreSocketDisconnected();

  void clientInitAck(const QVariantMap &msg);

  // for sync progress
  void networkInitDone();
  void checkSyncState();

  void syncToCore(const QVariantMap &sessionState);
  void internalSessionStateReceived(const QVariant &packedState);
  void sessionStateReceived(const QVariantMap &state);

  void doCoreSetup(const QVariant &setupData);
#ifdef HAVE_SSL
    void sslErrors(const QList<QSslError> &errors);
#endif

private:
  QPointer<QIODevice> socket;
  quint32 blockSize;
  QVariantMap coreConnectionInfo;

  QSet<QObject *> netsToSync;
  int numNetsToSync;
};

#endif
