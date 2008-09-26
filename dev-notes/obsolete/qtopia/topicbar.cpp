/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QtGui>

#include "topicbar.h"
#include "client.h"


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
  timer->setInterval(25);
  connect(timer, SIGNAL(timeout()), this, SLOT(updateOffset()));
  connect(this, SIGNAL(clicked()), this, SLOT(startScrolling()));

  _model = Client::bufferModel();
  connect(_model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
          this, SLOT(dataChanged(QModelIndex, QModelIndex)));

  _selectionModel = Client::bufferModel()->standardSelectionModel();
  connect(_selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
          this, SLOT(currentChanged(QModelIndex, QModelIndex)));
}

TopicBar::~TopicBar() {


}

void TopicBar::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);
  setContents(current.sibling(current.row(), 1).data().toString());
}

void TopicBar::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QItemSelectionRange changedArea(topLeft, bottomRight);
  QModelIndex currentTopicIndex = _selectionModel->currentIndex().sibling(_selectionModel->currentIndex().row(), 1);
  if(changedArea.contains(currentTopicIndex))
    setContents(currentTopicIndex.data().toString());
};

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
    //timer->start();  // uncomment this to get autoscroll rather than on-demand
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
  offset+=1;
  if(offset >= secondTextStart) {
    offset = 0;
    if(oneshot) timer->stop(); // only scroll once!
  }
  update();
}

void TopicBar::startScrolling() {
  if(displayText.length() > text.length()) {
    //oneshot = false;
    timer->start();
  }
}

void TopicBar::stopScrolling() {
  oneshot = true;
}
