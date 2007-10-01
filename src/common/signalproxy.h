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

#ifndef _RPCPEER_H_
#define _RPCPEER_H_

#include "qxtrpcpeer.h"
#include <QtCore>

class SignalProxy : public QObject {
  Q_OBJECT

  public:

    enum ProxyType { Client, Server };

    SignalProxy(ProxyType type, QIODevice *device = 0, QObject *parent = 0);
    ~SignalProxy();

    void attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction = QByteArray());
    void attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot);

  public slots:
    void addPeer(QIODevice *device);

    void sendSignal(const char *signal, QVariant p1 = QVariant(), QVariant p2 = QVariant(), QVariant p3 = QVariant(), QVariant p4 = QVariant(),
              QVariant p5 = QVariant(), QVariant p6 = QVariant(), QVariant p7 = QVariant(), QVariant p8 = QVariant(), QVariant p9 = QVariant());
  
    //void detachSender();
    void detachObject(QObject *);

  signals:
    void peerDisconnected();

  private slots:
    void socketDisconnected();

  private:
    struct Connection {
      QPointer<QxtRPCPeer> peer;
      QPointer<QIODevice> device;
    };

    struct SignalDesc {
      QObject *sender;
      const char *signal;
      QByteArray rpcFunction;

      SignalDesc(QObject *sndr, const char *sig, const QByteArray &func) : sender(sndr), signal(sig), rpcFunction(func) {}
    };

    struct SlotDesc {
      QByteArray rpcFunction;
      QObject *recv;
      const char *slot;

      SlotDesc(const QByteArray& func, QObject* r, const char* s) : rpcFunction(func), recv(r), slot(s) {}
    };

    ProxyType type;
    QList<Connection> peers;
    QList<SignalDesc> attachedSignals;
    QList<SlotDesc> attachedSlots;

};



#endif
