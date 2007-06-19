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

#ifndef _GUIPROXY_H_
#define _GUIPROXY_H_

#include "proxy_common.h"
#include "message.h"
#include "global.h"

#include <QObject>
#include <QVariant>
#include <QTcpSocket>
#include <QStringList>

/** This class is the GUI side of the proxy. The GUI connects its signals and slots to it,
 *  and the calls are marshalled and sent to (or received and unmarshalled from) the CoreProxy.
 *  The connection function is defined in main/main_gui.cpp or main/main_mono.cpp.
 */
class GUIProxy : public QObject {
  Q_OBJECT

  public:
    GUIProxy();

  public slots:
    inline void gsUserInput(BufferId id, QString msg)                 { send(GS_USER_INPUT, QVariant::fromValue(id), msg); }
    inline void gsRequestConnect(QStringList networks)                { send(GS_REQUEST_CONNECT, networks); }
    inline void gsImportBacklog()                                     { send(GS_IMPORT_BACKLOG); }
    inline void gsRequestBacklog(BufferId id, QVariant v1, QVariant v2) { send(GS_REQUEST_BACKLOG, QVariant::fromValue(id), v1, v2); }

    inline void gsGeneric(GUISignal sig, QVariant v1 = QVariant(), QVariant v2 = QVariant(), QVariant v3 = QVariant()) { send(sig, v1, v2, v3); }

    void connectToCore(QString host, quint16 port);
    void disconnectFromCore();

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

    void coreConnected();
    void coreDisconnected();
    void coreConnectionError(QString errorMsg);

    void recvPartialItem(quint32 avail, quint32 size);

    void send(GUISignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  public slots:
    void recv(CoreSignal, QVariant arg1 = QVariant(), QVariant arg2 = QVariant(), QVariant arg3 = QVariant());

  private slots:
    void updateCoreData(QString);

    void serverError(QAbstractSocket::SocketError);
    void serverHasData();

  private:
    QTcpSocket socket;
    quint32 blockSize;

  friend class CoreProxy;

};

extern GUIProxy *guiProxy;



#endif
