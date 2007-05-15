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
#include "global.h"

class Core : public QObject {
  Q_OBJECT

  public:

    Core();
    ~Core();
    QHash<QString, QList<Message> > getBackLog() { return QHash<QString, QList<Message> >()/*backLog*/; }; // FIXME
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
    //void serverStatesRequested();
    void globalDataUpdated(QString);
    void recvStatusMsgFromServer(QString msg);
    //void recvMessageFromServer(Message msg);
    void recvMessageFromServer(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
    void serverDisconnected(QString net);

  private:
    Backlog backlog;
    QDir backLogDir;
    bool backLogEnabled;
    QHash<QString, Server *> servers;
    //QHash<QString, QList<Message> > backLog;
    //QHash<QString, int> netIdx;
    //QHash<QString, QFile *> logFiles;
    //QHash<QString, QDataStream *> logStreams;
    //QHash<QString, QDate> logFileDates;
    //QHash<QString, QDir> logFileDirs;

    //QSqlDatabase logDb;

    //uint getNetIdx(QString net);
    //void initBackLog();
    //void initBackLogOld();
    //void logMessage(QString, Message);
    //void logMessageOld(QString, Message);

};

extern Core *core;

#endif
