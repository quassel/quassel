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

#include <QString>
#include <QVariant>
#include <QTcpServer>
#include <QTcpSocket>

#include "global.h"

class CoreSession;
class Storage;

class Core : public QObject {
  Q_OBJECT

  public:
    static Core * instance();
    static void destroy();

    static CoreSession * session(UserId);
    static CoreSession * localSession();
    static CoreSession * createSession(UserId);

    static QVariant connectLocalClient(QString user, QString passwd);
    static void disconnectLocalClient();

  private slots:
    bool startListening(uint port = DEFAULT_PORT);
    void stopListening();
    void incomingConnection();
    void clientHasData();
    void clientDisconnected();

    bool initStorageSqlite(QVariantMap dbSettings, bool setup);

  private:
    Core();
    ~Core();
    void init();
    static Core *instanceptr;
    
    //! Initiate a session for the user with the given credentials if one does not already exist.
    /** This function is called during the init process for a new client. If there is no session for the
     *  given user, one is created.
     * \param userId The user
     * \return A QVariant containing the session data, e.g. global data and buffers
     */
    QVariant initSession(UserId userId);
    void processClientInit(QTcpSocket *socket, const QVariantMap &msg);
    void processCoreSetup(QTcpSocket *socket, QVariantMap &msg);
    
    QStringList availableStorageProviders();

    UserId guiUser;
    QHash<UserId, CoreSession *> sessions;
    Storage *storage;

    QTcpServer server; // TODO: implement SSL
    QHash<QTcpSocket *, quint32> blockSizes;
    
    bool configured;
};

#endif
