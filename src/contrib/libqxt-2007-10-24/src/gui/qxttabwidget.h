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
#ifndef QXTTABWIDGET_H
#define QXTTABWIDGET_H

#include <QTabWidget>
#include "qxtnamespace.h"
#include "qxtglobal.h"
#include "qxtpimpl.h"

class QxtTabWidgetPrivate;

class QXT_GUI_EXPORT QxtTabWidget : public QTabWidget
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtTabWidget);
    Q_PROPERTY(Qt::ContextMenuPolicy tabContextMenuPolicy READ tabContextMenuPolicy WRITE setTabContextMenuPolicy)

public:
    explicit QxtTabWidget(QWidget* parent = 0);
    virtual ~QxtTabWidget();

    Qt::ContextMenuPolicy tabContextMenuPolicy() const;
    void setTabContextMenuPolicy(Qt::ContextMenuPolicy policy);

    void addTabAction(int index, QAction* action);
    QAction* addTabAction(int index, const QString& text);
    QAction* addTabAction(int index, const QIcon& icon, const QString& text);
    QAction* addTabAction(int index, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut = 0);
    QAction* addTabAction(int index, const QIcon& icon, const QString& text, const QObject* receiver, const char* member, const QKeySequence& shortcut = 0);
    void addTabActions(int index, QList<QAction*> actions);
    void clearTabActions(int index);
    void insertTabAction(int index, QAction* before, QAction* action);
    void insertTabActions(int index, QAction* before, QList<QAction*> actions);
    void removeTabAction(int index, QAction* action);
    QList<QAction*> tabActions(int index) const;

signals:
    void tabContextMenuRequested(int index, const QPoint& globalPos);

protected:
#ifndef QXT_DOXYGEN_RUN
    virtual void tabInserted(int index);
    virtual void tabRemoved(int index);

    virtual void contextMenuEvent(QContextMenuEvent* event);
#endif // QXT_DOXYGEN_RUN
    virtual void tabContextMenuEvent(int index, QContextMenuEvent* event);
};

#endif // QXTQXTTABWIDGET_H
