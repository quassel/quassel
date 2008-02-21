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
  //TODO: work on a good button style solution
  QPushButton::paintEvent(event);
  QPainter painter(this);
  int border = QApplication::style()->pixelMetric(QStyle::PM_ButtonMargin);

  // if twice buttonMargin (+2 px from the adjust) is greater than the button height
  // then set the border to a third of the button height.
  if(2*border+2 >= event->rect().height()) border = event->rect().height()/3;

  QBrush brush;
  if(isEnabled()) {
    brush = QBrush(_color);
  } else {
    brush = QBrush(_color, Qt::Dense4Pattern);
  }
  painter.fillRect(rect().adjusted(border+1, border+1, -border-1, -border-1), brush);
  QStyleOptionFrame option;
  option.state = QStyle::State_Sunken;
  option.rect = rect().adjusted(border, border, -border, -border);

  QApplication::style()->drawPrimitive(QStyle::PE_Frame, &option, &painter);
}
