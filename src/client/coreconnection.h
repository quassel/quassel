/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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
#include "types.h"

class CoreAccountModel;
class Network;
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

    inline bool isConnected() const;
    inline ConnectionState state() const;
    inline CoreAccount currentAccount() const;

    bool isEncrypted() const;
    bool isLocalConnection() const;

    inline int progressMinimum() const;
    inline int progressMaximum() const;
    inline int progressValue() const;
    inline QString progressText() const;

    //! Check if we consider the last connect as reconnect
    inline bool wasReconnect() const { return _wasReconnect; }

#ifdef HAVE_SSL
    inline const QSslSocket *sslSocket() const;
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
    void connectToInternalCore(SignalProxy *proxy);

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

    QPointer<QAbstractSocket> _socket;
    quint32 _blockSize;
    ConnectionState _state;

    QTimer _reconnectTimer;
    bool _wantReconnect;

    QSet<QObject *> _netsToSync;
    int _numNetsToSync;
    int _progressMinimum, _progressMaximum, _progressValue;
    QString _progressText;

    QString _coreInfoString(const QVariantMap &);
    bool _wasReconnect;
    bool _requestedDisconnect;

    inline CoreAccountModel *accountModel() const;

    friend class CoreConfigWizard;
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

#ifdef HAVE_SSL
const QSslSocket *CoreConnection::sslSocket() const { return qobject_cast<QSslSocket *>(_socket); }
#endif

#endif
