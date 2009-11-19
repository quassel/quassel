/***************************************************************************
 *   Copyright (C) 2009 by the Quassel Project                             *
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

#ifndef CORECONNECTION_H_
#define CORECONNECTION_H_

// TODO: support system application proxy (new in Qt 4.6)

#include "QPointer"

#ifdef HAVE_SSL
#  include <QSslSocket>
#else
#  include <QTcpSocket>
#endif

#include "coreaccount.h"
#include "types.h"

class CoreAccountModel;
class Network;

class CoreConnection : public QObject {
  Q_OBJECT

public:
  enum ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Synchronizing,
    Synchronized
  };

  CoreConnection(CoreAccountModel *model, QObject *parent = 0);

  void init();
  void start();

  inline ConnectionState state() const;
  inline bool isConnected() const;
  inline CoreAccount currentAccount() const;

  inline int progressMinimum() const;
  inline int progressMaximum() const;
  inline int progressValue() const;
  inline QString progressText() const;

public slots:
  void connectToCore(AccountId);
  void reconnectToCore();
  void disconnectFromCore();

//  void useInternalCore();

signals:
  void stateChanged(CoreConnection::ConnectionState);
  void synchronized();

  void connectionError(const QString &errorMsg);
  void connectionWarnings(const QStringList &warnings);
  void connectionMsg(const QString &msg);
  void disconnected();

  void progressRangeChanged(int minimum, int maximum);
  void progressValueChanged(int value);
  void progressTextChanged(const QString &);

  void startCoreSetup(const QVariantList &);

  // This signal MUST be handled synchronously!
  void userAuthenticationRequired(CoreAccount *, const QString &errorMessage = QString());

  void handleIgnoreWarnings(bool permanently);

private slots:
  void socketStateChanged(QAbstractSocket::SocketState);
  void coreSocketError(QAbstractSocket::SocketError);
  void coreHasData();
  void coreSocketConnected();
  void coreSocketDisconnected();

  void clientInitAck(const QVariantMap &msg);

  // for sync progress
  void networkInitDone();
  void checkSyncState();

  void syncToCore(const QVariantMap &sessionState);
  //void internalSessionStateReceived(const QVariant &packedState);
  void sessionStateReceived(const QVariantMap &state);

  void setWarningsHandler(const char *slot);
  void resetWarningsHandler();
  void resetConnection();
  void connectionReady();
  //void doCoreSetup(const QVariant &setupData);

  void loginToCore();
  void loginSuccess();
  void loginFailed(const QString &errorMessage);

  void updateProgress(int value, int maximum);
  void setProgressText(const QString &text);
  void setProgressValue(int value);
  void setProgressMinimum(int minimum);
  void setProgressMaximum(int maximum);

  void setState(QAbstractSocket::SocketState socketState);
  void setState(ConnectionState state);

private:
  CoreAccountModel *_model;
  CoreAccount _account;
  QVariantMap _coreMsgBuffer;

  QPointer<QIODevice> _socket;
  quint32 _blockSize;
  ConnectionState _state;

  QSet<Network *> _netsToSync;
  int _numNetsToSync;
  int _progressMinimum, _progressMaximum, _progressValue;
  QString _progressText;

  QString _coreInfoString(const QVariantMap &);

  inline CoreAccountModel *accountModel() const;
};

Q_DECLARE_METATYPE(CoreConnection::ConnectionState)

// Inlines
int CoreConnection::progressMinimum() const { return _progressMinimum; }
int CoreConnection::progressMaximum() const { return _progressMaximum; }
int CoreConnection::progressValue() const { return _progressValue; }
QString CoreConnection::progressText() const { return _progressText; }

CoreConnection::ConnectionState CoreConnection::state() const { return _state; }
bool CoreConnection::isConnected() const { return state() >= Connected; }
CoreAccount CoreConnection::currentAccount() const { return _account; }
CoreAccountModel *CoreConnection::accountModel() const { return _model; }

#endif
