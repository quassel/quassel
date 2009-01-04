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
#include <QStyle>
#include <QStyleOptionFrame>

ColorButton::ColorButton(QWidget *parent) : QPushButton(parent) {

}

void ColorButton::setColor(const QColor &color) {
  _color = color;
  update();
}

QColor ColorButton::color() const {
  return _color;
}

/* This has been heavily inspired by KDE's KColorButton, thanks! */
void ColorButton::paintEvent(QPaintEvent *) {
  QPainter painter(this);

  QStyleOptionButton opt;
  initStyleOption(&opt);
  opt.state |= isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
  opt.features = QStyleOptionButton::None;
  if(isDefault())
    opt.features |= QStyleOptionButton::DefaultButton;

  // Draw bevel
  style()->drawControl(QStyle::CE_PushButtonBevel, &opt, &painter, this);

  // Calc geometry
  QRect labelRect = style()->subElementRect(QStyle::SE_PushButtonContents, &opt, this);
  int shift = style()->pixelMetric(QStyle::PM_ButtonMargin);
  labelRect.adjust(shift, shift, -shift, -shift);
  int x, y, w, h;
  labelRect.getRect(&x, &y, &w, &h);

  if(isChecked() || isDown()) {
    x += style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal);
    y += style()->pixelMetric(QStyle::PM_ButtonShiftVertical);
  }

  // Draw color rect
  QBrush brush = isEnabled() ? color() : palette().color(backgroundRole());
  qDrawShadePanel(&painter, x, y, w, h, palette(), true, 1, &brush);
}
