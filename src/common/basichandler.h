/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef BASICHANDLER_H
#define BASICHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QGenericArgument>

class BasicHandler : public QObject
{
    Q_OBJECT

public:
    BasicHandler(QObject *parent = 0);
    BasicHandler(const QString &methodPrefix, QObject *parent = 0);

    QStringList providesHandlers();

protected:
    virtual void handle(const QString &member, QGenericArgument val0 = QGenericArgument(0),
        QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(),
        QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
        QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
        QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument());

private:
    const QHash<QString, int> &handlerHash();
    QHash<QString, int> _handlerHash;
    int _defaultHandler;
    bool _initDone;
    QString _methodPrefix;
};


#endif
