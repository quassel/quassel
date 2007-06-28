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

#ifndef _SQLITESTORAGE_H_
#define _SQLITESTORAGE_H_

#include <QCryptographicHash>

#include "global.h"
#include "storage.h"

class QSqlQuery;

class SqliteStorage : public Storage {
  Q_OBJECT

  public:
    SqliteStorage();
    virtual ~SqliteStorage();

    static void init();

    /* General */

    static bool isAvailable();
    static QString displayName();

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    virtual UserId addUser(QString user, QString password);
    virtual void updateUser(UserId user, QString password);
    virtual UserId validateUser(QString user, QString password);
    virtual void delUser(UserId user);

    /* Buffer handling */

    virtual BufferId getBufferId(UserId user, QString network, QString buffer = "");
    virtual QList<BufferId> requestBuffers(UserId user, QDateTime since = QDateTime());

    /* Message handling */

    virtual MsgId logMessage(Message msg);
    virtual QList<Message> requestMsgs(BufferId buffer, int lastmsgs = -1, int offset = -1);
    virtual QList<Message> requestMsgs(BufferId buffer, QDateTime since, int offset = -1);
    virtual QList<Message> requestMsgRange(BufferId buffer, int first, int last);

  public slots:
    //! This is just for importing the old file-based backlog */
    /** This slot needs to be implemented in the storage backends.
     *  It should first prepare (delete?) the database, then call initBackLogOld(UserId id).
     *  If the importing was successful, backLogEnabledOld will be true afterwards.
     */
    void importOldBacklog();

  signals:
    void bufferIdUpdated(BufferId);

  protected:

  private:
    void initDb();
    void createBuffer(UserId user, QString network, QString buffer);
    QSqlQuery *logMessageQuery;
    QSqlQuery *addSenderQuery;
    QSqlQuery *getLastMessageIdQuery;
    QSqlQuery *requestMsgsQuery;
    QSqlQuery *requestMsgsOffsetQuery;
    QSqlQuery *requestMsgsSinceQuery;
    QSqlQuery *requestMsgsSinceOffsetQuery;
    QSqlQuery *requestMsgRangeQuery;
    QSqlQuery *createNetworkQuery;
    QSqlQuery *createBufferQuery;
    QSqlQuery *getBufferIdQuery;
};


#endif
