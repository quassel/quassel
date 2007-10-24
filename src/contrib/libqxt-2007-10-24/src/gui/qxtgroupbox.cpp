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
#include "qxtgroupbox.h"

#include <QChildEvent>

class QxtGroupBoxPrivate : public QxtPrivate<QxtGroupBox>
{
public:
    QXT_DECLARE_PUBLIC(QxtGroupBox);

    QxtGroupBoxPrivate();
    void init();
    bool collapsive;
};

QxtGroupBoxPrivate::QxtGroupBoxPrivate() : collapsive(true)
{}

void QxtGroupBoxPrivate::init()
{
    qxt_p().setCheckable(true);
    qxt_p().setChecked(true);
    QObject::connect(&qxt_p(), SIGNAL(toggled(bool)), &qxt_p(), SLOT(setExpanded(bool)));
}

/*!
    \class QxtGroupBox QxtGroupBox
    \ingroup QxtGui
    \brief A collapsive and checkable QGroupBox.

    QxtGroupBox is a checkable group box automatically expanding/collapsing
    its content according to the check state. QxtGroupBox shows its children
    when checked and hides its children when unchecked.

    \image html qxtgroupbox.png "Two QxtGroupBoxes - an expanded and a collapsed - on top of each other."
 */

/*!
    Constructs a new QxtGroupBox with \a parent.
 */
QxtGroupBox::QxtGroupBox(QWidget* parent)
        : QGroupBox(parent)
{
    QXT_INIT_PRIVATE(QxtGroupBox);
    qxt_d().init();
}

/*!
    Constructs a new QxtGroupBox with \a title and \a parent.
 */
QxtGroupBox::QxtGroupBox(const QString& title, QWidget* parent)
        : QGroupBox(title, parent)
{
    QXT_INIT_PRIVATE(QxtGroupBox);
    qxt_d().init();
}

/*!
    Destructs the group box.
 */
QxtGroupBox::~QxtGroupBox()
{}

/*!
    \property QxtGroupBox::collapsive
    \brief This property holds whether the group box is collapsive
 */
bool QxtGroupBox::isCollapsive() const
{
    return qxt_d().collapsive;
}

void QxtGroupBox::setCollapsive(bool enable)
{
    if (qxt_d().collapsive != enable)
    {
        qxt_d().collapsive = enable;
        if (!enable)
            setExpanded(true);
        else if (!isChecked())
            setExpanded(false);
    }
}

/*!
    Sets the group box \a collapsed.

    A collapsed group box hides its children.

    \sa setExpanded(), QGroupBox::toggled()
 */
void QxtGroupBox::setCollapsed(bool collapsed)
{
    setExpanded(!collapsed);
}

/*!
    Sets the group box \a expanded.

    An expanded group box shows its children.

    \sa setCollapsed(), QGroupBox::toggled()
 */
void QxtGroupBox::setExpanded(bool expanded)
{
    if (qxt_d().collapsive || expanded)
    {
        // show/hide direct children
        foreach (QObject* child, children())
        {
            if (child->isWidgetType())
                static_cast<QWidget*>(child)->setVisible(expanded);
        }
        setFlat(!expanded);
    }
}

void QxtGroupBox::childEvent(QChildEvent* event)
{
    QObject* child = event->child();
    if (event->added() && child->isWidgetType())
    {
        QWidget* widget = static_cast<QWidget*>(child);
        if (qxt_d().collapsive && !isChecked())
            widget->hide();
    }
}
