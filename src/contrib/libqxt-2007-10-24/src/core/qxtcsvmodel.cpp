/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
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

/*!
\class QxtCsvModel QxtCsvModel
\ingroup QxtCore
\brief Provides a QAbstractTableModel for CSV Files
 */

/*!
\fn QxtCsvModel::QxtCsvModel(QObject *parent = 0);
default Constructor
*/

/*!
\fn QxtCsvModel::QxtCsvModel(QIODevice *file, QObject *parent=0, bool withHeader = false, QChar separator= ',')
constructs a QxtCsvModel from a QIODevice as source \n
\p withHeader specifies if the data on the device contains a header or not.
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/

/*!
\fn QxtCsvModel::QxtCsvModel(const QString filename, QObject *parent = 0, bool withHeader = false, QChar separator = ',')
constructs a QxtCsvModel from a filename as source \n
\p withHeader specifies if the data in the file contains a header or not.
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/

/*!
\fn  void QxtCsvModel::setSource(QIODevice *file, bool withHeader = false, QChar separator = ',');
reads the cvs data from \p file
\p withHeader specifies if the data on the device contains a header or not.
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/


/*!
\fn    void QxtCsvModel::setSource(const QString filename, bool withHeader = false, QChar separator = ',');
reads the cvs data from \p filename
\p withHeader specifies if the data in the file contains a header or not.
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/



/*!
\fn   void QxtCsvModel::toCSV(QIODevice *file, bool withHeader = false, QChar separator = ',');
writes the cvs data to \p file
\p withHeader specifies if to write the header or not
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/


/*!
\fn    void QxtCsvModel::toCSV(const QString filename, bool withHeader = false, QChar separator = ',');
writes the cvs data to \p filename
\p withHeader specifies if to write the header or not
\p separator is the seperator to use for the columns. most widely used seperators are ','  '\\t' ';'
*/



#include "qxtcsvmodel.h"
#include <QFile>
#include <QDebug>

class QxtCsvModelPrivate : public QxtPrivate<QxtCsvModel>
{
public:
    QxtCsvModelPrivate() : csvData(), header(), maxColumn(0)
    {}
    QXT_DECLARE_PUBLIC(QxtCsvModel);

    QStringList csvData;
    QStringList header;
    int maxColumn;
};

QxtCsvModel::QxtCsvModel(QObject *parent) : QAbstractTableModel(parent)
{
    QXT_INIT_PRIVATE(QxtCsvModel);
}

QxtCsvModel::QxtCsvModel(QIODevice *file, QObject *parent, bool withHeader, QChar separator) : QAbstractTableModel(parent)
{
    QXT_INIT_PRIVATE(QxtCsvModel);
    setSource(file, withHeader, separator);
}

QxtCsvModel::QxtCsvModel(const QString filename, QObject *parent, bool withHeader, QChar separator) : QAbstractTableModel(parent)
{
    QXT_INIT_PRIVATE(QxtCsvModel);
    QFile src(filename);
    setSource(&src, withHeader, separator);
}

QxtCsvModel::~QxtCsvModel()
{}

int QxtCsvModel::rowCount(const QModelIndex& parent) const
{
    if (parent.row()!=-1 && parent.column()!=-1) return 0;
    return qxt_d().csvData.count();
}

int QxtCsvModel::columnCount(const QModelIndex& parent) const
{
    if (parent.row()!=-1 && parent.column()!=-1) return 0;
    return qxt_d().maxColumn;
}

QVariant QxtCsvModel::data(const QModelIndex& index, int role) const
{
    if (index.parent()!=QModelIndex()) return QVariant();
    if (role==Qt::DisplayRole || role==Qt::EditRole || role==Qt::UserRole)
        return qxt_d().csvData[index.row()].section(QChar(1),index.column(),index.column());
    else
    {
        //QVariant v;
        return QVariant();
    }
}

QVariant QxtCsvModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < qxt_d().header.count() && orientation==Qt::Horizontal && (role==Qt::DisplayRole || role==Qt::EditRole || role==Qt::UserRole))
    {
        return qxt_d().header[section];
    }
    else
        return QAbstractTableModel::headerData(section, orientation, role);
}

void QxtCsvModel::setSource(const QString filename, bool withHeader, QChar separator)
{
    QFile src(filename);
    setSource(&src, withHeader, separator);
}

void QxtCsvModel::setSource(QIODevice *file, bool withHeader, QChar separator)
{
    QxtCsvModelPrivate* d_ptr = &qxt_d();
    QString l;
    int size;
    bool isQuoted, headerSet = false;
    if (!file->isOpen()) file->open(QIODevice::ReadOnly);
    if (withHeader)
        d_ptr->maxColumn = 0;
    else
        d_ptr->maxColumn = d_ptr->header.size();
    d_ptr->csvData.clear();
    while (!file->atEnd())
    {
        l = file->readLine();
        l.replace(QChar('\n'),"");
        l.replace(QChar('\r'),"");
        size = l.length();
        isQuoted = false;
        for (int i=0;i<size;i++)
        {
            if (i>0)
            {
                if (l[i]=='"' && l[i-1]!='\\') isQuoted=!isQuoted;
                else if (!isQuoted && l[i]==separator) l[i]=QChar(1);
            }
            else
            {
                if (l[i]=='"') isQuoted=!isQuoted;
                else if (!isQuoted && l[i]==separator) l[i]=QChar(1);
            }
        }
        if (l.count(QChar(1))+1 > d_ptr->maxColumn) d_ptr->maxColumn = l.count(QChar(1))+1;
        if (withHeader && !headerSet)
        {
            d_ptr->header = l.split(QChar(1));
            headerSet=true;
        }
        else
            d_ptr->csvData.append(l);
    }
    file->close();
}

void QxtCsvModel::setHeaderData(const QStringList data)
{
    qxt_d().header = data;
    emit headerDataChanged(Qt::Horizontal, 0, data.count());
}

bool QxtCsvModel::setData(const QModelIndex& index, const QVariant& data, int role)
{
    if (index.parent()!=QModelIndex()) return false;

    QString before, after;
    if (role==Qt::DisplayRole || role==Qt::EditRole || role==Qt::UserRole)
    {
        if (index.row()>=rowCount() || index.column()>=columnCount() || index.row()<0 || index.column()<0) return false;
        if (index.column()!=0)
            before = qxt_d().csvData[index.row()].section(QChar(1),0,index.column()-1) + QChar(1);
        else
            before = "";
        after = qxt_d().csvData[index.row()].section(QChar(1),index.column()+1);
        qxt_d().csvData[index.row()] = before + data.toString() + QChar(1) + after;
        emit dataChanged(index, index);
        return true;
    }
    else
    {
        return false;
    }
}

bool QxtCsvModel::insertRow(int row, const QModelIndex& parent)
{
    return insertRows(row, 1, parent);
}

bool QxtCsvModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent!=QModelIndex() || row<0) return false;
    emit beginInsertRows(parent, row, row+count);
    QxtCsvModelPrivate& d_ptr = qxt_d();
    if (row>=rowCount())
    {
        for (int i=0;i<count;i++) d_ptr.csvData << "";
    }
    else
    {
        for (int i=0;i<count;i++) d_ptr.csvData.insert(row, "");
    }
    emit endInsertRows();
    return true;
}

bool QxtCsvModel::removeRow(int row, const QModelIndex& parent)
{
    return removeRows(row, 1, parent);
}

bool QxtCsvModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent!=QModelIndex() || row<0) return false;
    if (row>=rowCount()) return false;
    if (row+count>=rowCount()) count = rowCount()-row;
    emit beginRemoveRows(parent, row, row+count);
    QxtCsvModelPrivate& d_ptr = qxt_d();
    for (int i=0;i<count;i++)
        d_ptr.csvData.removeAt(row);
    emit endRemoveRows();
    return true;
}

bool QxtCsvModel::insertColumn(int col, const QModelIndex& parent)
{
    return insertColumns(col, 1, parent);
}

bool QxtCsvModel::insertColumns(int col, int count, const QModelIndex& parent)
{
    if (parent!=QModelIndex() || col<0) return false;
    emit beginInsertColumns(parent, col, col+count);
    QxtCsvModelPrivate& d_ptr = qxt_d();
    if (col<columnCount())
    {
        QString before, after;
        for (int i=0;i<rowCount();i++)
        {
            if (col>0)
                before = d_ptr.csvData[i].section(QChar(1),0,col-1)+QChar(1);
            else
                before = "";
            after = d_ptr.csvData[i].section(QChar(1),col);
            d_ptr.csvData[i] = before + QString(count, QChar(1)) + after;
        }
    }
    for (int i=0;i<count;i++)
        d_ptr.header.insert(col,"");
    d_ptr.maxColumn+=count;
    emit endInsertColumns();
    return true;
}

bool QxtCsvModel::removeColumn(int col, const QModelIndex& parent)
{
    return removeColumns(col, 1, parent);
}

bool QxtCsvModel::removeColumns(int col, int count, const QModelIndex& parent)
{
    if (parent!=QModelIndex() || col<0) return false;
    if (col>=columnCount()) return false;
    if (col+count>=columnCount()) count = columnCount()-col;
    emit beginRemoveColumns(parent, col, col+count);
    QxtCsvModelPrivate& d_ptr = qxt_d();
    QString before, after;
    for (int i=0;i<rowCount();i++)
    {
        if (col>0)
            before = d_ptr.csvData[i].section(QChar(1),0,col-1)+QChar(1);
        else
            before = "";
        after = d_ptr.csvData[i].section(QChar(1),col+count);
        d_ptr.csvData[i] = before + after;
    }
    for (int i=0;i<count;i++)
        d_ptr.header.removeAt(col);
    emit endRemoveColumns();
    return true;
}


void QxtCsvModel::toCSV(QIODevice* dest, bool withHeader, QChar separator)
{
    QxtCsvModelPrivate& d_ptr = qxt_d();
    int row, col, rows, cols;
    rows = rowCount();
    cols = columnCount();
    QString data;
    if (!dest->isOpen()) dest->open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (withHeader)
    {
        data = "";
        for (col = 0; col < cols; col++)
        {
            data += '"' + d_ptr.header.at(col) + '"';
            if (col<cols-1) data += separator;
        }
        data += QChar(10);
        dest->write(data.toLatin1());
    }
    for (row = 0; row < rows; row++)
    {
        data = "";
        for (col = 0; col < cols; col++)
        {
            data += '"' + d_ptr.csvData[row].section(QChar(1),col,col) + '"';
            if (col<cols-1) data += separator;
        }
        data += QChar(10);
        dest->write(data.toLatin1());
    }
    dest->close();
}

void QxtCsvModel::toCSV(const QString filename, bool withHeader, QChar separator)
{
    QFile dest(filename);
    toCSV(&dest, withHeader, separator);
}

Qt::ItemFlags QxtCsvModel::flags(const QModelIndex& index) const
{
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}
