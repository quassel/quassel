/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef CORECONNECTION_H_
#define CORECONNECTION_H_

// TODO: support system application proxy (new in Qt 4.6)

#include "QPointer"
#include "QTimer"

#ifdef HAVE_SSL
#  include <QSslSocket>
#else
#  include <QTcpSocket>
#endif

#ifdef HAVE_KDE
#  include <Solid/Networking>
#endif

#include "coreaccount.h"
#include "remotepeer.h"
#include "types.h"

class CoreAccountModel;
class InternalPeer;
class Network;
class Peer;
class SignalProxy;

class CoreConnection : public QObject
{
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

    bool isConnected() const;
    ConnectionState state() const;
    CoreAccount currentAccount() const;

    bool isEncrypted() const;
    bool isLocalConnection() const;

    int progressMinimum() const;
    int progressMaximum() const;
    int progressValue() const;
    QString progressText() const;

    //! Check if we consider the last connect as reconnect
    bool wasReconnect() const { return _wasReconnect; }

#ifdef HAVE_SSL
    const QSslSocket *sslSocket() const;
#endif

public slots:
    bool connectToCore(AccountId = 0);
    void reconnectToCore();
    void disconnectFromCore();

signals:
    void stateChanged(CoreConnection::ConnectionState);
    void encrypted(bool isEncrypted = true);
    void synchronized();
    void lagUpdated(int msecs);

    void connectionError(const QString &errorMsg);
    void connectionErrorPopup(const QString &errorMsg);
    void connectionWarnings(const QStringList &warnings);
    void connectionMsg(const QString &msg);
    void disconnected();

    void progressRangeChanged(int minimum, int maximum);
    void progressValueChanged(int value);
    void progressTextChanged(const QString &);

    void startCoreSetup(const QVariantList &);
    void coreSetupSuccess();
    void coreSetupFailed(const QString &error);

    void startInternalCore();
    void connectToInternalCore(InternalPeer *connection);

    // These signals MUST be handled synchronously!
    void userAuthenticationRequired(CoreAccount *, bool *valid, const QString &errorMessage = QString());
    void handleNoSslInClient(bool *accepted);
    void handleNoSslInCore(bool *accepted);
#ifdef HAVE_SSL
    void handleSslErrors(const QSslSocket *socket, bool *accepted, bool *permanently);
#endif

private slots:
    void connectToCurrentAccount();
    void disconnectFromCore(const QString &errorString, bool wantReconnect = true);

    void socketStateChanged(QAbstractSocket::SocketState);
    void coreSocketError(QAbstractSocket::SocketError);
    void coreHasData(const QVariant &item);
    void coreSocketConnected();
    void coreSocketDisconnected();

    void clientInitAck(const QVariantMap &msg);

    // for sync progress
    void networkInitDone();
    void checkSyncState();

    void syncToCore(const QVariantMap &sessionState);
    void internalSessionStateReceived(const QVariant &packedState);
    void sessionStateReceived(const QVariantMap &state);

    void resetConnection(bool wantReconnect = false);
    void connectionReady();

    void loginToCore(const QString &user, const QString &password, bool remember); // for config wizard
    void loginToCore(const QString &previousError = QString());
    void loginSuccess();
    void loginFailed(const QString &errorMessage);

    void doCoreSetup(const QVariant &setupData);

    void updateProgress(int value, int maximum);
    void setProgressText(const QString &text);
    void setProgressValue(int value);
    void setProgressMinimum(int minimum);
    void setProgressMaximum(int maximum);

    void setState(QAbstractSocket::SocketState socketState);
    void setState(ConnectionState state);

#ifdef HAVE_SSL
    void sslSocketEncrypted();
    void sslErrors();
#endif

    void networkDetectionModeChanged(const QVariant &mode);
    void pingTimeoutIntervalChanged(const QVariant &interval);
    void reconnectIntervalChanged(const QVariant &interval);
    void reconnectTimeout();

#ifdef HAVE_KDE
    void solidNetworkStatusChanged(Solid::Networking::Status status);
#endif

private:
    CoreAccountModel *_model;
    CoreAccount _account;
    QVariantMap _coreMsgBuffer;

    QPointer<QTcpSocket> _socket;
    QPointer<Peer> _peer;
    ConnectionState _state;

    QTimer _reconnectTimer;
    bool _wantReconnect;
    bool _wasReconnect;

    QSet<QObject *> _netsToSync;
    int _numNetsToSync;
    int _progressMinimum, _progressMaximum, _progressValue;
    QString _progressText;

    QString _coreInfoString(const QVariantMap &);
    bool _resetting;

    inline CoreAccountModel *accountModel() const;

    friend class CoreConfigWizard;
};


Q_DECLARE_METATYPE(CoreConnection::ConnectionState)

// Inlines
inline int CoreConnection::progressMinimum() const { return _progressMinimum; }
inline int CoreConnection::progressMaximum() const { return _progressMaximum; }
inline int CoreConnection::progressValue() const { return _progressValue; }
inline QString CoreConnection::progressText() const { return _progressText; }

inline CoreConnection::ConnectionState CoreConnection::state() const { return _state; }
inline bool CoreConnection::isConnected() const { return state() >= Connected; }
inline CoreAccount CoreConnection::currentAccount() const { return _account; }
inline CoreAccountModel *CoreConnection::accountModel() const { return _model; }

#ifdef HAVE_SSL
inline const QSslSocket *CoreConnection::sslSocket() const { return qobject_cast<QSslSocket *>(_socket); }
#endif

#endif
