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
\class QxtSqlPackageModel QxtSqlPackageModel

\ingroup QxtSql

\brief  provides a read-only data model for QxtSqlPackage result..
*/



#ifndef QXTSQLTABLEMODEL_H
#define QXTSQLTABLEMODEL_H
#include <QAbstractTableModel>
#include <qxtsqlpackage.h>
#include <qxtglobal.h>
#include <QHash>


class QXT_SQL_EXPORT QxtSqlPackageModel : public  QAbstractTableModel
{
public:
/// \reimp
    QxtSqlPackageModel  (QObject * parent = 0 );



/// set the data for the model. do this before any access
    void setQuery(QxtSqlPackage a) ;


/// \reimp
    int rowCount ( const QModelIndex &  = QModelIndex()) const ;
/// \reimp
    int columnCount ( const QModelIndex  & = QModelIndex() ) const ;
/// \reimp
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
/// \reimp
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

private:

    QxtSqlPackage pack;
};


#endif



