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

#ifndef _QUASSEL_H_
#define _QUASSEL_H_

class Quassel;

#include <QtCore>
//#include <QMutex>

/* Some global stuff */
typedef QMap<QString, QVariant> VarMap;
extern Quassel *quassel;

/**
 * A static class containing global data.
 * This is used in both core and GUI modules. Where appropriate, accessors are thread-safe
 * to account for that fact.
 */
class Quassel : public QObject {
  Q_OBJECT

  public:
    static Quassel * init();
    //static Logger *getLogger();
    //static void setLogger(Logger *);

//    static QIcon *getIcon(QString symbol);

    QVariant getData(QString key);

  public slots:
    void putData(QString key, const QVariant &data);

  signals:
    void dataChanged(QString key, const QVariant &data);

  public:
    enum RunMode { Monolithic, GUIOnly, CoreOnly };
    static RunMode runMode;

  private:
    static void initIconMap();
    
    //static Logger *logger;

//    static QString iconPath;
    static QHash<QString, QString> iconMap;
    static QMutex mutex;
    static QHash<QString, QVariant> data;
};

class Exception {
  public:
    Exception(QString msg = "Unknown Exception") : _msg(msg) {};
    virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;

};

#endif
