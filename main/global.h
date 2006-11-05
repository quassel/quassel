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
#define GUI_PROTOCOL 1

#define BACKLOG_FORMAT 2
#define BACKLOG_STRING "QuasselIRC Backlog File"

class Global;

#include <QtCore>
//#include <QMutex>

/* Some global stuff */
typedef QMap<QString, QVariant> VarMap;
extern Global *global;

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
    Global();
    //static Logger *getLogger();
    //static void setLogger(Logger *);

//    static QIcon *getIcon(QString symbol);

    QVariant getData(QString key, QVariant defaultValue = QVariant());
    QStringList getKeys();

  public slots:
    void putData(QString key, QVariant data);      ///< Store data changed locally, will be propagated to all other clients and the core
    void updateData(QString key, QVariant data);   ///< Update stored data if requested by the core or other clients

  signals:
    void dataPutLocally(QString key);
    void dataUpdatedRemotely(QString key);  // sent by remote update only!

  public:
    enum RunMode { Monolithic, GUIOnly, CoreOnly };
    static RunMode runMode;
    static QString quasselDir;

  private:
    static void initIconMap();

    //static Logger *logger;

//    static QString iconPath;
    QHash<QString, QString> iconMap;
    QMutex mutex;
    QHash<QString, QVariant> data;
};

class Exception {
  public:
    Exception(QString msg = "Unknown Exception") : _msg(msg) {};
    virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;

};

#endif
