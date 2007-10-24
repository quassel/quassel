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
#ifndef QXTDOCKWIDGET_H
#define QXTDOCKWIDGET_H

#include <QDockWidget>
#include "qxtglobal.h"
#include "qxtpimpl.h"

class QxtDockWidgetPrivate;

class QXT_GUI_EXPORT QxtDockWidget : public QDockWidget
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtDockWidget);

public:
    explicit QxtDockWidget(const QString& title, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    explicit QxtDockWidget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~QxtDockWidget();

#ifndef QXT_DOXYGEN_RUN
    QSize sizeHint() const;
    void setVisible(bool visible);
#endif // QXT_DOXYGEN_RUN
};

#endif // QXTDOCKWIDGET_H
