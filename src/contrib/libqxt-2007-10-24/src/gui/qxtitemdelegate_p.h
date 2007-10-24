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
#ifndef QXTITEMDELEGATE_P_H
#define QXTITEMDELEGATE_P_H

#include "qxtpimpl.h"
#include "qxtitemdelegate.h"
#include <QPersistentModelIndex>
#include <QPointer>

class QPainter;
class QTreeView;

class QxtItemDelegatePrivate : public QObject, public QxtPrivate<QxtItemDelegate>
{
    Q_OBJECT

public:
    QXT_DECLARE_PUBLIC(QxtItemDelegate);
    QxtItemDelegatePrivate();

    void paintButton(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QTreeView* view) const;
    void paintMenu(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QTreeView* view) const;
    void paintProgress(QPainter* painter, const QStyleOptionViewItem& option, int progress) const;
    void setCurrentEditor(QWidget* editor, const QModelIndex& index) const;

    bool textVisible;
    QString progressFormat;
    Qt::TextElideMode elide;
    Qxt::DecorationStyle style;
    mutable QPointer<QWidget> currentEditor;
    mutable QPersistentModelIndex currentEdited;

private slots:
    void closeEditor(QWidget* editor);
};

#endif // QXTITEMDELEGATE_P_H
