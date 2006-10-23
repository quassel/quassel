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
    inline void csSendMessage(QString net, QString chan, QString msg)   { send(CS_SEND_MESSAGE, net, chan, msg); }
    inline void csSendStatusMsg(QString net, QString msg)               { send(CS_SEND_STATUS_MSG, net, msg); }

  signals:
    void gsPutGlobalData(QString, QVariant);
    void gsUserInput(QString);
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
