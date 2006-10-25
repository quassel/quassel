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

#include <QtCore>
#include <QTcpSocket>
#include <QTcpServer>

/** This class is the Core side of the proxy. The Core connects its signals and slots to it,
 *  and the calls are marshalled and sent to (or received and unmarshalled from) the GUIProxy.
 *  The connection functions are defined in main/main_core.cpp or main/main_mono.cpp.
 */
class CoreProxy : public QObject {
  Q_OBJECT

  public:
    CoreProxy();

  public slots:
    inline void csUpdateGlobalData(QString key, QVariant data)          { send(CS_UPDATE_GLOBAL_DATA, key, data); }
    inline void csDisplayMsg(QString net, QString buf, Message msg)     { send(CS_DISPLAY_MSG, net, buf, QVariant::fromValue(msg)); }
    inline void csDisplayStatusMsg(QString net, QString msg)            { send(CS_DISPLAY_STATUS_MSG, net, msg); }
    inline void csModeSet(QString net, QString target, QString mode)    { send(CS_MODE_SET, net, target, mode); }
    inline void csTopicSet(QString net, QString buf, QString topic)     { send(CS_TOPIC_SET, net, buf, topic); }
    inline void csSetNicks(QString net, QString buf, QStringList nicks) { send(CS_SET_NICKS, net, buf, nicks); }
    inline void csNickAdded(QString net, QString nick, VarMap props)    { send(CS_NICK_ADDED, net, nick, props); }
    inline void csNickRemoved(QString net, QString nick)                { send(CS_NICK_REMOVED, net, nick); }
    inline void csNickRenamed(QString net, QString oldn, QString newn)  { send(CS_NICK_RENAMED, net, oldn, newn); }
    inline void csNickUpdated(QString net, QString nick, VarMap props)  { send(CS_NICK_UPDATED, net, nick, props); }
    inline void csOwnNickSet(QString net, QString nick)                 { send(CS_OWN_NICK_SET, net, nick); }

  signals:
    void gsPutGlobalData(QString, QVariant);
    void gsUserInput(QString, QString, QString);
    void gsRequestConnect(QStringList networks);

  private:
    void send(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());
    void recv(GUISignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());
    void sendToGUI(CoreSignal, QVariant arg1, QVariant arg2, QVariant arg3);

    void processClientInit(QTcpSocket *socket, const QVariant &v);
    void processClientUpdate(QTcpSocket *, QString key, QVariant data);

  private slots:
    void incomingConnection();
    void clientHasData();
    void clientDisconnected();
    void updateGlobalData(QString key);

  private:
    QTcpServer server;
    QList<QTcpSocket *> clients;
    QHash<QTcpSocket *, quint32> blockSizes;

  friend class GUIProxy;
};

extern CoreProxy *coreProxy;



#endif
