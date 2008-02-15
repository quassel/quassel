/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "colorbutton.h"

#include <QPainter>
#include <QDebug>
#include <QPaintEvent>
#include <QApplication>
#include <QStyle>
#include <QStyleOptionFrame>

ColorButton::ColorButton(QWidget *parent) :
    QPushButton(parent),
    _color(QColor()) //default is white; 
    {

}

void ColorButton::setColor(const QColor &color) {
  _color = color;
  update();
}

QColor ColorButton::color() const {
  return _color;
}

void ColorButton::paintEvent(QPaintEvent *event) {
  QPushButton::paintEvent(event);
  QPainter painter(this);
  int border = QApplication::style()->pixelMetric(QStyle::PM_ButtonMargin);
  painter.fillRect(rect().adjusted(border+1, border+1, -border-1, -border-1), QBrush(_color));
  QStyleOptionFrame option;
  option.state = QStyle::State_Sunken;
  option.rect = rect().adjusted(border, border, -border, -border);
  //TODO: setBackground instead of the fillRect()
  //painter.setBackground(_color);
  //painter.setBackgroundMode(Qt::OpaqueMode);
  //painter.fillRect(QApplication::style()->subElementRect(QStyle::SE_FrameContents, &option), QBrush(_color));
  //qDebug() << option << QApplication::style()->subElementRect(QStyle::SE_PushButtonContents, &option);
  QApplication::style()->drawPrimitive(QStyle::PE_Frame, &option, &painter);
  //painter.fillRect(QApplication::style()->subElementRect(QStyle::SE_FrameContents, &option), QBrush(_color));
  //border += QStyle::PM_DefaultFrameWidth;
  //painter.fillRect(rect().adjusted(border, border, -border, -border), QBrush(_color));
}
