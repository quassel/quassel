/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#ifndef _COREPROXY_H_
#define _COREPROXY_H_

#include "proxy_common.h"
#include "message.h"
#include "global.h"

//#include <QtCore>
#include <QStringList>
#include <QDebug>

/** This class is the Core side of the proxy. The Core connects its signals and slots to it,
 *  and the calls are marshalled and sent to (or received and unmarshalled from) the GuiProxy.
 *  The connection functions are defined in main/main_core.cpp or main/main_mono.cpp.
 */
class CoreProxy : public QObject {
  Q_OBJECT

  public:
    CoreProxy();

  public slots:
    inline void csSessionDataChanged(const QString &key, const QVariant &data) { send(CS_SESSION_DATA_CHANGED, key, data); }
    inline void csServerConnected(QString net)                          { send(CS_SERVER_CONNECTED, net); }
    inline void csServerDisconnected(QString net)                       { send(CS_SERVER_DISCONNECTED, net); }
    inline void csServerState(QString net, QVariantMap data)                 { send(CS_SERVER_STATE, net, data); }
    inline void csDisplayMsg(Message msg)                               { send(CS_DISPLAY_MSG, QVariant::fromValue(msg));}
    inline void csDisplayStatusMsg(QString net, QString msg)            { send(CS_DISPLAY_STATUS_MSG, net, msg); }
    inline void csModeSet(QString net, QString target, QString mode)    { send(CS_MODE_SET, net, target, mode); }
    inline void csTopicSet(QString net, QString buf, QString topic)     { send(CS_TOPIC_SET, net, buf, topic); }
    inline void csNickAdded(QString net, QString nick, QVariantMap props)    { send(CS_NICK_ADDED, net, nick, props); }
    inline void csNickRemoved(QString net, QString nick)                { send(CS_NICK_REMOVED, net, nick); }
    inline void csNickRenamed(QString net, QString oldn, QString newn)  { send(CS_NICK_RENAMED, net, oldn, newn); }
    inline void csNickUpdated(QString net, QString nick, QVariantMap props)  { send(CS_NICK_UPDATED, net, nick, props); }
    inline void csOwnNickSet(QString net, QString nick)                 { send(CS_OWN_NICK_SET, net, nick); }
    inline void csQueryRequested(QString net, QString nick)             { send(CS_QUERY_REQUESTED, net, nick); }
    inline void csBacklogData(BufferId id, QList<QVariant> msg, bool done) { send(CS_BACKLOG_DATA, QVariant::fromValue(id), msg, done); }
    inline void csUpdateBufferId(BufferId id)                           { send(CS_UPDATE_BUFFERID, QVariant::fromValue(id)); }

    inline void csGeneric(CoreSignal sig, QVariant v1 = QVariant(), QVariant v2 = QVariant(), QVariant v3 = QVariant()) { send(sig, v1, v2, v3); }

  signals:
    void gsSessionDataChanged(const QString &, const QVariant &);
    void gsUserInput(BufferId, QString);
    void gsRequestConnect(QStringList networks);
    void gsImportBacklog();
    void gsRequestBacklog(BufferId, QVariant, QVariant);
    void gsRequestNetworkStates();

    void gsGeneric(ClientSignal, QVariant, QVariant, QVariant);

    void requestServerStates();

    void send(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  public:
    void recv(ClientSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());


};


#endif
