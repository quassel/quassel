/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel Project                          *
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

#include "topicbutton.h"


#include <QDebug>

#include <QApplication>
#include <QPainter>
#include <QHBoxLayout>
#include <QFont>
#include <QFontMetrics>

#include "qtui.h"
#include "message.h"

TopicButton::TopicButton(QWidget *parent)
  : QAbstractButton(parent),
    _sizeHint(QSize())
{
}

void TopicButton::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  QFontMetrics metrics(qApp->font());

  QPoint topLeft = rect().topLeft();
  int height = sizeHint().height();
  int width = 0;
  QRect drawRect;
  QString textPart;
  foreach(QTextLayout::FormatRange fr, styledText.formats) {
    textPart = styledText.text.mid(fr.start, fr.length);
    width = metrics.width(textPart);
    drawRect = QRect(topLeft, QPoint(topLeft.x() + width, topLeft.y() + height));
    // qDebug() << drawRect << textPart << width << fr.format.background();
    painter.setPen(QPen(fr.format.foreground(), 0));
    painter.setBackground(fr.format.background()); // no clue why this doesnt work properly o_O
    painter.drawText(drawRect, Qt::AlignLeft|Qt::TextSingleLine, textPart);
    topLeft.setX(topLeft.x() + width);
  }
}

void TopicButton::setAndStyleText(const QString &text) {
  styledText = QtUi::style()->styleString(Message::mircToInternal(text));
  setText(styledText.text);
  
  QFontMetrics metrics(qApp->font());
  _sizeHint = metrics.boundingRect(styledText.text).size();
}
