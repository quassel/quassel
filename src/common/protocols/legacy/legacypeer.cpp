/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "legacypeer.h"

#include <QDataStream>
#include <QHostAddress>
#include <QTcpSocket>

#include "quassel.h"

#include "serializers/serializers.h"

/* version.inc is no longer used for this */
const uint protocolVersion = 10;
const uint coreNeedsProtocol = protocolVersion;
const uint clientNeedsProtocol = protocolVersion;

using namespace Protocol;

LegacyPeer::LegacyPeer(::AuthHandler* authHandler, QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent)
    : RemotePeer(authHandler, socket, level, parent)
    , _useCompression(false)
{
    qDebug() << "[DEBUG] LegacyPeer constructor called - Qt5/Qt6 legacy protocol active";
}

void LegacyPeer::setSignalProxy(::SignalProxy* proxy)
{
    RemotePeer::setSignalProxy(proxy);

    // FIXME only in compat mode
    if (proxy) {
        // enable compression now if requested - the initial handshake is uncompressed in the legacy protocol!
        _useCompression = socket()->property("UseCompression").toBool();
        if (_useCompression)
            qDebug() << "Using compression for peer:" << qPrintable(address());
    }
}

void LegacyPeer::processMessage(const QByteArray& msg)
{
    QDataStream stream(msg);
    stream.setVersion(QDataStream::Qt_4_2);

    QVariant item;
    if (_useCompression) {
        QByteArray rawItem;
        if (!Serializers::deserialize(stream, features(), rawItem)) {
            close("Peer sent corrupt data: unable to load QVariant!");
            return;
        }

        int nbytes = rawItem.size();
        if (nbytes <= 4) {
            const char* data = rawItem.constData();
            if (nbytes < 4 || (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0)) {
                close("Peer sent corrupted compressed data!");
                return;
            }
        }

        rawItem = qUncompress(rawItem);

        QDataStream itemStream(&rawItem, QIODevice::ReadOnly);
        itemStream.setVersion(QDataStream::Qt_4_2);
        if (!Serializers::deserialize(itemStream, features(), item)) {
            close("Peer sent corrupt data: unable to load QVariant!");
            return;
        }
    }
    else {
        if (!Serializers::deserialize(stream, features(), item)) {
            close("Peer sent corrupt data: unable to load QVariant!");
            return;
        }
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

void LegacyPeer::writeMessage(const QVariant& item)
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

void LegacyPeer::handleHandshakeMessage(const QVariant& msg)
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
        uint ver = m["ProtocolVersion"].toUInt();  // actually an UInt
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
        handle(SetupData(map["AdminUser"].toString(),
                         map["AdminPasswd"].toString(),
                         map["Backend"].toString(),
                         map["ConnectionProperties"].toMap(),
                         map["Authenticator"].toString(),
                         map["AuthProperties"].toMap()));
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

void LegacyPeer::dispatch(const RegisterClient& msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientInit";
    m["Features"] = static_cast<quint32>(msg.features.toLegacyFeatures());
    m["FeatureList"] = msg.features.toStringList();
    m["ClientVersion"] = msg.clientVersion;
    m["ClientDate"] = msg.buildDate;

    // FIXME only in compat mode
    m["ProtocolVersion"] = protocolVersion;
    m["UseSsl"] = true;
#ifndef QT_NO_COMPRESS
    m["UseCompression"] = true;
#else
    m["UseCompression"] = false;
#endif

    writeMessage(m);
}

void LegacyPeer::dispatch(const ClientDenied& msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientInitReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}

void LegacyPeer::dispatch(const ClientRegistered& msg)
{
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
    m["SupportsCompression"] = socket()->property("UseCompression").toBool();  // this property gets already set in the ClientInit handler

    // This is only used for display by really old v10 clients (pre-0.5), and we no longer set this
    m["CoreInfo"] = QString();

    m["LoginEnabled"] = m["Configured"] = msg.coreConfigured;

    writeMessage(m);
}

void LegacyPeer::dispatch(const SetupData& msg)
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

void LegacyPeer::dispatch(const SetupFailed& msg)
{
    QVariantMap m;
    m["MsgType"] = "CoreSetupReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}

void LegacyPeer::dispatch(const SetupDone& msg)
{
    Q_UNUSED(msg)

    QVariantMap m;
    m["MsgType"] = "CoreSetupAck";

    writeMessage(m);
}

void LegacyPeer::dispatch(const Login& msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientLogin";
    m["User"] = msg.user;
    m["Password"] = msg.password;

    writeMessage(m);
}

void LegacyPeer::dispatch(const LoginFailed& msg)
{
    QVariantMap m;
    m["MsgType"] = "ClientLoginReject";
    m["Error"] = msg.errorString;

    writeMessage(m);
}

void LegacyPeer::dispatch(const LoginSuccess& msg)
{
    Q_UNUSED(msg)

    QVariantMap m;
    m["MsgType"] = "ClientLoginAck";

    writeMessage(m);
}

void LegacyPeer::dispatch(const SessionState& msg)
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

void LegacyPeer::handlePackedFunc(const QVariant& packedFunc)
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
        QByteArray signalName = params.takeFirst().toByteArray();
        handle(Protocol::RpcCall(signalName, params));
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

        qDebug() << "[DEBUG] LegacyPeer::handlePackedFunc - Received InitData for class:" << className << "objectName:" << objectName;
        qDebug() << "[DEBUG] LegacyPeer::handlePackedFunc - initData keys:" << initData.keys();

        // we need to special-case IrcUsersAndChannels here, since the format changed
        if (className == "Network") {
            qDebug() << "[DEBUG] LegacyPeer::handlePackedFunc - Processing Network InitData, calling fromLegacyIrcUsersAndChannels";
            fromLegacyIrcUsersAndChannels(initData);
        } else {
            qDebug() << "[DEBUG] LegacyPeer::handlePackedFunc - No conversion needed for class:" << className;
        }
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

void LegacyPeer::dispatch(const Protocol::SyncMessage& msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)Sync << msg.className << msg.objectName << msg.slotName << msg.params);
}

void LegacyPeer::dispatch(const Protocol::RpcCall& msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)RpcCall << msg.signalName << msg.params);
}

void LegacyPeer::dispatch(const Protocol::InitRequest& msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)InitRequest << msg.className << msg.objectName);
}

void LegacyPeer::dispatch(const Protocol::InitData& msg)
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

void LegacyPeer::dispatch(const Protocol::HeartBeat& msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeat << msg.timestamp.time());
}

void LegacyPeer::dispatch(const Protocol::HeartBeatReply& msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeatReply << msg.timestamp.time());
}

void LegacyPeer::dispatchPackedFunc(const QVariantList& packedFunc)
{
    writeMessage(QVariant(packedFunc));
}

// Handle the changed format for Network's initData
// cf. Network::initIrcUsersAndChannels()
void LegacyPeer::fromLegacyIrcUsersAndChannels(QVariantMap& initData)
{
    qDebug() << "=== fromLegacyIrcUsersAndChannels START ===";
    qDebug() << "[DEBUG] initData keys:" << initData.keys();
    
    if (!initData.contains("IrcUsersAndChannels")) {
        qDebug() << "[DEBUG] ERROR: initData missing 'IrcUsersAndChannels' key!";
        qDebug() << "=== fromLegacyIrcUsersAndChannels END (ERROR) ===";
        return;
    }
    
    const QVariantMap& legacyMap = initData["IrcUsersAndChannels"].toMap();
    qDebug() << "[DEBUG] legacyMap keys:" << legacyMap.keys();
    qDebug() << "[DEBUG] legacyMap['users'] exists:" << legacyMap.contains("users");
    qDebug() << "[DEBUG] legacyMap['channels'] exists:" << legacyMap.contains("channels");
    qDebug() << "[DEBUG] legacyMap['Users'] exists:" << legacyMap.contains("Users");
    qDebug() << "[DEBUG] legacyMap['Channels'] exists:" << legacyMap.contains("Channels");
    
    if (legacyMap.contains("users")) {
        qDebug() << "[DEBUG] legacyMap['users'] size:" << legacyMap["users"].toMap().size();
        if (legacyMap["users"].toMap().size() > 0) {
            qDebug() << "[DEBUG] Sample user keys:" << legacyMap["users"].toMap().keys().mid(0, 3);
            QVariantMap users = legacyMap["users"].toMap();
            if (!users.isEmpty()) {
                auto firstUser = users.constBegin();
                qDebug() << "[DEBUG] First user key:" << firstUser.key();
                qDebug() << "[DEBUG] First user value type:" << firstUser.value().typeName();
                qDebug() << "[DEBUG] First user value as map keys:" << firstUser.value().toMap().keys();
            }
        }
    }
    if (legacyMap.contains("channels")) {
        qDebug() << "[DEBUG] legacyMap['channels'] size:" << legacyMap["channels"].toMap().size();
        if (legacyMap["channels"].toMap().size() > 0) {
            qDebug() << "[DEBUG] Sample channel keys:" << legacyMap["channels"].toMap().keys().mid(0, 3);
        }
    }
    QVariantMap newMap;

    // Qt5 format stores user data transposed: each property maps to a list of values for all users
    // Property "nick" contains [nick1, nick2, nick3...], "host" contains [host1, host2, host3...], etc.
    // We need to transpose this back to: each user hostmask maps to a complete user object
    
    QVariantMap userMap;
    QString usersKey = legacyMap.contains("Users") ? "Users" : "users";
    if (legacyMap.contains(usersKey)) {
        const QVariantMap& usersData = legacyMap[usersKey].toMap();
        qDebug() << "[DEBUG] LegacyPeer: Qt5 users data contains properties:" << usersData.keys();
        
        // Get the nick property to determine how many users we have
        if (usersData.contains("nick")) {
            const QVariantList nickList = usersData["nick"].toList();
            qDebug() << "[DEBUG] Found" << nickList.size() << "users in Qt5 format";
            
            // For each user index, build a complete user object
            for (int i = 0; i < nickList.size(); ++i) {
                QVariantMap userObject;
                
                // Extract each property for this user index
                for (auto it = usersData.constBegin(); it != usersData.constEnd(); ++it) {
                    const QVariantList propertyList = it.value().toList();
                    if (i < propertyList.size()) {
                        userObject[it.key()] = propertyList[i];
                    }
                }
                
                // Use nick!user@host as the key for Qt6 format
                QString nick = userObject["nick"].toString();
                QString user = userObject["user"].toString();
                QString host = userObject["host"].toString();
                QString hostmask = QString("%1!%2@%3").arg(nick, user, host);
                
                userMap[hostmask.toLower()] = userObject;
                qDebug() << "[DEBUG] LegacyPeer: Converted Qt5 user" << i << "nick:" << nick << "to hostmask:" << hostmask.toLower();
            }
        }
    }
    newMap["IrcUsers"] = userMap;

    // Same transposed format for channels
    QVariantMap channelMap;
    QString channelsKey = legacyMap.contains("Channels") ? "Channels" : "channels";
    if (legacyMap.contains(channelsKey)) {
        const QVariantMap& channelsData = legacyMap[channelsKey].toMap();
        qDebug() << "[DEBUG] LegacyPeer: Qt5 channels data contains properties:" << channelsData.keys();
        
        // Get the name property to determine how many channels we have
        if (channelsData.contains("name")) {
            const QVariantList nameList = channelsData["name"].toList();
            qDebug() << "[DEBUG] Found" << nameList.size() << "channels in Qt5 format";
            
            // For each channel index, build a complete channel object
            for (int i = 0; i < nameList.size(); ++i) {
                QVariantMap channelObject;
                
                // Extract each property for this channel index
                for (auto it = channelsData.constBegin(); it != channelsData.constEnd(); ++it) {
                    const QVariantList propertyList = it.value().toList();
                    if (i < propertyList.size()) {
                        channelObject[it.key()] = propertyList[i];
                    }
                }
                
                // Use channel name as the key for Qt6 format
                QString channelName = channelObject["name"].toString();
                channelMap[channelName.toLower()] = channelObject;
                qDebug() << "[DEBUG] LegacyPeer: Converted Qt5 channel" << i << "name:" << channelName;
            }
        }
    }
    newMap["IrcChannels"] = channelMap;

    qDebug() << "[DEBUG] Generated newMap['IrcUsers'] size:" << newMap["IrcUsers"].toMap().size();
    qDebug() << "[DEBUG] Generated newMap['IrcChannels'] size:" << newMap["IrcChannels"].toMap().size();
    
    initData["IrcUsersAndChannels"] = newMap;
    qDebug() << "=== fromLegacyIrcUsersAndChannels END (SUCCESS) ===";
}

void LegacyPeer::toLegacyIrcUsersAndChannels(QVariantMap& initData)
{
    qDebug() << "=== toLegacyIrcUsersAndChannels START ===";
    qDebug() << "[DEBUG] initData keys:" << initData.keys();
    
    const QVariantMap& usersAndChannels = initData["IrcUsersAndChannels"].toMap();
    qDebug() << "[DEBUG] usersAndChannels keys:" << usersAndChannels.keys();
    
    QVariantMap legacyMap;

    // Convert Qt6 format to legacy format
    // Qt6: IrcUsers -> map[nick] = complete user data
    // Legacy: users -> map[hostmask] = complete user data
    
    QVariantMap userMap;
    const QVariantMap& users = usersAndChannels["IrcUsers"].toMap();
    qDebug() << "[DEBUG] Converting" << users.size() << "IrcUsers to legacy format";
    
    for (auto it = users.constBegin(); it != users.constEnd(); ++it) {
        QVariantMap userData = it.value().toMap();
        QString nick = userData["nick"].toString();
        QString user = userData["user"].toString();
        QString host = userData["host"].toString();
        QString hostmask = QString("%1!%2@%3").arg(nick, user, host);
        userMap[hostmask.toLower()] = userData;
        qDebug() << "[DEBUG] Converted user:" << it.key() << "to hostmask:" << hostmask.toLower();
    }
    legacyMap["users"] = userMap;

    // Convert Qt6 format to legacy format  
    // Qt6: IrcChannels -> map[channelname] = complete channel data
    // Legacy: channels -> map[channelname] = complete channel data (same format)
    
    QVariantMap channelMap;
    const QVariantMap& channels = usersAndChannels["IrcChannels"].toMap();
    qDebug() << "[DEBUG] Converting" << channels.size() << "IrcChannels to legacy format";
    
    for (auto it = channels.constBegin(); it != channels.constEnd(); ++it) {
        QVariantMap channelData = it.value().toMap();
        QString channelName = channelData["name"].toString();
        channelMap[channelName.toLower()] = channelData;
        qDebug() << "[DEBUG] Converted channel:" << it.key() << "to name:" << channelName.toLower();
    }
    legacyMap["channels"] = channelMap;

    qDebug() << "[DEBUG] Generated legacy map with" << userMap.size() << "users and" << channelMap.size() << "channels";
    initData["IrcUsersAndChannels"] = legacyMap;
    qDebug() << "=== toLegacyIrcUsersAndChannels END ===";
}
