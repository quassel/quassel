/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "signalproxy.h"

SignalProxy::SignalProxy(ProxyType _type, QIODevice *dev, QObject *parent) : QObject(parent), type(_type) {
  if(dev) {
    if(type != Client) {
      qWarning() << tr("Device given for ProxyType == Server, ignoring...").toAscii();
    } else {
      addPeer(dev);
    }
  }
}

SignalProxy::~SignalProxy() {
  foreach(Connection conn, peers) {
    conn.peer->deleteLater(); conn.device->deleteLater();
  }
}

void SignalProxy::addPeer(QIODevice *dev) {
  if(type == Client && peers.count()) {
    qWarning() << tr("Cannot add more than one peer to a SignalProxy in client mode!").toAscii();
    return;
  }
  Connection conn;
  conn.device = dev;
  conn.peer = new QxtRPCPeer(dev, QxtRPCPeer::Peer, this);

  foreach(SlotDesc slot, attachedSlots) {
    conn.peer->attachSlot(slot.rpcFunction, slot.recv, slot.slot);
  }
  foreach(SignalDesc sig, attachedSignals) {
    conn.peer->attachSignal(sig.sender, sig.signal, sig.rpcFunction);
  }
  peers.append(conn);

}

void SignalProxy::attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction) {
  foreach(Connection conn, peers) {
    conn.peer->attachSignal(sender, signal, rpcFunction);
  }
  attachedSignals.append(SignalDesc(sender, signal, rpcFunction));

}

void SignalProxy::attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot) {
  foreach(Connection conn, peers) {
    conn.peer->attachSlot(rpcFunction, recv, slot);
  }
  attachedSlots.append(SlotDesc(rpcFunction, recv, slot));
}

void SignalProxy::detachObject(QObject* obj) {
  Q_ASSERT(false); // not done yet
  foreach(Connection conn, peers) {
    conn.peer->detachObject(obj);
  }
  // FIXME: delete attached signal/slot info

}

