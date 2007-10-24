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
#ifndef QXTGROUPBOX_H
#define QXTGROUPBOX_H

#include <QGroupBox>
#include "qxtglobal.h"
#include "qxtpimpl.h"

class QxtGroupBoxPrivate;

class QXT_GUI_EXPORT QxtGroupBox : public QGroupBox
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtGroupBox);
    Q_PROPERTY(bool collapsive READ isCollapsive WRITE setCollapsive)

public:
    explicit QxtGroupBox(QWidget* parent = 0);
    explicit QxtGroupBox(const QString& title, QWidget* parent = 0);
    virtual ~QxtGroupBox();

    bool isCollapsive() const;
    void setCollapsive(bool enabled);

public slots:
    void setCollapsed(bool collapsed = true);
    void setExpanded(bool expanded = true);

#ifndef QXT_DOXYGEN_RUN
protected:
    virtual void childEvent(QChildEvent* event);
#endif // QXT_DOXYGEN_RUN
};

#endif // QXTGROUPBOX_H
