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

#ifndef _CORE_H_
#define _CORE_H_

#include <QMap>
#include <QString>
#include <QVariant>
#include <QSqlDatabase>

#include "server.h"
#include "backlog.h"
#include "storage.h"
#include "global.h"

class Core : public QObject {
  Q_OBJECT

  public:

    Core();
    ~Core();
    QList<BufferId> getBuffers();

  public slots:
    void connectToIrc(QStringList);
    void sendBacklog(BufferId, QVariant, QVariant);
    void msgFromGUI(BufferId, QString message);

  signals:
    void msgFromGUI(QString net, QString buf, QString message);
    void displayMsg(Message message);
    void displayStatusMsg(QString, QString);

    void connectToIrc(QString net);
    void disconnectFromIrc(QString net);
    void serverStateRequested();

    void backlogData(BufferId, QList<QVariant>, bool done);

    void bufferIdUpdated(BufferId);

  private slots:
    void globalDataUpdated(QString);
    void recvStatusMsgFromServer(QString msg);
    void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
    void serverDisconnected(QString net);

  private:
    Storage *storage;
    QHash<QString, Server *> servers;
    UserId user;

};

extern Core *core;

#endif
