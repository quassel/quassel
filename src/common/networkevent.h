/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#ifndef NETWORKEVENT_H
#define NETWORKEVENT_H

#include <QStringList>
#include <QVariantList>

#include "event.h"
#include "network.h"

class NetworkEvent : public Event {

public:
  explicit NetworkEvent(EventManager::EventType type, Network *network)
    : Event(type),
      _network(network)
  {}

  inline NetworkId networkId() const { return network()? network()->networkId() : NetworkId(); }
  inline Network *network() const { return _network; }

protected:
  virtual inline QString className() const { return "NetworkEvent"; }
  virtual inline void debugInfo(QDebug &dbg) const { dbg.nospace() << ", net = " << qPrintable(_network->networkName()); }

private:
  Network *_network;
};

class NetworkConnectionEvent : public NetworkEvent {

public:
  explicit NetworkConnectionEvent(EventManager::EventType type, Network *network, Network::ConnectionState state)
    : NetworkEvent(type, network),
      _state(state)
  {}

  inline Network::ConnectionState connectionState() const { return _state; }
  inline void setConnectionState(Network::ConnectionState state) { _state = state; }

protected:
  virtual inline QString className() const { return "NetworkConnectionEvent"; }
  virtual inline void debugInfo(QDebug &dbg) const {
    NetworkEvent::debugInfo(dbg);
    dbg.nospace() << ", state = " << qPrintable(QString::number(_state));
  }

private:
  Network::ConnectionState _state;
};

class NetworkDataEvent : public NetworkEvent {

public:
  explicit NetworkDataEvent(EventManager::EventType type, Network *network, const QByteArray &data)
    : NetworkEvent(type, network),
      _data(data)
  {}

  inline QByteArray data() const { return _data; }
  inline void setData(const QByteArray &data) { _data = data; }

protected:
  virtual inline QString className() const { return "NetworkDataEvent"; }
  virtual inline void debugInfo(QDebug &dbg) const {
    NetworkEvent::debugInfo(dbg);
    dbg.nospace() << ", data = " << data();
  }

private:
  QByteArray _data;
};

class NetworkSplitEvent : public NetworkEvent {

public:
  explicit NetworkSplitEvent(EventManager::EventType type,
                             Network *network,
                             const QString &channel,
                             const QStringList &users,
                             const QString &quitMsg)
    : NetworkEvent(type, network),
      _channel(channel),
      _users(users),
      _quitMsg(quitMsg)
  {}

  inline QString channel() const { return _channel; }
  inline QStringList users() const { return _users; }
  inline QString quitMessage() const { return _quitMsg; }

private:
  QString _channel;
  QStringList _users;
  QString _quitMsg;
};


#endif
