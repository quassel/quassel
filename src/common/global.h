/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

/** The protocol version we use fo the communication between core and GUI */
#define GUI_PROTOCOL 3

#define BACKLOG_FORMAT 2
#define BACKLOG_STRING "QuasselIRC Backlog File"

class Global;

#include <QHash>
#include <QMutex>
#include <QString>
#include <QVariant>

/* Some global stuff */
typedef QMap<QString, QVariant> VarMap;

typedef uint UserId;
typedef uint MsgId;

/**
 * This class is mostly a globally synchronized data store, meant for storing systemwide settings such
 * as identities or network lists. This class is a singleton, but not static as we'd like to use signals and
 * slots with it.
 * The global object is used in both Core and GUI clients. Storing and retrieving data is thread-safe.
 * \note While updated data is propagated to all the remote parts of Quassel quite quickly, the synchronization
 *       protocol is in no way designed to guarantee strict consistency at all times. In other words, it may
 *       well happen that different instances of global data differ from one another for a little while until
 *       all update messages have been processed. You should never rely on all global data stores being consistent.
*/
class Global : public QObject {
  Q_OBJECT

  public:
    //static Logger *getLogger();
    //static void setLogger(Logger *);

//    static QIcon *getIcon(QString symbol);

    static Global *instance();
    static void destroy();
    static void setGuiUser(UserId);

    static QVariant data(QString key, QVariant defaultValue = QVariant());
    static QVariant data(UserId, QString key, QVariant defaultValue = QVariant());
    static QStringList keys();
    static QStringList keys(UserId);

    static void putData(QString key, QVariant data);      ///< Store data changed locally, will be propagated to all other clients and the core
    static void putData(UserId, QString key, QVariant data);

    static void updateData(QString key, QVariant data);   ///< Update stored data if requested by the core or other clients
    static void updateData(UserId, QString key, QVariant data);

  signals:
    void dataPutLocally(UserId, QString key);
    void dataUpdatedRemotely(UserId, QString key);  // sent by remote update only!

  public:
    enum RunMode { Monolithic, ClientOnly, CoreOnly };
    static RunMode runMode;
    static QString quasselDir;

  private:
    Global();
    ~Global();
    static Global *instanceptr;

    static UserId guiUser;
    //static void initIconMap();

    //static Logger *logger;

//    static QString iconPath;
    //QHash<QString, QString> iconMap;
    static QMutex mutex;
    QHash<UserId, QHash<QString, QVariant> > datastore;
};

struct Exception {
    Exception(QString msg = "Unknown Exception") : _msg(msg) {};
    virtual ~Exception() {}; // make gcc happy
    virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;

};

class BufferId {
  public:
    BufferId() { id = gid = 0; } // FIXME
    BufferId(uint uid, QString net, QString buf, uint gid = 0);

    inline uint uid() const { return id; }
    inline uint groupId() const { return gid; }
    inline QString network() const { return net; }
    QString buffer() const; // nickfrommask?

    void setGroupId(uint _gid) { gid = _gid; }

    inline bool operator==(const BufferId &other) const { return id == other.id; }

  private:
    uint id;
    uint gid;
    QString net;
    QString buf;

    friend uint qHash(const BufferId &);
    friend QDataStream &operator<<(QDataStream &out, const BufferId &bufferId);
    friend QDataStream &operator>>(QDataStream &in, BufferId &bufferId);
};

QDataStream &operator<<(QDataStream &out, const BufferId &bufferId);
QDataStream &operator>>(QDataStream &in, BufferId &bufferId);

Q_DECLARE_METATYPE(BufferId);

uint qHash(const BufferId &);

#endif
