/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "clientsyncer.h"

#ifndef QT_NO_NETWORKPROXY
#  include <QNetworkProxy>
#endif

#include "client.h"
#include "identity.h"
#include "network.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "util.h"

ClientSyncer::ClientSyncer(QObject *parent)
  : QObject(parent),
    _socket(0),
    _blockSize(0)
{
}

ClientSyncer::~ClientSyncer() {
}

void ClientSyncer::coreHasData() {
  QVariant item;
  while(SignalProxy::readDataFromDevice(_socket, _blockSize, item)) {
    emit recvPartialItem(1,1);
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
      emit coreSetupSuccess();
    } else if(msg["MsgType"] == "CoreSetupReject") {
      emit coreSetupFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginReject") {
      emit loginFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginAck") {
      // prevent multiple signal connections
      disconnect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));
      connect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));
      emit loginSuccess();
    } else if(msg["MsgType"] == "SessionInit") {
      sessionStateReceived(msg["SessionState"].toMap());
      break; // this is definitively the last message we process here!
    } else {
      emit connectionError(tr("<b>Invalid data received from core!</b><br>Disconnecting."));
      disconnectFromCore();
      return;
    }
  }
  if(_blockSize > 0) {
    emit recvPartialItem(_socket->bytesAvailable(), _blockSize);
  }
}

void ClientSyncer::coreSocketError(QAbstractSocket::SocketError) {
  qDebug() << "coreSocketError" << _socket << _socket->errorString();
  emit connectionError(_socket->errorString());
  resetConnection();
}

void ClientSyncer::disconnectFromCore() {
  resetConnection();
}

void ClientSyncer::connectToCore(const QVariantMap &conn) {
  resetConnection();
  coreConnectionInfo = conn;

  if(conn["Host"].toString().isEmpty()) {
    emit connectionError(tr("No Host to connect to specified."));
    return;
  }

  Q_ASSERT(!_socket);
#ifdef HAVE_SSL
  QSslSocket *sock = new QSslSocket(Client::instance());
#else
  if(conn["useSsl"].toBool()) {
    emit connectionError(tr("<b>This client is built without SSL Support!</b><br />Disable the usage of SSL in the account settings."));
    return;
  }
  QTcpSocket *sock = new QTcpSocket(Client::instance());
#endif

#ifndef QT_NO_NETWORKPROXY
  if(conn.contains("useProxy") && conn["useProxy"].toBool()) {
    QNetworkProxy proxy((QNetworkProxy::ProxyType)conn["proxyType"].toInt(), conn["proxyHost"].toString(), conn["proxyPort"].toUInt(), conn["proxyUser"].toString(), conn["proxyPassword"].toString());
    sock->setProxy(proxy);
  }
#endif

  _socket = sock;
  connect(sock, SIGNAL(readyRead()), this, SLOT(coreHasData()));
  connect(sock, SIGNAL(connected()), this, SLOT(coreSocketConnected()));
  connect(sock, SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
  connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
  connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)));
  sock->connectToHost(conn["Host"].toString(), conn["Port"].toUInt());
}

void ClientSyncer::coreSocketConnected() {
  //connect(this, SIGNAL(recvPartialItem(uint, uint)), this, SIGNAL(coreConnectionProgress(uint, uint)));
  // Phase One: Send client info and wait for core info

  //emit coreConnectionMsg(tr("Synchronizing to core..."));
  QVariantMap clientInit;
  clientInit["MsgType"] = "ClientInit";
  clientInit["ClientVersion"] = Quassel::buildInfo().fancyVersionString;
  clientInit["ClientDate"] = Quassel::buildInfo().buildDate;
  clientInit["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
  clientInit["UseSsl"] = coreConnectionInfo["useSsl"];
#ifndef QT_NO_COMPRESS
  clientInit["UseCompression"] = true;
#else
  clientInit["UseCompression"] = false;
#endif

  SignalProxy::writeDataToDevice(_socket, clientInit);
}

void ClientSyncer::useInternalCore() {
  AccountId internalAccountId;

  CoreAccountSettings accountSettings;
  QList<AccountId> knownAccounts = accountSettings.knownAccounts();
  foreach(AccountId id, knownAccounts) {
    if(!id.isValid())
      continue;
    QVariantMap data = accountSettings.retrieveAccountData(id);
    if(data.contains("InternalAccount") && data["InternalAccount"].toBool()) {
      internalAccountId = id;
      break;
    }
  }

  if(!internalAccountId.isValid()) {
    for(AccountId i = 1;; i++) {
      if(!knownAccounts.contains(i)) {
	internalAccountId = i;
	break;
      }
    }
    QVariantMap data;
    data["InternalAccount"] = true;
    accountSettings.storeAccountData(internalAccountId, data);
  }

  coreConnectionInfo["AccountId"] = QVariant::fromValue<AccountId>(internalAccountId);
  emit startInternalCore(this);
  emit connectToInternalCore(Client::instance()->signalProxy());
}

void ClientSyncer::coreSocketDisconnected() {
  emit socketDisconnected();
  resetConnection();
  // FIXME handle disconnects gracefully
}

void ClientSyncer::clientInitAck(const QVariantMap &msg) {
  // Core has accepted our version info and sent its own. Let's see if we accept it as well...
  uint ver = msg["ProtocolVersion"].toUInt();
  if(ver < Quassel::buildInfo().clientNeedsProtocol) {
    emit connectionError(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
        "Need at least core/client protocol v%1 to connect.").arg(Quassel::buildInfo().clientNeedsProtocol));
    disconnectFromCore();
    return;
  }
  emit connectionMsg(msg["CoreInfo"].toString());

#ifndef QT_NO_COMPRESS
  if(msg["SupportsCompression"].toBool()) {
    _socket->setProperty("UseCompression", true);
  }
#endif

  _coreMsgBuffer = msg;
#ifdef HAVE_SSL
  if(coreConnectionInfo["useSsl"].toBool()) {
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

void ClientSyncer::connectionReady() {
  if(!_coreMsgBuffer["Configured"].toBool()) {
    // start wizard
    emit startCoreSetup(_coreMsgBuffer["StorageBackends"].toList());
  } else if(_coreMsgBuffer["LoginEnabled"].toBool()) {
    emit startLogin();
  }
  _coreMsgBuffer.clear();
  resetWarningsHandler();
}

void ClientSyncer::doCoreSetup(const QVariant &setupData) {
  QVariantMap setup;
  setup["MsgType"] = "CoreSetupData";
  setup["SetupData"] = setupData;
  SignalProxy::writeDataToDevice(_socket, setup);
}

void ClientSyncer::loginToCore(const QString &user, const QString &passwd) {
  emit connectionMsg(tr("Logging in..."));
  QVariantMap clientLogin;
  clientLogin["MsgType"] = "ClientLogin";
  clientLogin["User"] = user;
  clientLogin["Password"] = passwd;
  SignalProxy::writeDataToDevice(_socket, clientLogin);
}

void ClientSyncer::internalSessionStateReceived(const QVariant &packedState) {
  QVariantMap state = packedState.toMap();
  emit sessionProgress(1, 1);
  Client::instance()->setConnectedToCore(coreConnectionInfo["AccountId"].value<AccountId>());
  syncToCore(state);
}

void ClientSyncer::sessionStateReceived(const QVariantMap &state) {
  emit sessionProgress(1, 1);
  disconnect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));

  // rest of communication happens through SignalProxy...
  disconnect(_socket, 0, this, 0);
  // ... but we still want to be notified about errors...
  connect(_socket, SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
  connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));

  Client::instance()->setConnectedToCore(coreConnectionInfo["AccountId"].value<AccountId>(), _socket);
  syncToCore(state);
}

void ClientSyncer::syncToCore(const QVariantMap &sessionState) {
  // create identities
  foreach(QVariant vid, sessionState["Identities"].toList()) {
    Client::instance()->coreIdentityCreated(vid.value<Identity>());
  }

  // create buffers
  // FIXME: get rid of this crap
  QVariantList bufferinfos = sessionState["BufferInfos"].toList();
  NetworkModel *networkModel = Client::networkModel();
  Q_ASSERT(networkModel);
  foreach(QVariant vinfo, bufferinfos)
    networkModel->bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

  QVariantList networkids = sessionState["NetworkIds"].toList();

  // prepare sync progress thingys...
  // FIXME: Care about removal of networks
  numNetsToSync = networkids.count();
  emit networksProgress(0, numNetsToSync);

  // create network objects
  foreach(QVariant networkid, networkids) {
    NetworkId netid = networkid.value<NetworkId>();
    if(Client::network(netid))
      continue;
    Network *net = new Network(netid, Client::instance());
    netsToSync.insert(net);
    connect(net, SIGNAL(initDone()), this, SLOT(networkInitDone()));
    Client::addNetwork(net);
  }
  checkSyncState();
}

void ClientSyncer::networkInitDone() {
  netsToSync.remove(sender());
  emit networksProgress(numNetsToSync - netsToSync.count(), numNetsToSync);
  checkSyncState();
}

void ClientSyncer::checkSyncState() {
  if(netsToSync.isEmpty()) {
    Client::instance()->setSyncedToCore();
    emit syncFinished();
  }
}

void ClientSyncer::setWarningsHandler(const char *slot) {
  resetWarningsHandler();
  connect(this, SIGNAL(handleIgnoreWarnings(bool)), this, slot);
}

void ClientSyncer::resetWarningsHandler() {
  disconnect(this, SIGNAL(handleIgnoreWarnings(bool)), this, 0);
}

void ClientSyncer::resetConnection() {
  if(_socket) {
    disconnect(_socket, 0, this, 0);
    _socket->deleteLater();
    _socket = 0;
  }
  _blockSize = 0;

  coreConnectionInfo.clear();
  _coreMsgBuffer.clear();

  netsToSync.clear();
  numNetsToSync = 0;
}

#ifdef HAVE_SSL
void ClientSyncer::ignoreSslWarnings(bool permanently) {
  QSslSocket *sock = qobject_cast<QSslSocket *>(_socket);
  if(sock) {
    // ensure that a proper state is displayed and no longer a warning
    emit socketStateChanged(sock->state());
  }
  if(permanently) {
    if(!sock)
      qWarning() << Q_FUNC_INFO << "unable to save cert digest! Socket is either a nullptr or not a QSslSocket";
    else
      KnownHostsSettings().saveKnownHost(sock);
  }
  emit connectionMsg(_coreMsgBuffer["CoreInfo"].toString());
  connectionReady();
}

void ClientSyncer::sslSocketEncrypted() {
  QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
  Q_ASSERT(socket);

  // if there were sslErrors we already had extensive error handling
  // no need to check for a digest change again.
  if(!socket->sslErrors().isEmpty())
    return;

  QByteArray knownDigest = KnownHostsSettings().knownDigest(socket);
  if(knownDigest == socket->peerCertificate().digest()) {
    connectionReady();
    return;
  }

  QStringList warnings;
  if(!knownDigest.isEmpty()) {
    warnings << tr("Cert Digest changed! was: %1").arg(QString(prettyDigest(knownDigest)));
  }

  setWarningsHandler(SLOT(ignoreSslWarnings(bool)));
  emit connectionWarnings(warnings);
}

void ClientSyncer::sslErrors(const QList<QSslError> &errors) {
  QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
  Q_ASSERT(socket);

  socket->ignoreSslErrors();

  QByteArray knownDigest = KnownHostsSettings().knownDigest(socket);
  if(knownDigest == socket->peerCertificate().digest()) {
    connectionReady();
    return;
  }

  QStringList warnings;

  foreach(QSslError err, errors)
    warnings << err.errorString();

  if(!knownDigest.isEmpty()) {
    warnings << tr("Cert Digest changed! was: %1").arg(QString(prettyDigest(knownDigest)));
  }

  setWarningsHandler(SLOT(ignoreSslWarnings(bool)));
  emit connectionWarnings(warnings);
}
#endif
