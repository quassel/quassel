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
  : QAbstractButton(parent)
{
  setFixedHeight(QFontMetrics(qApp->font()).height());
}

void TopicButton::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setBackgroundMode(Qt::OpaqueMode);

  QRect drawRect = rect();
  QRect brect;
  QString textPart;
  foreach(QTextLayout::FormatRange fr, styledText.formats) {
    textPart = styledText.text.mid(fr.start, fr.length);
    painter.setFont(fr.format.font());
    painter.setPen(QPen(fr.format.foreground(), 0));
    painter.setBackground(fr.format.background());
    painter.drawText(drawRect, Qt::AlignLeft|Qt::TextSingleLine, textPart, &brect);
    drawRect.setLeft(brect.right());
  }
}

void TopicButton::setAndStyleText(const QString &text) {
  if(QAbstractButton::text() == text)
    return;

  setText(text); // this triggers a repaint event

  styledText = QtUi::style()->styleString(Message::mircToInternal(text));
  int height = 1;
  foreach(QTextLayout::FormatRange fr, styledText.formats) {
    height = qMax(height, QFontMetrics(fr.format.font()).height());
  }

  // ensure the button is editable (height != 1) if there is no text to show
  if(text.isEmpty())
    height = QFontMetrics(qApp->font()).height();
  
  setFixedHeight(height);
}
