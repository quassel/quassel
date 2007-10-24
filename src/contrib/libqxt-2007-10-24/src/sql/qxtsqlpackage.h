/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtSql module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/


/**
\class QxtSqlPackage QxtSqlPackage

\ingroup QxtSql

\brief full serialiseable QSqlQuery storage


Sometimes you want to set sql results over network or store them into files. QxtSqlPackage can provide you a storage that is still valid after the actual QSqlQuery has been destroyed
for confidence the interface is similiar to QSqlQuery.
*/

#ifndef QXTSQLPACKAGE_H
#define QXTSQLPACKAGE_H
#include <QObject>
#include <QHash>
#include <QList>
#include <QtSql>
#include <qxtglobal.h>

class QXT_SQL_EXPORT QxtSqlPackage : public  QObject
{
    Q_OBJECT

public:
    QxtSqlPackage(QObject *parent = 0);
    QxtSqlPackage(const QxtSqlPackage & other,QObject *parent = 0);

    ///determinates if the package curently points to a valid row
    bool isValid();

    ///curent pointer position
    int at();

    /** \brief point to next entry

    returns false if there is no next entry.\n
    provided for easy porting from QSqlQuery.

    \code	
    while (query.next())
    	{
    	}
    \endcode 
    */
    bool next();

    ///point to last entry in storage
    bool last();

    ///point to first entry in storage
    bool first();

    /** \brief return a cloumn in curent row

    in contrast to QSqlQuery you have to provide the name of the key.

    the entry is returned as QString becouse in most cases you need QString anyway, and converting to needed data type is easy.
    \code
    QString name = query.value("name");	
    \endcode 
    */
    QString value(const QString& key);

    /** \brief read from QSqlQuery

    read out a QSqlQuery and store the result. you may close the query after reading, the data will stay.

    \code
    QxSqlPackage::insert(QSqlQuery::exec("select name,foo,bar from table;"));
    \endcode 
    */
    void insert(QSqlQuery query);

    ///Returns the number of rows stored
    int count() const;

    ///serialise Data
    QByteArray data() const;

    ///deserialise data
    void setData(const QByteArray& data);

    ///return a specific row as Hash
    QHash<QString,QString> hash(int index);
    ///return the curent row as Hash
    QHash<QString,QString> hash();
    QxtSqlPackage& operator= (const QxtSqlPackage& other);

private:
    QList<QHash<QString,QString> > map;
    int record;
};



#endif
