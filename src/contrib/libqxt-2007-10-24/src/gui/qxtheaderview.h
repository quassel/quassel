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
#if 0
#ifndef QxtHeaderViewHHGuard
#define QxtHeaderViewHHGuard
#include <Qxt/qxtglobal.h>
#include <QHeaderView>


class QPainter;
class QAction;
class QxtHeaderViewPrivate;
class QXT_GUI_EXPORT QxtHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    QxtHeaderView (Qt::Orientation orientation ,QWidget * parent);
    void addAction(QAction * action);
protected:
    virtual void subPaint(QPainter * painter, const QRect & rect, int logicalIndex,QSize  icon_size, int spacing) const;
    virtual void subClick(QMouseEvent * m,QSize icon_size, int spacing ) ;
    virtual int  subWidth(QSize icon_size, int spacing) const;
signals:
    void checkBoxChanged(bool);

private:
    virtual	void paintSection ( QPainter * painter, const QRect & rect, int logicalIndex ) const;
    virtual void mousePressEvent ( QMouseEvent * m );
    virtual void mouseMoveEvent ( QMouseEvent * event );

    QxtHeaderViewPrivate * priv;///TODO NO. this is wrong!!!

};


#endif
#endif
