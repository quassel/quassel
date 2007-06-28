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

#ifndef _CLIENTPROXY_H_
#define _CLIENTPROXY_H_

#include <QStringList>

#include "proxy_common.h"
#include "message.h"
#include "global.h"

class ClientProxy : public QObject {
  Q_OBJECT

  public:
    static ClientProxy *instance();
    static void destroy();

  public slots:
    inline void gsUserInput(BufferId id, QString msg)                 { send(GS_USER_INPUT, QVariant::fromValue(id), msg); }
    inline void gsRequestConnect(QStringList networks)                { send(GS_REQUEST_CONNECT, networks); }
    inline void gsImportBacklog()                                     { send(GS_IMPORT_BACKLOG); }
    inline void gsRequestBacklog(BufferId id, QVariant v1, QVariant v2) { send(GS_REQUEST_BACKLOG, QVariant::fromValue(id), v1, v2); }
    inline void gsRequestNetworkStates()                              { send(GS_REQUEST_NETWORK_STATES); }

    inline void gsGeneric(ClientSignal sig, QVariant v1 = QVariant(), QVariant v2 = QVariant(), QVariant v3 = QVariant()) { send(sig, v1, v2, v3); }

  signals:
    void csCoreState(QVariant);
    void csServerState(QString, QVariant);
    void csServerConnected(QString);
    void csServerDisconnected(QString);
    void csDisplayMsg(Message);
    void csDisplayStatusMsg(QString, QString);
    void csUpdateGlobalData(QString key, QVariant data);
    void csGlobalDataChanged(QString key);
    void csModeSet(QString, QString, QString);
    void csTopicSet(QString, QString, QString);
    void csNickAdded(QString, QString, VarMap);
    void csNickRemoved(QString, QString);
    void csNickRenamed(QString, QString, QString);
    void csNickUpdated(QString, QString, VarMap);
    void csOwnNickSet(QString, QString);
    void csQueryRequested(QString, QString);
    void csBacklogData(BufferId, QList<QVariant>, bool);
    void csUpdateBufferId(BufferId);

    void csGeneric(CoreSignal, QVariant, QVariant, QVariant);

    void send(ClientSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  public slots:
    void recv(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  private:
    ClientProxy();
    static ClientProxy *instanceptr;
};

#endif
