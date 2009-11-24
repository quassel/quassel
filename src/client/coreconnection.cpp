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

#include "coreconnection.h"

#ifndef QT_NO_NETWORKPROXY
#  include <QNetworkProxy>
#endif

#include "client.h"
#include "clientsettings.h"
#include "coreaccountmodel.h"
#include "identity.h"
#include "network.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "util.h"

CoreConnection::CoreConnection(CoreAccountModel *model, QObject *parent)
  : QObject(parent),
  _model(model),
  _blockSize(0),
  _state(Disconnected),
  _progressMinimum(0),
  _progressMaximum(-1),
  _progressValue(-1)
{
  qRegisterMetaType<ConnectionState>("CoreConnection::ConnectionState");

}

void CoreConnection::init() {
  connect(Client::signalProxy(), SIGNAL(disconnected()), SLOT(coreSocketDisconnected()));
}

void CoreConnection::setProgressText(const QString &text) {
  if(_progressText != text) {
    _progressText = text;
    emit progressTextChanged(text);
  }
}

void CoreConnection::setProgressValue(int value) {
  if(_progressValue != value) {
    _progressValue = value;
    emit progressValueChanged(value);
  }
}

void CoreConnection::setProgressMinimum(int minimum) {
  if(_progressMinimum != minimum) {
    _progressMinimum = minimum;
    emit progressRangeChanged(minimum, _progressMaximum);
  }
}

void CoreConnection::setProgressMaximum(int maximum) {
  if(_progressMaximum != maximum) {
    _progressMaximum = maximum;
    emit progressRangeChanged(_progressMinimum, maximum);
  }
}

void CoreConnection::updateProgress(int value, int max) {
  if(max != _progressMaximum) {
    _progressMaximum = max;
    emit progressRangeChanged(_progressMinimum, _progressMaximum);
  }
  setProgressValue(value);
}

void CoreConnection::resetConnection() {
  if(_socket) {
    disconnect(_socket, 0, this, 0);
    _socket->deleteLater();
    _socket = 0;
  }
  _blockSize = 0;

  _coreMsgBuffer.clear();

  _netsToSync.clear();
  _numNetsToSync = 0;
  _state = Disconnected;

  setProgressMaximum(-1); // disable
  emit connectionMsg(tr("Disconnected from core."));
}

void CoreConnection::socketStateChanged(QAbstractSocket::SocketState socketState) {
  QString text;

  switch(socketState) {
  case QAbstractSocket::UnconnectedState:
    text = tr("Disconnected.");
    break;
  case QAbstractSocket::HostLookupState:
    text = tr("Looking up %1...").arg(currentAccount().hostName());
    break;
  case QAbstractSocket::ConnectingState:
    text = tr("Connecting to %1...").arg(currentAccount().hostName());
    break;
  case QAbstractSocket::ConnectedState:
    text = tr("Connected to %1.").arg(currentAccount().hostName());
    break;
  case QAbstractSocket::ClosingState:
    text = tr("Disconnecting from %1...").arg(currentAccount().hostName());
    break;
  default:
    break;
  }

  if(!text.isEmpty())
    emit progressTextChanged(text);

  setState(socketState);
}

void CoreConnection::setState(QAbstractSocket::SocketState socketState) {
  ConnectionState state;

  switch(socketState) {
  case QAbstractSocket::UnconnectedState:
    state = Disconnected;
    break;
  case QAbstractSocket::HostLookupState:
  case QAbstractSocket::ConnectingState:
    state = Connecting;
    break;
  case QAbstractSocket::ConnectedState:
    state = Connected;
    break;
  default:
    state = Disconnected;
  }

  setState(state);
}

void CoreConnection::setState(ConnectionState state) {
  if(state != _state) {
    _state = state;
    emit stateChanged(state);
  }
}

void CoreConnection::setWarningsHandler(const char *slot) {
  resetWarningsHandler();
  connect(this, SIGNAL(handleIgnoreWarnings(bool)), this, slot);
}

void CoreConnection::resetWarningsHandler() {
  disconnect(this, SIGNAL(handleIgnoreWarnings(bool)), this, 0);
}

void CoreConnection::coreSocketError(QAbstractSocket::SocketError) {
  qDebug() << "coreSocketError" << _socket << _socket->errorString();
  emit connectionError(_socket->errorString());
  resetConnection();
}

void CoreConnection::coreSocketDisconnected() {
  setState(Disconnected);
  emit disconnected();
  resetConnection();
  // FIXME handle disconnects gracefully
}

void CoreConnection::coreHasData() {
  QVariant item;
  while(SignalProxy::readDataFromDevice(_socket, _blockSize, item)) {
    QVariantMap msg = item.toMap();
    if(!msg.contains("MsgType")) {
      // This core is way too old and does not even speak our init protocol...
      emit connectionError(tr("The Quassel Core you try to connect to is too old! Please consider upgrading."));
      disconnectFromCore();
      return;
    }
    if(msg["MsgType"] == "ClientInitAck") {
      clientInitAck(msg);
    } else if(msg["MsgType"] == "ClientInitReject") {
      emit connectionError(msg["Error"].toString());
      disconnectFromCore();
      return;
    } else if(msg["MsgType"] == "CoreSetupAck") {
      //emit coreSetupSuccess();
    } else if(msg["MsgType"] == "CoreSetupReject") {
      //emit coreSetupFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginReject") {
      loginFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginAck") {
      loginSuccess();
    } else if(msg["MsgType"] == "SessionInit") {
      // that's it, let's hand over to the signal proxy
      // if the socket is an orphan, the signalProxy adopts it.
      // -> we don't need to care about it anymore
      _socket->setParent(0);
      Client::signalProxy()->addPeer(_socket);

      sessionStateReceived(msg["SessionState"].toMap());
      break; // this is definitively the last message we process here!
    } else {
      emit connectionError(tr("<b>Invalid data received from core!</b><br>Disconnecting."));
      disconnectFromCore();
      return;
    }
  }
  if(_blockSize > 0) {
    updateProgress(_socket->bytesAvailable(), _blockSize);
  }
}

void CoreConnection::disconnectFromCore() {
  Client::signalProxy()->removeAllPeers();
  resetConnection();
}

void CoreConnection::reconnectToCore() {
  if(currentAccount().isValid())
    connectToCore(currentAccount().accountId());
}

bool CoreConnection::connectToCore(AccountId accId) {
  if(isConnected())
    return false;

  CoreAccountSettings s;

  if(!accId.isValid()) {
    // check our settings and figure out what to do
    if(!s.autoConnectOnStartup())
      return false;
    if(s.autoConnectToFixedAccount())
      accId = s.autoConnectAccount();
    else
      accId = s.lastAccount();
    if(!accId.isValid())
      return false;
  }
  _account = accountModel()->account(accId);
  if(!_account.accountId().isValid()) {
    return false;
  }

  s.setLastAccount(accId);
  connectToCurrentAccount();
  return true;
}

void CoreConnection::connectToCurrentAccount() {
  resetConnection();

  Q_ASSERT(!_socket);
#ifdef HAVE_SSL
  QSslSocket *sock = new QSslSocket(Client::instance());
#else
  if(_account.useSsl()) {
    emit connectionError(tr("<b>This client is built without SSL Support!</b><br />Disable the usage of SSL in the account settings."));
    return;
  }
  QTcpSocket *sock = new QTcpSocket(Client::instance());
#endif

#ifndef QT_NO_NETWORKPROXY
  if(_account.useProxy()) {
    QNetworkProxy proxy(_account.proxyType(), _account.proxyHostName(), _account.proxyPort(), _account.proxyUser(), _account.proxyPassword());
    sock->setProxy(proxy);
  }
#endif

  _socket = sock;
  connect(sock, SIGNAL(readyRead()), SLOT(coreHasData()));
  connect(sock, SIGNAL(connected()), SLOT(coreSocketConnected()));
  connect(sock, SIGNAL(disconnected()), SLOT(coreSocketDisconnected()));
  connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(coreSocketError(QAbstractSocket::SocketError)));
  connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));

  emit connectionMsg(tr("Connecting to %1...").arg(currentAccount().accountName()));
  sock->connectToHost(_account.hostName(), _account.port());
}

void CoreConnection::coreSocketConnected() {
  // Phase One: Send client info and wait for core info

  emit connectionMsg(tr("Synchronizing to core..."));

  QVariantMap clientInit;
  clientInit["MsgType"] = "ClientInit";
  clientInit["ClientVersion"] = Quassel::buildInfo().fancyVersionString;
  clientInit["ClientDate"] = Quassel::buildInfo().buildDate;
  clientInit["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
  clientInit["UseSsl"] = _account.useSsl();
#ifndef QT_NO_COMPRESS
  clientInit["UseCompression"] = true;
#else
  clientInit["UseCompression"] = false;
#endif

  SignalProxy::writeDataToDevice(_socket, clientInit);
}

void CoreConnection::clientInitAck(const QVariantMap &msg) {
  // Core has accepted our version info and sent its own. Let's see if we accept it as well...
  uint ver = msg["ProtocolVersion"].toUInt();
  if(ver < Quassel::buildInfo().clientNeedsProtocol) {
    emit connectionError(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
        "Need at least core/client protocol v%1 to connect.").arg(Quassel::buildInfo().clientNeedsProtocol));
    disconnectFromCore();
    return;
  }

#ifndef QT_NO_COMPRESS
  if(msg["SupportsCompression"].toBool()) {
    _socket->setProperty("UseCompression", true);
  }
#endif

  _coreMsgBuffer = msg;
#ifdef HAVE_SSL
  if(currentAccount().useSsl()) {
    if(msg["SupportSsl"].toBool()) {
      QSslSocket *sslSocket = qobject_cast<QSslSocket *>(_socket);
      Q_ASSERT(sslSocket);
      connect(sslSocket, SIGNAL(encrypted()), this, SLOT(sslSocketEncrypted()));
      connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));

      sslSocket->startClientEncryption();
    } else {
      emit connectionError(tr("<b>The Quassel Core you are trying to connect to does not support SSL!</b><br />If you want to connect anyways, disable the usage of SSL in the account settings."));
      disconnectFromCore();
    }
    return;
  }
#endif
  // if we use SSL we wait for the next step until every SSL warning has been cleared
  connectionReady();

}

void CoreConnection::connectionReady() {
  if(!_coreMsgBuffer["Configured"].toBool()) {
    // start wizard
    emit startCoreSetup(_coreMsgBuffer["StorageBackends"].toList());
  } else if(_coreMsgBuffer["LoginEnabled"].toBool()) {
    loginToCore();
  }
  _coreMsgBuffer.clear();
  resetWarningsHandler();
}

void CoreConnection::loginToCore(const QString &prevError) {
  emit connectionMsg(tr("Logging in..."));
  if(currentAccount().user().isEmpty() || currentAccount().password().isEmpty() || !prevError.isEmpty()) {
    bool valid = false;
    emit userAuthenticationRequired(&_account, &valid, prevError);  // *must* be a synchronous call
    if(!valid || currentAccount().user().isEmpty() || currentAccount().password().isEmpty()) {
      disconnectFromCore();
      emit connectionError(tr("Login canceled"));
      return;
    }
  }

  QVariantMap clientLogin;
  clientLogin["MsgType"] = "ClientLogin";
  clientLogin["User"] = currentAccount().user();
  clientLogin["Password"] = currentAccount().password();
  SignalProxy::writeDataToDevice(_socket, clientLogin);
}

void CoreConnection::loginFailed(const QString &error) {
  loginToCore(error);
}

void CoreConnection::loginSuccess() {
  updateProgress(0, 0);

  // save current account data
  _model->createOrUpdateAccount(currentAccount());
  _model->save();

  setProgressText(tr("Receiving session state"));
  setState(Synchronizing);
  emit connectionMsg(tr("Synchronizing to %1...").arg(currentAccount().accountName()));
}

void CoreConnection::sessionStateReceived(const QVariantMap &state) {
  updateProgress(100, 100);

  // rest of communication happens through SignalProxy...
  disconnect(_socket, SIGNAL(readyRead()), this, 0);
  disconnect(_socket, SIGNAL(connected()), this, 0);

  //Client::instance()->setConnectedToCore(currentAccount().accountId(), _socket);
  syncToCore(state);
}

void CoreConnection::syncToCore(const QVariantMap &sessionState) {
  setProgressText(tr("Receiving network states"));
  updateProgress(0, 100);

  // create identities
  foreach(QVariant vid, sessionState["Identities"].toList()) {
    Client::instance()->coreIdentityCreated(vid.value<Identity>());
  }

  // create buffers
  // FIXME: get rid of this crap -- why?
  QVariantList bufferinfos = sessionState["BufferInfos"].toList();
  NetworkModel *networkModel = Client::networkModel();
  Q_ASSERT(networkModel);
  foreach(QVariant vinfo, bufferinfos)
    networkModel->bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

  QVariantList networkids = sessionState["NetworkIds"].toList();

  // prepare sync progress thingys...
  // FIXME: Care about removal of networks
  _numNetsToSync = networkids.count();
  updateProgress(0, _numNetsToSync);

  // create network objects
  foreach(QVariant networkid, networkids) {
    NetworkId netid = networkid.value<NetworkId>();
    if(Client::network(netid))
      continue;
    Network *net = new Network(netid, Client::instance());
    _netsToSync.insert(net);
    connect(net, SIGNAL(initDone()), SLOT(networkInitDone()));
    connect(net, SIGNAL(destroyed()), SLOT(networkInitDone()));
    Client::addNetwork(net);
  }
  checkSyncState();
}

void CoreConnection::networkInitDone() {
  Network *net = qobject_cast<Network *>(sender());
  Q_ASSERT(net);
  disconnect(net, 0, this, 0);
  _netsToSync.remove(net);
  updateProgress(_numNetsToSync - _netsToSync.count(), _numNetsToSync);
  checkSyncState();
}

void CoreConnection::checkSyncState() {
  if(_netsToSync.isEmpty()) {
    setState(Synchronized);
    setProgressText(QString());
    setProgressMaximum(-1);
    emit synchronized();
  }
}
