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
#ifndef QXTCONFIGDIALOG_P_H
#define QXTCONFIGDIALOG_P_H

#include "qxtpimpl.h"
#include "qxtconfigdialog.h"
#include <QItemDelegate>
#include <QListWidget>

class QGridLayout;
class QStackedWidget;
class QDialogButtonBox;

class QxtConfigListWidget : public QListWidget
{
public:
    QxtConfigListWidget(QWidget* parent = 0);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void invalidate();

    bool hasHoverEffect() const;
    void setHoverEffect(bool enabled);

protected:
    void scrollContentsBy(int dx, int dy);

private:
    mutable QSize hint;
};

class QxtConfigDelegate : public QItemDelegate
{
public:
    QxtConfigDelegate(QObject* parent = 0);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    bool hover;
};

class QxtConfigDialogPrivate : public QObject, public QxtPrivate<QxtConfigDialog>
{
    Q_OBJECT

public:
    QXT_DECLARE_PUBLIC(QxtConfigDialog);

    void init(QxtConfigDialog::IconPosition position = QxtConfigDialog::West);
    void initList();
    void relayout();

    QGridLayout* grid;
    QStackedWidget* stack;
    QxtConfigListWidget* list;
#if QT_VERSION >= 0x040200
    QDialogButtonBox* buttons;
#else // QT_VERSION >= 0x040200
    QWidget* buttons;
#endif // QT_VERSION
    QxtConfigDialog::IconPosition pos;
};

#endif // QXTCONFIGDIALOG_P_H
