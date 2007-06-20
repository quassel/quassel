/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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

#ifndef _BACKLOG_H_
#define _BACKLOG_H_

#include <QtCore>
#include <QtSql>

#include "global.h"
#include "message.h"

// TODO: is this threadsafe? should it be?
class Backlog : public QObject {
  Q_OBJECT

  public:
    Backlog();
    ~Backlog();

    void init(QString user);
    void setDefaultSettings(int lines = -1, QDateTime time = QDateTime());
    void setSettings(QString network, QString buffer, int lines = -1, QDateTime time = QDateTime());

    BufferId getBufferId(QString network, QString buffer);
    uint logMessage(Message msg);

    QList<Message> requestMsgs(BufferId, int lastlines = -1, int offset = -1);
    QList<Message> requestMsgs(BufferId, QDateTime since, int offset = -1);
    //QList<Message> requestMsgRange(BufferId, int first, int last);

    QList<BufferId> requestBuffers(QDateTime since = QDateTime());

  public slots:
    void importOldBacklog();

  signals:
    void bufferIdUpdated(BufferId); // sent also if a new bufferid is created

  private:
    QString user;
    bool backlogEnabled;
    QSqlDatabase logDb;

    uint nextMsgId, nextBufferId, nextSenderId;

    QTimer cleanupTimer;

    void cleanup();

    // Old stuff, just for importing old file-based data
    void initBackLogOld();
    void logMessageOld(QString net, Message);

    bool backLogEnabledOld;
    QDir backLogDir;
    QHash<QString, QList<Message> > backLog;
    //QHash<QString, int> netIdx;
    QHash<QString, QFile *> logFiles;
    QHash<QString, QDataStream *> logStreams;
    QHash<QString, QDate> logFileDates;
    QHash<QString, QDir> logFileDirs;

};



#endif
