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

#ifndef CTCPPARSER_H
#define CTCPPARSER_H

#include <QUuid>

#include "corenetwork.h"
#include "eventmanager.h"
#include "ircevent.h"

class CoreSession;
class CtcpEvent;

class CtcpParser : public QObject {
  Q_OBJECT

public:
  CtcpParser(CoreSession *coreSession, QObject *parent = 0);

  inline CoreSession *coreSession() const { return _coreSession; }

  void query(CoreNetwork *network, const QString &bufname, const QString &ctcpTag, const QString &message);
  void reply(CoreNetwork *network, const QString &bufname, const QString &ctcpTag, const QString &message);

  Q_INVOKABLE void processIrcEventRawNotice(IrcEventRawMessage *event);
  Q_INVOKABLE void processIrcEventRawPrivmsg(IrcEventRawMessage *event);

  Q_INVOKABLE void sendCtcpEvent(CtcpEvent *event);

signals:
  void newEvent(Event *event);

protected:
  inline CoreNetwork *coreNetwork(NetworkEvent *e) const { return qobject_cast<CoreNetwork *>(e->network()); }

  // FIXME duplicates functionality in EventStringifier, maybe want to put that in something common
  //! Creates and sends a MessageEvent
  void displayMsg(NetworkEvent *event,
                  Message::Type msgType,
                  const QString &msg,
                  const QString &sender = QString(),
                  const QString &target = QString(),
                  Message::Flags msgFlags = Message::None);

  void parse(IrcEventRawMessage *event, Message::Type msgType);

  QByteArray lowLevelQuote(const QByteArray &);
  QByteArray lowLevelDequote(const QByteArray &);
  QByteArray xdelimQuote(const QByteArray &);
  QByteArray xdelimDequote(const QByteArray &);

  QByteArray pack(const QByteArray &ctcpTag, const QByteArray &message);
  void packedReply(CoreNetwork *network, const QString &bufname, const QList<QByteArray> &replies);

private:
  inline QString targetDecode(IrcEventRawMessage *e, const QByteArray &msg) { return coreNetwork(e)->userDecode(e->target(), msg); }

  CoreSession *_coreSession;

  struct CtcpReply {
    CoreNetwork *network;
    QString bufferName;
    QList<QByteArray> replies;

    CtcpReply() : network(0) {}
    CtcpReply(CoreNetwork *net, const QString &buf) : network(net), bufferName(buf) {}
  };

  QHash<QUuid, CtcpReply> _replies;

  QHash<QByteArray, QByteArray> _ctcpMDequoteHash;
  QHash<QByteArray, QByteArray> _ctcpXDelimDequoteHash;
};

#endif
