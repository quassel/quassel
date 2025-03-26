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

#pragma once

#include "authhandler.h"
#include "compressor.h"
#include "coreaccount.h"

class QSslSocket;

class RemotePeer;

class ClientAuthHandler : public AuthHandler
{
    Q_OBJECT

public:
    enum DigestVersion
    {
        Md5,
        Sha2_512,
        Latest = Sha2_512
    };

    ClientAuthHandler(CoreAccount account, QObject* parent = nullptr);

    Peer* peer() const;

public slots:
    void connectToCore();

    void login(const QString& previousError = QString());
    void login(const QString& user, const QString& password, bool remember);
    void setupCore(const Protocol::SetupData& setupData);

signals:
    void statusMessage(const QString& message);
    void errorMessage(const QString& message);
    void errorPopup(const QString& message);
    void transferProgress(int current, int max);

    void requestDisconnect(const QString& errorString = QString(), bool wantReconnect = false);

    void connectionReady();
    void loginSuccessful(const CoreAccount& account);
    void handshakeComplete(RemotePeer* peer, const Protocol::SessionState& sessionState);

    // These signals MUST be handled synchronously!
    void userAuthenticationRequired(CoreAccount* account, bool* valid, const QString& errorMessage = QString());
    void handleNoSslInClient(bool* accepted);
    void handleNoSslInCore(bool* accepted);
    void handleSslErrors(const QSslSocket* socket, bool* accepted, bool* permanently);

    void encrypted(bool isEncrypted = true);
    void startCoreSetup(const QVariantList& backendInfo, const QVariantList& authenticatorInfo);
    void coreSetupSuccessful();
    void coreSetupFailed(const QString& error);

private:
    using AuthHandler::handle;

    void handle(const Protocol::ClientDenied& msg) override;
    void handle(const Protocol::ClientRegistered& msg) override;
    void handle(const Protocol::SetupFailed& msg) override;
    void handle(const Protocol::SetupDone& msg) override;
    void handle(const Protocol::LoginFailed& msg) override;
    void handle(const Protocol::LoginSuccess& msg) override;
    void handle(const Protocol::SessionState& msg) override;

    void setPeer(RemotePeer* peer);
    void checkAndEnableSsl(bool coreSupportsSsl);
    void startRegistration();

private slots:
    void onSocketConnected();
    void onSocketStateChanged(QAbstractSocket::SocketState state);
    void onSocketError(QAbstractSocket::SocketError) override;
    void onSocketDisconnected() override;
    void onImplicitTLSSocketConnected();
    void onImplicitTlsSocketEncrypted();
    void onReadyRead();

    void onSslSocketEncrypted();
    void onSslErrors();

    void onProtocolVersionMismatch(int actual, int expected);

    void onConnectionReady();

private:
    RemotePeer* _peer;
    bool _coreConfigured;
    QVariantList _backendInfo;
    QVariantList _authenticatorInfo;
    CoreAccount _account;
    bool _probing;
    bool _legacy;
    bool _tls;
    quint8 _connectionFeatures;
};
