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

#ifndef PEER_H
#define PEER_H

#include <QAbstractSocket>

#include "protocol.h"
#include "signalproxy.h"

class Peer : public QObject
{
    Q_OBJECT

public:
    Peer(QObject *parent = 0) : QObject(parent) {}

    virtual QString description() const = 0;

    virtual SignalProxy *signalProxy() const = 0;
    virtual void setSignalProxy(SignalProxy *proxy) = 0;

    virtual bool isOpen() const = 0;
    virtual bool isSecure() const = 0;
    virtual bool isLocal() const = 0;

    virtual int lag() const = 0;

public slots:
    virtual void dispatch(const Protocol::SyncMessage &msg) = 0;
    virtual void dispatch(const Protocol::RpcCall &msg) = 0;
    virtual void dispatch(const Protocol::InitRequest &msg) = 0;
    virtual void dispatch(const Protocol::InitData &msg) = 0;

    virtual void close(const QString &reason = QString()) = 0;

signals:
    void disconnected();
    void error(QAbstractSocket::SocketError);
    void secureStateChanged(bool secure = true);
    void lagUpdated(int msecs);

protected:
    template<class T>
    void handle(const T &protoMessage);
};


// Template method needed in the header
template<class T> inline
void Peer::handle(const T &protoMessage)
{
    switch(protoMessage.handler()) {
        case Protocol::SignalProxy:
            if (!signalProxy()) {
                qWarning() << Q_FUNC_INFO << "Cannot handle message without a SignalProxy!";
                return;
            }
            signalProxy()->handle(this, protoMessage);
            break;

        default:
            qWarning() << Q_FUNC_INFO << "Unknown handler for protocol message!";
            return;
    }
}

#endif
