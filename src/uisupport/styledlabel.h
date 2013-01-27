/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef STYLEDLABEL_H
#define STYLEDLABEL_H

#include <QFrame>

#include "clickable.h"
#include "uistyle.h"

class StyledLabel : public QFrame
{
    Q_OBJECT

public:
    enum ResizeMode {
        NoResize,
        DynamicResize,
        ResizeOnHover
    };

    StyledLabel(QWidget *parent = 0);

    void setText(const QString &text);
    void setCustomFont(const QFont &font);

    virtual QSize sizeHint() const;
    //virtual QSize minimumSizeHint() const;

    inline QTextOption::WrapMode wrapMode() const { return _wrapMode; }
    void setWrapMode(QTextOption::WrapMode mode);

    inline Qt::Alignment alignment() const { return _alignment; }
    void setAlignment(Qt::Alignment alignment);

    inline bool toolTipEnabled() const { return _toolTipEnabled; }
    void setToolTipEnabled(bool);

    inline ResizeMode resizeMode() const { return _resizeMode; }
    void setResizeMode(ResizeMode);

signals:
    void clickableActivated(const Clickable &click);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);

    int posToCursor(const QPointF &pos);

private:
    QSize _sizeHint;
    QTextOption::WrapMode _wrapMode;
    Qt::Alignment _alignment;
    QTextLayout _layout;
    ClickableList _clickables;
    bool _toolTipEnabled;
    ResizeMode _resizeMode;

    QList<QTextLayout::FormatRange> _layoutList;
    QVector<QTextLayout::FormatRange> _extraLayoutList;

    void layout();
    void updateSizeHint();
    void updateToolTip();

    void setHoverMode(int start, int length);
    void endHoverMode();
};


#endif
