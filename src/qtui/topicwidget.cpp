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

#include "topicwidget.h"

#include <QDebug>

TopicWidget::TopicWidget(QWidget *parent)
  : QWidget(parent)
{
  ui.setupUi(this);
  ui.topicLineEdit->hide();
  ui.topicLineEdit->installEventFilter(this);
  ui.topicButton->show();
}

void TopicWidget::setTopic(const QString &newtopic) {
  ui.topicButton->setAndStyleText(newtopic);
  ui.topicLineEdit->setText(newtopic);
  switchPlain();
}

void TopicWidget::on_topicLineEdit_returnPressed() {
  switchPlain();
  emit topicChanged(topic());
}

void TopicWidget::on_topicButton_clicked() {
  switchEditable();
}

void TopicWidget::switchEditable() {
  ui.topicButton->hide();
  ui.topicLineEdit->show();
  ui.topicLineEdit->setFocus();
}

void TopicWidget::switchPlain() {
  ui.topicLineEdit->hide();
  ui.topicButton->show();
}

bool TopicWidget::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() != QEvent::KeyPress)
    return QObject::eventFilter(obj, event);

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

  if(keyEvent->key() == Qt::Key_Escape) {
    switchPlain();
    return true;
  }
  
  return false;
}
