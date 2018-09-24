/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#pragma once

#include "uisupport-export.h"

#include <QFrame>

#include "clickable.h"
#include "uistyle.h"

class UISUPPORT_EXPORT StyledLabel : public QFrame
{
    Q_OBJECT

public:
    enum ResizeMode
    {
        NoResize,
        DynamicResize,
        ResizeOnHover
    };

    StyledLabel(QWidget* parent = nullptr);

    void setText(const QString& text);
    void setCustomFont(const QFont& font);

    QSize sizeHint() const override;
    // virtual QSize minimumSizeHint() const;

    inline QTextOption::WrapMode wrapMode() const { return _wrapMode; }
    void setWrapMode(QTextOption::WrapMode mode);

    inline Qt::Alignment alignment() const { return _alignment; }
    void setAlignment(Qt::Alignment alignment);

    inline bool toolTipEnabled() const { return _toolTipEnabled; }
    void setToolTipEnabled(bool);

    inline ResizeMode resizeMode() const { return _resizeMode; }
    void setResizeMode(ResizeMode);

signals:
    void clickableActivated(const Clickable& click);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEvent*) override;
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    int posToCursor(const QPointF& pos);

private:
    QSize _sizeHint;
    QTextOption::WrapMode _wrapMode{QTextOption::NoWrap};
    Qt::Alignment _alignment;
    QTextLayout _layout;
    ClickableList _clickables;
    bool _toolTipEnabled{true};
    ResizeMode _resizeMode{NoResize};

    QList<QTextLayout::FormatRange> _layoutList;
    QVector<QTextLayout::FormatRange> _extraLayoutList;

    void layout();
    void updateSizeHint();
    void updateToolTip();

    void setHoverMode(int start, int length);
    void endHoverMode();
};
