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
#ifndef QXTCSVMODEL_H
#define QXTCSVMODEL_H

#include <QAbstractTableModel>
#include <QVariant>
#include <QIODevice>
#include <QChar>
#include <QString>
#include <QStringList>
#include <QModelIndex>
#include <qxtpimpl.h>
#include <qxtglobal.h>

class QxtCsvModelPrivate;
class QXT_CORE_EXPORT QxtCsvModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    QxtCsvModel(QObject *parent = 0);
    QxtCsvModel(QIODevice *file, QObject *parent = 0, bool withHeader = false, QChar separator = ',');
    QxtCsvModel(const QString filename, QObject *parent = 0, bool withHeader = false, QChar separator = ',');
    ~QxtCsvModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    void setHeaderData(const QStringList data);

    bool insertRow(int row, const QModelIndex& parent = QModelIndex());
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
    bool insertColumn(int col, const QModelIndex& parent = QModelIndex());
    bool insertColumns(int col, int count, const QModelIndex& parent = QModelIndex());
    bool removeColumn(int col, const QModelIndex& parent = QModelIndex());
    bool removeColumns(int col, int count, const QModelIndex& parent = QModelIndex());

    void setSource(QIODevice *file, bool withHeader = false, QChar separator = ',');
    void setSource(const QString filename, bool withHeader = false, QChar separator = ',');

    void toCSV(QIODevice *file, bool withHeader = false, QChar separator = ',');
    void toCSV(const QString filename, bool withHeader = false, QChar separator = ',');

    Qt::ItemFlags flags(const QModelIndex& index) const;

private:
    QXT_DECLARE_PRIVATE(QxtCsvModel);
};

#endif
