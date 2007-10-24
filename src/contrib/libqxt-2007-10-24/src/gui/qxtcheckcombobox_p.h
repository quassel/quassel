/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
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
#ifndef QXTCHECKCOMBOBOX_P_H
#define QXTCHECKCOMBOBOX_P_H

#include <QListView>
#include <QStandardItemModel>
#include "qxtcheckcombobox.h"
#include "qxtpimpl.h"

class QxtCheckComboView : public QListView
{
    Q_OBJECT

public:
    explicit QxtCheckComboView(QWidget* parent = 0);
    ~QxtCheckComboView();

    virtual bool eventFilter(QObject* object, QEvent* event);
    QxtCheckComboBox::CheckMode mode;
    bool handleIndicatorRelease(QMouseEvent* event, const QModelIndex& index);
    bool handleItemRelease(QMouseEvent* event, const QModelIndex& index);

signals:
    void hideRequested();
};

class QxtCheckComboModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit QxtCheckComboModel(QObject* parent = 0);
    ~QxtCheckComboModel();

    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

signals:
    void checkStateChanged();
};

class QxtCheckComboBoxPrivate : public QObject, public QxtPrivate<QxtCheckComboBox>
{
    Q_OBJECT

public:
    QXT_DECLARE_PUBLIC(QxtCheckComboBox);
    QxtCheckComboBoxPrivate();
    QString separator;
    QString defaultText;
    QStringList checkedItems;
    QxtCheckComboView* view;

public slots:
    void hidePopup();
    void updateCheckedItems();
};

#endif // QXTCHECKCOMBOBOX_P_H
