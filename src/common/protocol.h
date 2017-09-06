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

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <QByteArray>
#include <QDateTime>
#include <QVariantList>

namespace Protocol {

const quint32 magic = 0x42b33f00;

enum Type {
    InternalProtocol = 0x00,
    LegacyProtocol = 0x01,
    DataStreamProtocol = 0x02
};


enum Feature {
    Encryption = 0x01,
    Compression = 0x02
};


enum class Handler {
    SignalProxy,
    AuthHandler
};


/*** Handshake, handled by AuthHandler ***/

struct HandshakeMessage {
    inline Handler handler() const { return Handler::AuthHandler; }
};


struct RegisterClient : public HandshakeMessage
{
    inline RegisterClient(const QString &clientVersion, const QString &buildDate, bool sslSupported = false, int32_t features = 0)
    : clientVersion(clientVersion)
    , buildDate(buildDate)
    , sslSupported(sslSupported)
    , clientFeatures(features){}

    QString clientVersion;
    QString buildDate;

    // this is only used by the LegacyProtocol in compat mode
    bool sslSupported;
    int32_t clientFeatures;
};


struct ClientDenied : public HandshakeMessage
{
    inline ClientDenied(const QString &errorString)
    : errorString(errorString) {}

    QString errorString;
};


struct ClientRegistered : public HandshakeMessage
{
    inline ClientRegistered(quint32 coreFeatures, bool coreConfigured, const QVariantList &backendInfo, bool sslSupported, const QVariantList &authenticatorInfo)
    : coreFeatures(coreFeatures)
    , coreConfigured(coreConfigured)
    , backendInfo(backendInfo)
    , authenticatorInfo(authenticatorInfo)
    , sslSupported(sslSupported)
    {}

    quint32 coreFeatures;
    bool coreConfigured;

    // The authenticatorInfo should be optional!
    QVariantList backendInfo; // TODO: abstract this better
    QVariantList authenticatorInfo;

    // this is only used by the LegacyProtocol in compat mode
    bool sslSupported;
};


struct SetupData : public HandshakeMessage
{
    inline SetupData(const QString &adminUser, const QString &adminPassword, const QString &backend,
                     const QVariantMap &setupData, const QString &authenticator = QString(),
                     const QVariantMap &authSetupData = QVariantMap())
    : adminUser(adminUser)
    , adminPassword(adminPassword)
    , backend(backend)
    , setupData(setupData)
    , authenticator(authenticator)
    , authSetupData(authSetupData)
    {}

    QString adminUser;
    QString adminPassword;
    QString backend;
    QVariantMap setupData;
    QString authenticator;
    QVariantMap authSetupData;
};


struct SetupFailed : public HandshakeMessage
{
    inline SetupFailed(const QString &errorString)
    : errorString(errorString) {}

    QString errorString;
};


struct SetupDone : public HandshakeMessage
{
    inline SetupDone() {}
};


struct Login : public HandshakeMessage
{
    inline Login(const QString &user, const QString &password)
    : user(user), password(password) {}

    QString user;
    QString password;
};


struct LoginFailed : public HandshakeMessage
{
    inline LoginFailed(const QString &errorString)
    : errorString(errorString) {}

    QString errorString;
};


struct LoginSuccess : public HandshakeMessage
{
    inline LoginSuccess() {}
};


// TODO: more generic format
struct SessionState : public HandshakeMessage
{
    inline SessionState() {} // needed for QMetaType (for the mono client)
    inline SessionState(const QVariantList &identities, const QVariantList &bufferInfos, const QVariantList &networkIds)
    : identities(identities), bufferInfos(bufferInfos), networkIds(networkIds) {}

    QVariantList identities;
    QVariantList bufferInfos;
    QVariantList networkIds;
};

/*** handled by SignalProxy ***/

struct SignalProxyMessage
{
    inline Handler handler() const { return Handler::SignalProxy; }
};


struct SyncMessage : public SignalProxyMessage
{
    inline SyncMessage(const QByteArray &className, const QString &objectName, const QByteArray &slotName, const QVariantList &params)
    : className(className), objectName(objectName), slotName(slotName), params(params) {}

    QByteArray className;
    QString objectName;
    QByteArray slotName;
    QVariantList params;
};


struct RpcCall : public SignalProxyMessage
{
    inline RpcCall(const QByteArray &slotName, const QVariantList &params)
    : slotName(slotName), params(params) {}

    QByteArray slotName;
    QVariantList params;
};


struct InitRequest : public SignalProxyMessage
{
    inline InitRequest(const QByteArray &className, const QString &objectName)
    : className(className), objectName(objectName) {}

    QByteArray className;
    QString objectName;
};


struct InitData : public SignalProxyMessage
{
    inline InitData(const QByteArray &className, const QString &objectName, const QVariantMap &initData)
    : className(className), objectName(objectName), initData(initData) {}

    QByteArray className;
    QString objectName;
    QVariantMap initData;
};


/*** handled by RemoteConnection ***/

struct HeartBeat
{
    inline HeartBeat(const QDateTime &timestamp) : timestamp(timestamp) {}

    QDateTime timestamp;
};


struct HeartBeatReply
{
    inline HeartBeatReply(const QDateTime &timestamp) : timestamp(timestamp) {}

    QDateTime timestamp;
};


};

#endif
