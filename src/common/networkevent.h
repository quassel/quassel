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

#include <utility>

#include <QPointer>
#include <QStringList>
#include <QVariantList>

#include "event.h"
#include "network.h"

class COMMON_EXPORT NetworkEvent : public Event
{
    Q_OBJECT
public:
    explicit NetworkEvent(EventManager::EventType type, Network* network, QObject* parent = nullptr)
        : Event(type, parent)
        , _network(network)
    {}

    inline NetworkId networkId() const { return network() ? network()->networkId() : NetworkId(); }
    virtual inline Network* network() const { return _network; }

    static Event* create(EventManager::EventType type, QVariantMap& map, Network* network);

protected:
    explicit NetworkEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent = nullptr);
    virtual void toVariantMap(QVariantMap& map) const override;

    virtual inline QString className() const override { return "NetworkEvent"; }
    virtual inline void debugInfo(QDebug& dbg) const override { dbg.nospace() << ", net = " << qPrintable(_network ? _network->networkName() : "null"); }

    virtual ~NetworkEvent() override = default;

private:
    Network* _network;
};

/*****************************************************************************/

class COMMON_EXPORT NetworkDataEvent : public NetworkEvent
{
    Q_OBJECT
public:
    explicit NetworkDataEvent(EventManager::EventType type, Network* network, QByteArray data, QObject* parent = nullptr)
        : NetworkEvent(type, network, parent)
        , _data(std::move(data))
        , _networkPtr(network)
    {
        if (!network) {
            qWarning() << Q_FUNC_INFO << "NetworkDataEvent created with null network pointer!";
        }
    }

    inline QByteArray data() const { return _data; }
    inline void setData(const QByteArray& data) { _data = data; }
    inline Network* network() const override
    {
        Network* net = _networkPtr.data();
        qDebug() << Q_FUNC_INFO << "Accessing network pointer:" << net;
        return net;
    }

protected:
    explicit NetworkDataEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent = nullptr);
    void toVariantMap(QVariantMap& map) const override;

    inline QString className() const override { return "NetworkDataEvent"; }
    inline void debugInfo(QDebug& dbg) const override
    {
        NetworkEvent::debugInfo(dbg);
        dbg.nospace() << ", data = " << data();
    }

    virtual ~NetworkDataEvent() override = default;

private:
    QByteArray _data;
    QPointer<Network> _networkPtr;
    friend class NetworkEvent;
};

/*****************************************************************************/

class COMMON_EXPORT NetworkConnectionEvent : public NetworkEvent
{
    Q_OBJECT
public:
    explicit NetworkConnectionEvent(EventManager::EventType type, Network* network, Network::ConnectionState state, QObject* parent = nullptr)
        : NetworkEvent(type, network, parent)
        , _state(state)
    {}

    inline Network::ConnectionState connectionState() const { return _state; }
    inline void setConnectionState(Network::ConnectionState state) { _state = state; }

protected:
    explicit NetworkConnectionEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent = nullptr);
    void toVariantMap(QVariantMap& map) const override;

    inline QString className() const override { return "NetworkConnectionEvent"; }
    inline void debugInfo(QDebug& dbg) const override
    {
        NetworkEvent::debugInfo(dbg);
        dbg.nospace() << ", state = " << qPrintable(QString::number(_state));
    }

    virtual ~NetworkConnectionEvent() override = default;

private:
    Network::ConnectionState _state;
    friend class NetworkEvent;
};

/*****************************************************************************/

class COMMON_EXPORT NetworkSplitEvent : public NetworkEvent
{
    Q_OBJECT
public:
    explicit NetworkSplitEvent(EventManager::EventType type, Network* network, QString channel, QStringList users, QString quitMsg, QObject* parent = nullptr)
        : NetworkEvent(type, network, parent)
        , _channel(std::move(channel))
        , _users(std::move(users))
        , _quitMsg(std::move(quitMsg))
    {}

    inline QString channel() const { return _channel; }
    inline QStringList users() const { return _users; }
    inline QString quitMessage() const { return _quitMsg; }

protected:
    explicit NetworkSplitEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent = nullptr);
    void toVariantMap(QVariantMap& map) const override;

    inline QString className() const override { return "NetworkSplitEvent"; }
    inline void debugInfo(QDebug& dbg) const override
    {
        NetworkEvent::debugInfo(dbg);
        dbg.nospace() << ", channel = " << qPrintable(channel()) << ", users = " << users() << ", quitmsg = " << quitMessage();
    }

    virtual ~NetworkSplitEvent() override = default;

private:
    QString _channel;
    QStringList _users;
    QString _quitMsg;
    friend class NetworkEvent;
};
