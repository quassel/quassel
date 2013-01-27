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

#include "colorbutton.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrame>

#ifdef HAVE_KDE
#  include <KColorDialog>
#else
#  include <QColorDialog>
#endif

ColorButton::ColorButton(QWidget *parent) : QToolButton(parent)
{
    setText("");
    connect(this, SIGNAL(clicked()), SLOT(chooseColor()));
}


void ColorButton::setColor(const QColor &color)
{
    _color = color;
    QPixmap pixmap(QSize(32, 32));
    pixmap.fill(color);
    setIcon(pixmap);

    emit colorChanged(color);
}


QColor ColorButton::color() const
{
    return _color;
}


void ColorButton::chooseColor()
{
#ifdef HAVE_KDE
    QColor c = color();
    KColorDialog::getColor(c, this);
#else
    QColor c = QColorDialog::getColor(color(), this);
#endif

    if (c.isValid()) {
        setColor(c);
    }
}
