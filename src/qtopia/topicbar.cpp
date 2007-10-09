/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "topicbar.h"

#include <QtGui>


TopicBar::TopicBar(QWidget *parent) : QPushButton(parent) {
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

  // Define the font and calculate the metrics for it
  topicFont = font();
  topicFont.setPointSize(5);

  // frameWidth = style()->pixelMetric(QStyle::PM_ButtonMargin); // Nice idea, but Qtopia's buttons are just too large...
  frameWidth = 3; // so we hardcode a more reasonable framewidth than 7
  setFixedHeight(QFontMetrics(topicFont).height() + 2*frameWidth);

  textWidth = 0;
  fillText = " *** ";
  oneshot = true;
  timer = new QTimer(this);
  timer->setInterval(20);
  connect(timer, SIGNAL(timeout()), this, SLOT(updateOffset()));
  connect(this, SIGNAL(clicked()), this, SLOT(startScrolling()));
}

TopicBar::~TopicBar() {


}

void TopicBar::resizeEvent(QResizeEvent *event) {
  QPushButton::resizeEvent(event);
  calcTextMetrics();
}

void TopicBar::calcTextMetrics() {
  int w = width() - 2*frameWidth;
  QRect boundingRect = QFontMetrics(topicFont).boundingRect(text);
  textWidth = boundingRect.width();
  if(textWidth <= w) {
    offset = 0; fillTextStart = -1; secondTextStart = -1;
    displayText = text;
    timer->stop();
  } else {
    fillTextStart = textWidth;
    boundingRect = QFontMetrics(topicFont).boundingRect(fillText);
    secondTextStart = fillTextStart + boundingRect.width();
    displayText = QString("%1%2%1").arg(text).arg(fillText);
    offset = 0;
    timer->start();
  }
}

// TODO catch resizeEvent for scroll settings
void TopicBar::setContents(QString t, bool _oneshot) {
  text = t; oneshot = _oneshot;
  calcTextMetrics();
}

void TopicBar::paintEvent(QPaintEvent *event) {
  QPushButton::paintEvent(event);

  QPainter painter(this);
  painter.setFont(topicFont);
  painter.setClipRect(frameWidth, frameWidth, rect().width() - 2*frameWidth, rect().height() - 2*frameWidth);
  painter.drawText(QPoint(-offset + frameWidth, QFontMetrics(topicFont).ascent() + frameWidth), displayText);

}

void TopicBar::updateOffset() {
  offset++;
  if(offset >= secondTextStart) {
    offset = 0;
    if(oneshot) timer->stop(); // only scroll once!
  }
  update();
}

void TopicBar::startScrolling() {
  if(displayText.length() > text.length()) {
    oneshot = false;
    timer->start();
  }
}

void TopicBar::stopScrolling() {
  oneshot = true;
}
