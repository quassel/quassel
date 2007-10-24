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
#ifndef QXTPROGRESSLABEL_H
#define QXTPROGRESSLABEL_H

#include <QLabel>
#include "qxtglobal.h"
#include "qxtpimpl.h"

class QxtProgressLabelPrivate;

class QXT_GUI_EXPORT QxtProgressLabel : public QLabel
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtProgressLabel);
    Q_PROPERTY(QString contentFormat READ contentFormat WRITE setContentFormat)
    Q_PROPERTY(QString timeFormat READ timeFormat WRITE setTimeFormat)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval)

public:
    explicit QxtProgressLabel(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    explicit QxtProgressLabel(const QString& text, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~QxtProgressLabel();

    QString contentFormat() const;
    void setContentFormat(const QString& format);

    QString timeFormat() const;
    void setTimeFormat(const QString& format);

    int updateInterval() const;
    void setUpdateInterval(int msecs);

public slots:
    void setValue(int value);
    void refresh();
    void restart();

#ifndef QXT_DOXYGEN_RUN
    virtual void timerEvent(QTimerEvent* event);
#endif // QXT_DOXYGEN_RUN
};

#endif // QXTPROGRESSLABEL_H
