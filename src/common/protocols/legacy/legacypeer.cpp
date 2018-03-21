/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QHostAddress>
#include <QDataStream>
#include <QTcpSocket>

#include "legacypeer.h"
#include "quassel.h"

/* version.inc is no longer used for this */
const uint protocolVersion = 10;
const uint coreNeedsProtocol = protocolVersion;
const uint clientNeedsProtocol = protocolVersion;

using namespace Protocol;

LegacyPeer::LegacyPeer(::AuthHandler *authHandler, QTcpSocket *socket, Compressor::CompressionLevel level, QObject *parent)
    : RemotePeer(authHandler, socket, level, parent),
    _useCompression(false)
{

}


void LegacyPeer::setSignalProxy(::SignalProxy *proxy)
{
    RemotePeer::setSignalProxy(proxy);

    // FIXME only in compat mode
    if (proxy) {
        // enable compression now if requested - the initial handshake is uncompressed in the legacy protocol!
        _useCompression = socket()->property("UseCompression").toBool();
        if (_useCompression)
            qDebug() << "Using compression for peer:" << qPrintable(socket()->peerAddress().toString());
    }

}


void LegacyPeer::processMessage(const QByteArray &msg)
{
    QDataStream stream(msg);
    stream.setVersion(QDataStream::Qt_4_2);

    QVariant item;
    if (_useCompression) {
        QByteArray rawItem;
        stream >> rawItem;

        int nbytes = rawItem.size();
        if (nbytes <= 4) {
            const char *data = rawItem.constData();
            if (nbytes < 4 || (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0)) {
                close("Peer sent corrupted compressed data!");
                return;
            }
        }

        rawItem = qUncompress(rawItem);

        QDataStream itemStream(&rawItem, QIODevice::ReadOnly);
        itemStream.setVersion(QDataStream::Qt_4_2);
        itemStream >> item;
    }
    else {
        stream >> item;
    }

    if (stream.status() != QDataStream::Ok || !item.isValid()) {
        close("Peer sent corrupt data: unable to load QVariant!");
        return;
    }

    // if no sigproxy is set, we're in handshake mode and let the data be handled elsewhere
    if (!signalProxy())
        handleHandshakeMessage(item);
    else
        handlePackedFunc(item);
}


void LegacyPeer::writeMessage(const QVariant &item)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_2);

    if (_useCompression) {
        QByteArray rawItem;
        QDataStream itemStream(&rawItem, QIODevice::WriteOnly);
        itemStream.setVersion(QDataStream::Qt_4_2);
        itemStream << item;

        rawItem = qCompress(rawItem);

        out << rawItem;
    }
    else {
        out << item;
    }

    writeMessage(block);
}


/*** Handshake messages ***/

/* These messages are transmitted during handshake phase, which in case of the legacy protocol means they have
 * a structure different from those being used after the handshake.
 * Also, the legacy handshake does not fully match the redesigned one, so we'll have to do various mappings here.
 */

void LegacyPeer::handleHandshakeMessage(const QVariant &msg)
{
    QVariantMap m = msg.toMap();

    QString msgType = m["MsgType"].toString();
    if (msgType.isEmpty()) {
        emit protocolError(tr("Invalid handshake message!"));
        return;
    }

    if (msgType == "ClientInit") {
        // FIXME only in compat mode
        uint ver = m["ProtocolVersion"].toUInt();
        if (ver < coreNeedsProtocol) {
            emit protocolVersionMismatch((int)ver, (int)coreNeedsProtocol);
            return;
        }

#ifndef QT_NO_COMPRESS
        // FIXME only in compat mode
        if (m["UseCompression"].toBool()) {
            socket()->setProperty("UseCompression", true);
        }
#endif
        handle(RegisterClient{Quassel::Features{m["FeatureList"].toStringList(), Quassel::LegacyFeatures(m["Features"].toUInt())},
                              m["ClientVersion"].toString(),
                              m["ClientDate"].toString(),
                              m["UseSsl"].toBool()});
    }

    else if (msgType == "ClientInitReject") {
        handle(ClientDenied(m["Error"].toString()));
    }

    else if (msgType == "ClientInitAck") {
        // FIXME only in compat mode
        uint ver = m["ProtocolVersion"].toUInt(); // actually an UInt
        if (ver < clientNeedsProtocol) {
            emit protocolVersionMismatch((int)ver, (int)clientNeedsProtocol);
            return;
        }
#ifndef QT_NO_COMPRESS
        if (m["SupportsCompression"].toBool())
            socket()->setProperty("UseCompression", true);
#endif

        handle(ClientRegistered{Quassel::Features{m["FeatureList"].toStringList(), Quassel::LegacyFeatures(m["CoreFeatures"].toUInt())},
                                m["Configured"].toBool(),
                                m["StorageBackends"].toList(),
                                m["Authenticators"].toList(),
                                m["SupportSsl"].toBool()});
    }

    else if (msgType == "CoreSetupData") {
        QVariantMap map = m["SetupData"].toMap();
        handle(SetupData(map["AdminUser"].toString(), map["AdminPasswd"].toString(), map["Backend"].toString(), map["ConnectionProperties"].toMap(), map["Authenticator"].toString(), map["AuthProperties"].toMap()));
    }

    else if (msgType == "CoreSetupReject") {
        handle(SetupFailed(m["Error"].toString()));
    }

    else if (msgType == "CoreSetupAck") {
        handle(SetupDone());
    }

    else if (msgType == "ClientLogin") {
        handle(Login(m["User"].toString(), m["Password"].toString()));
    }

    else if (msgType == "ClientLoginReject") {
        handle(LoginFailed(m["Error"].toString()));
    }

    else if (msgType == "ClientLoginAck") {
        handle(LoginSuccess());
    }

    else if (msgType == "SessionInit") {
        QVariantMap map = m["SessionState"].toMap();
        handle(SessionState(map["Identities"].toList(), map["BufferInfos"].toList(), map["NetworkIds"].toList()));
    }

    else {
        emit protocolError(tr("Unknown protocol message of type %1").arg(msgType));
    }
}


void LegacyPeer::dispatch(const RegisterClient &msg) {
    QVariantMap m;
    m["MsgType"] = "ClientInit";
    m["Features"] = static_cast<quint32>(msg.features.toLegacyFeatures());
    m["FeatureList"] = msg.features.toStringList();
    m["ClientVersion"] = msg.clientVersion;
    m["ClientDate"] = msg.buildDate;

    // FIXME only in compat mode
    m["ProtocolVersion"] = protocolVersion;
    m["UseSsl"] = msg.sslSupported;
#ifndef QT_NO_COMPRESS
    m["UseCompression"] = true;
#else
    m["UseCompression"] = false;
#endif

    writeMessage(m);
}


void LegacyPeer::dispatch(const ClientDenied &msg) {
    QVariantMap m;
    m["MsgType"] = "ClientInitReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}


void LegacyPeer::dispatch(const ClientRegistered &msg) {
    QVariantMap m;
    m["MsgType"] = "ClientInitAck";
    if (hasFeature(Quassel::Feature::ExtendedFeatures)) {
        m["FeatureList"] = msg.features.toStringList();
    }
    else {
        m["CoreFeatures"] = static_cast<quint32>(msg.features.toLegacyFeatures());
    }
    m["StorageBackends"] = msg.backendInfo;
    if (hasFeature(Quassel::Feature::Authenticators)) {
        m["Authenticators"] = msg.authenticatorInfo;
    }

    // FIXME only in compat mode
    m["ProtocolVersion"] = protocolVersion;
    m["SupportSsl"] = msg.sslSupported;
    m["SupportsCompression"] = socket()->property("UseCompression").toBool(); // this property gets already set in the ClientInit handler

    // This is only used for display by really old v10 clients (pre-0.5), and we no longer set this
    m["CoreInfo"] = QString();

    m["LoginEnabled"] = m["Configured"] = msg.coreConfigured;

    writeMessage(m);
}


void LegacyPeer::dispatch(const SetupData &msg)
{
    QVariantMap map;
    map["AdminUser"] = msg.adminUser;
    map["AdminPasswd"] = msg.adminPassword;
    map["Backend"] = msg.backend;
    map["ConnectionProperties"] = msg.setupData;

    // Auth backend properties.
    map["Authenticator"] = msg.authenticator;
    map["AuthProperties"] = msg.authSetupData;

    QVariantMap m;
    m["MsgType"] = "CoreSetupData";
    m["SetupData"] = map;
    writeMessage(m);
}


void LegacyPeer::dispatch(const SetupFailed &msg)
{
    QVariantMap m;
    m["MsgType"] = "CoreSetupReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}


void LegacyPeer::dispatch(const SetupDone &msg)
{
    Q_UNUSED(msg)

    QVariantMap m;
    m["MsgType"] = "CoreSetupAck";

    writeMessage(m);
}


void LegacyPeer::dispatch(const Login &msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientLogin";
    m["User"] = msg.user;
    m["Password"] = msg.password;

    writeMessage(m);
}


void LegacyPeer::dispatch(const LoginFailed &msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientLoginReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}


void LegacyPeer::dispatch(const LoginSuccess &msg)
{
    Q_UNUSED(msg)

    QVariantMap m;
    m["MsgType"] = "ClientLoginAck";

    writeMessage(m);
}


void LegacyPeer::dispatch(const SessionState &msg)
{
    QVariantMap m;
    m["MsgType"] = "SessionInit";

    QVariantMap map;
    map["BufferInfos"] = msg.bufferInfos;
    map["NetworkIds"] = msg.networkIds;
    map["Identities"] = msg.identities;
    m["SessionState"] = map;

    writeMessage(m);
}


/*** Standard messages ***/

void LegacyPeer::handlePackedFunc(const QVariant &packedFunc)
{
    QVariantList params(packedFunc.toList());

    if (params.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Received incompatible data:" << packedFunc;
        return;
    }

    // TODO: make sure that this is a valid request type
    RequestType requestType = (RequestType)params.takeFirst().value<int>();
    switch (requestType) {
        case Sync: {
            if (params.count() < 3) {
                qWarning() << Q_FUNC_INFO << "Received invalid sync call:" << params;
                return;
            }
            QByteArray className = params.takeFirst().toByteArray();
            QString objectName = params.takeFirst().toString();
            QByteArray slotName = params.takeFirst().toByteArray();
            handle(Protocol::SyncMessage(className, objectName, slotName, params));
            break;
        }
        case RpcCall: {
            if (params.empty()) {
                qWarning() << Q_FUNC_INFO << "Received empty RPC call!";
                return;
            }
            QByteArray slotName = params.takeFirst().toByteArray();
            handle(Protocol::RpcCall(slotName, params));
            break;
        }
        case InitRequest: {
            if (params.count() != 2) {
                qWarning() << Q_FUNC_INFO << "Received invalid InitRequest:" << params;
                return;
            }
            QByteArray className = params[0].toByteArray();
            QString objectName = params[1].toString();
            handle(Protocol::InitRequest(className, objectName));
            break;
        }
        case InitData: {
            if (params.count() != 3) {
                qWarning() << Q_FUNC_INFO << "Received invalid InitData:" << params;
                return;
            }
            QByteArray className = params[0].toByteArray();
            QString objectName = params[1].toString();
            QVariantMap initData = params[2].toMap();

            // we need to special-case IrcUsersAndChannels here, since the format changed
            if (className == "Network")
                fromLegacyIrcUsersAndChannels(initData);
            handle(Protocol::InitData(className, objectName, initData));
            break;
        }
        case HeartBeat: {
            if (params.count() != 1) {
                qWarning() << Q_FUNC_INFO << "Received invalid HeartBeat:" << params;
                return;
            }
            // The legacy protocol would only send a QTime, no QDateTime
            // so we assume it's sent today, which works in exactly the same cases as it did in the old implementation
            QDateTime dateTime = QDateTime::currentDateTime().toUTC();
            dateTime.setTime(params[0].toTime());
            handle(Protocol::HeartBeat(dateTime));
            break;
        }
        case HeartBeatReply: {
            if (params.count() != 1) {
                qWarning() << Q_FUNC_INFO << "Received invalid HeartBeat:" << params;
                return;
            }
            // The legacy protocol would only send a QTime, no QDateTime
            // so we assume it's sent today, which works in exactly the same cases as it did in the old implementation
            QDateTime dateTime = QDateTime::currentDateTime().toUTC();
            dateTime.setTime(params[0].toTime());
            handle(Protocol::HeartBeatReply(dateTime));
            break;
        }

    }
}


void LegacyPeer::dispatch(const Protocol::SyncMessage &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)Sync << msg.className << msg.objectName << msg.slotName << msg.params);
}


void LegacyPeer::dispatch(const Protocol::RpcCall &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)RpcCall << msg.slotName << msg.params);
}


void LegacyPeer::dispatch(const Protocol::InitRequest &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)InitRequest << msg.className << msg.objectName);
}


void LegacyPeer::dispatch(const Protocol::InitData &msg)
{
    // We need to special-case IrcUsersAndChannels, as the format changed
    if (msg.className == "Network") {
        QVariantMap initData = msg.initData;
        toLegacyIrcUsersAndChannels(initData);
        dispatchPackedFunc(QVariantList() << (qint16)InitData << msg.className << msg.objectName << initData);
    }
    else
        dispatchPackedFunc(QVariantList() << (qint16)InitData << msg.className << msg.objectName << msg.initData);
}


void LegacyPeer::dispatch(const Protocol::HeartBeat &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeat << msg.timestamp.time());
}


void LegacyPeer::dispatch(const Protocol::HeartBeatReply &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeatReply << msg.timestamp.time());
}


void LegacyPeer::dispatchPackedFunc(const QVariantList &packedFunc)
{
    writeMessage(QVariant(packedFunc));
}


// Handle the changed format for Network's initData
// cf. Network::initIrcUsersAndChannels()
void LegacyPeer::fromLegacyIrcUsersAndChannels(QVariantMap &initData)
{
    const QVariantMap &legacyMap = initData["IrcUsersAndChannels"].toMap();
    QVariantMap newMap;

    QHash<QString, QVariantList> users;
    foreach(const QVariant &v, legacyMap["users"].toMap().values()) {
        const QVariantMap &map = v.toMap();
        foreach(const QString &key, map.keys())
            users[key] << map[key];
    }
    QVariantMap userMap;
    foreach(const QString &key, users.keys())
        userMap[key] = users[key];
    newMap["Users"] = userMap;

    QHash<QString, QVariantList> channels;
    foreach(const QVariant &v, legacyMap["channels"].toMap().values()) {
        const QVariantMap &map = v.toMap();
        foreach(const QString &key, map.keys())
            channels[key] << map[key];
    }
    QVariantMap channelMap;
    foreach(const QString &key, channels.keys())
        channelMap[key] = channels[key];
    newMap["Channels"] = channelMap;

    initData["IrcUsersAndChannels"] = newMap;
}


void LegacyPeer::toLegacyIrcUsersAndChannels(QVariantMap &initData)
{
    const QVariantMap &usersAndChannels = initData["IrcUsersAndChannels"].toMap();
    QVariantMap legacyMap;

    // toMap() and toList() are cheap, so no need to copy to a hash

    QVariantMap userMap;
    const QVariantMap &users = usersAndChannels["Users"].toMap();

    int size = users["nick"].toList().size(); // we know this key exists
    for(int i = 0; i < size; i++) {
        QVariantMap map;
        foreach(const QString &key, users.keys())
            map[key] = users[key].toList().at(i);
        QString hostmask = QString("%1!%2@%3").arg(map["nick"].toString(), map["user"].toString(), map["host"].toString());
        userMap[hostmask.toLower()] = map;
    }
    legacyMap["users"] = userMap;

    QVariantMap channelMap;
    const QVariantMap &channels = usersAndChannels["Channels"].toMap();

    size = channels["name"].toList().size();
    for(int i = 0; i < size; i++) {
        QVariantMap map;
        foreach(const QString &key, channels.keys())
            map[key] = channels[key].toList().at(i);
        channelMap[map["name"].toString().toLower()] = map;
    }
    legacyMap["channels"] = channelMap;

    initData["IrcUsersAndChannels"] = legacyMap;
}
