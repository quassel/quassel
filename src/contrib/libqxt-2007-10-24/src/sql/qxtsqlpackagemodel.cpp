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

#include "qxtsqlpackagemodel.h"



QxtSqlPackageModel::QxtSqlPackageModel  (QObject * parent ) : QAbstractTableModel(parent)
{}


void QxtSqlPackageModel::setQuery(QxtSqlPackage a)
{
    pack =a;

}


int QxtSqlPackageModel::rowCount ( const QModelIndex &  ) const
{
    return pack.count();
}


int QxtSqlPackageModel::columnCount ( const QModelIndex & ) const
{
    QxtSqlPackage p =pack;
    return p.hash(0).count();
}


QVariant QxtSqlPackageModel::data ( const QModelIndex  & index, int role  ) const
{
    if  (role != Qt::DisplayRole )
        return QVariant();



    if ((index.row()<0)  || (index.column()<0)  ) return QVariant();
    QxtSqlPackage p =pack;

    return  p.hash(index.row()).values ().at( index.column());


}


QVariant QxtSqlPackageModel::headerData ( int section, Qt::Orientation orientation, int role  ) const
{

    if (orientation == Qt::Vertical && role == Qt::DisplayRole)
        return section;


    if (orientation==Qt::Horizontal && role == Qt::DisplayRole)
    {
        QxtSqlPackage p =pack;
        return p.hash(0).keys ().at( section )    ;
    }

    return QAbstractItemModel::headerData(section, orientation, role);

}

