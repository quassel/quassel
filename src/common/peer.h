/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "common-export.h"

#include <QAbstractSocket>
#include <QDataStream>
#include <QPointer>

#include "authhandler.h"
#include "protocol.h"
#include "quassel.h"
#include "signalproxy.h"

class COMMON_EXPORT Peer : public QObject
{
    Q_OBJECT

public:
    explicit Peer(AuthHandler *authHandler, QObject *parent = nullptr);

    virtual Protocol::Type protocol() const = 0;
    virtual QString description() const = 0;

    virtual SignalProxy *signalProxy() const = 0;
    virtual void setSignalProxy(SignalProxy *proxy) = 0;

    QDateTime connectedSince() const;
    void setConnectedSince(const QDateTime &connectedSince);

    QString buildDate() const;
    void setBuildDate(const QString &buildDate);

    QString clientVersion() const;
    void setClientVersion(const QString &clientVersion);

    bool hasFeature(Quassel::Feature feature) const;
    Quassel::Features features() const;
    void setFeatures(Quassel::Features features);

    int id() const;
    void setId(int id);

    AuthHandler *authHandler() const;

    virtual bool isOpen() const = 0;
    virtual bool isSecure() const = 0;
    virtual bool isLocal() const = 0;

    virtual int lag() const = 0;

    virtual QString address() const = 0;
    virtual quint16 port() const = 0;

public slots:
    /* Handshake messages */
    virtual void dispatch(const Protocol::RegisterClient &) = 0;
    virtual void dispatch(const Protocol::ClientDenied &) = 0;
    virtual void dispatch(const Protocol::ClientRegistered &) = 0;
    virtual void dispatch(const Protocol::SetupData &) = 0;
    virtual void dispatch(const Protocol::SetupFailed &) = 0;
    virtual void dispatch(const Protocol::SetupDone &) = 0;
    virtual void dispatch(const Protocol::Login &) = 0;
    virtual void dispatch(const Protocol::LoginFailed &) = 0;
    virtual void dispatch(const Protocol::LoginSuccess &) = 0;
    virtual void dispatch(const Protocol::SessionState &) = 0;

    /* Sigproxy messages */
    virtual void dispatch(const Protocol::SyncMessage &) = 0;
    virtual void dispatch(const Protocol::RpcCall &) = 0;
    virtual void dispatch(const Protocol::InitRequest &) = 0;
    virtual void dispatch(const Protocol::InitData &) = 0;

    virtual void close(const QString &reason = QString()) = 0;

signals:
    void disconnected();
    void secureStateChanged(bool secure = true);
    void lagUpdated(int msecs);

protected:
    template<typename T>
    void handle(const T &protoMessage);

private:
    QPointer<AuthHandler> _authHandler;

    QDateTime _connectedSince;

    QString _buildDate;
    QString _clientVersion;
    Quassel::Features _features;

    int _id = -1;
};

// We need to special-case Peer* in attached signals/slots, so typedef it for the meta type system
typedef Peer * PeerPtr;
Q_DECLARE_METATYPE(PeerPtr)

QDataStream &operator<<(QDataStream &out, PeerPtr ptr);
QDataStream &operator>>(QDataStream &in, PeerPtr &ptr);


// Template method needed in the header
template<typename T> inline
void Peer::handle(const T &protoMessage)
{
    switch(protoMessage.handler()) {
        case Protocol::Handler::SignalProxy:
            if (!signalProxy()) {
                qWarning() << Q_FUNC_INFO << "Cannot handle message without a SignalProxy!";
                return;
            }
            signalProxy()->handle(this, protoMessage);
            break;

        case Protocol::Handler::AuthHandler:
            if (!authHandler()) {
                qWarning() << Q_FUNC_INFO << "Cannot handle auth messages without an active AuthHandler!";
                return;
            }
            authHandler()->handle(protoMessage);
            break;

        default:
            qWarning() << Q_FUNC_INFO << "Unknown handler for protocol message!";
            return;
    }
}
