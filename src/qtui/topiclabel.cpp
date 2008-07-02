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

#include "topiclabel.h"


#include <QDebug>

#include <QApplication>
#include <QDesktopServices>
#include <QPainter>
// #include <QHBoxLayout>
#include <QFont>
#include <QFontMetrics>

#include "qtui.h"
#include "message.h"

TopicLabel::TopicLabel(QWidget *parent)
  : QFrame(parent),
    offset(0),
    dragStartX(0),
    dragMode(false)
{
}

void TopicLabel::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  textPartOffset.clear();
  
  QPainter painter(this);
  painter.setBackgroundMode(Qt::OpaqueMode);

  // FIXME re-enable topic painting
#ifndef SPUTDEV
  QRect drawRect = rect().adjusted(offset, 0, 0, 0);
  QRect brect;
  QString textPart;
  foreach(QTextLayout::FormatRange fr, styledContents.formatList) {
    textPart = styledContents.plainText.mid(fr.start, fr.length);
    textPartOffset << drawRect.left();
    painter.setFont(fr.format.font());
    painter.setPen(QPen(fr.format.foreground(), 0));
    painter.setBackground(fr.format.background());
    painter.drawText(drawRect, Qt::AlignLeft|Qt::AlignVCenter, textPart, &brect);
    drawRect.setLeft(brect.right());
  }
#endif
}

void TopicLabel::setText(const QString &text) {
  if(_text == text)
    return;

  _text = text;
  update();

#ifndef SPUTDEV
  styledContents = QtUi::style()->styleString(Message::mircToInternal(text));
  int height = 1;
  foreach(QTextLayout::FormatRange fr, styledContents.formatList) {
    height = qMax(height, QFontMetrics(fr.format.font()).height());
  }

  // ensure the label is editable (height != 1) if there is no text to show
  if(text.isEmpty())
    height = QFontMetrics(qApp->font()).height();
  
  // setFixedHeight(height);
#endif  
  // show topic in tooltip
  setToolTip(_text);
}


void TopicLabel::mouseMoveEvent(QMouseEvent *event) {
  if(!dragMode)
    return;

  event->accept();
  offset = event->pos().x() - dragStartX;
  update();
}

void TopicLabel::mousePressEvent(QMouseEvent *event) {
  event->accept();
  dragMode = true;
  dragStartX = event->pos().x() - offset;
}

void TopicLabel::mouseReleaseEvent(QMouseEvent *event) {
  event->accept();
  dragMode = false;
  if(qAbs(offset) < 10) {
    offset = 0;
    update();
  }
}

void TopicLabel::mouseDoubleClickEvent(QMouseEvent *event) {
#ifndef SPUTDEV
  event->accept();
  int textPart = 0;
  int textOffset = 0;

  if(textPartOffset.isEmpty())
    return;

  // find the text part that contains the url. We don't expect color codes in urls so we expect only full parts (yet?)
  int x = event->pos().x();
  while(textPart + 1 < textPartOffset.count()) {
    if(textPartOffset[textPart + 1] < x) {
      textPart++;
      textOffset = textPartOffset[textPart];
    } else {
      break;
    }
  }

  // we've Identified the needed text part \o/
  QString text = styledContents.plainText.mid(styledContents.formatList[textPart].start, styledContents.formatList[textPart].length);

  // now we have to find the the left and right word delimiters of the clicked word
  QFontMetrics fontMetric(styledContents.formatList[textPart].format.font());
  
  int start = 0;
  int spacePos = text.indexOf(" ");
  while(spacePos != -1) {
    if(fontMetric.width(text.left(spacePos + 1)) + textOffset < x) {
      start = spacePos + 1;
      spacePos = text.indexOf(" ", start + 1);
    } else {
      break;
    }
  }

  int end = text.indexOf(" ", start);
  int len = -1;
  if(end != -1) {
    len = end - start;
  }
  QString word = text.mid(start, len);
  qDebug() << word;
  QRegExp regex("^(h|f)t{1,2}ps?:\\/\\/");
  if(regex.indexIn(word) != -1) {
    QDesktopServices::openUrl(QUrl(word));
  }
#endif
  
}
