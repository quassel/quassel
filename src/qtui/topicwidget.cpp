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

#include "client.h"
#include "networkmodel.h"

TopicWidget::TopicWidget(QWidget *parent)
  : AbstractItemView(parent)
{
  ui.setupUi(this);
  ui.topicLineEdit->hide();
  ui.topicLineEdit->installEventFilter(this);
  ui.topicLabel->show();
}

void TopicWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);
  setTopic(current.sibling(current.row(), 1).data().toString());
}

void TopicWidget::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QItemSelectionRange changedArea(topLeft, bottomRight);
  QModelIndex currentTopicIndex = selectionModel()->currentIndex().sibling(selectionModel()->currentIndex().row(), 1);
  if(changedArea.contains(currentTopicIndex))
    setTopic(currentTopicIndex.data().toString());
};

void TopicWidget::setTopic(const QString &newtopic) {
  if(_topic == newtopic)
    return;
  
  _topic = newtopic;
  ui.topicLabel->setText(newtopic);
  ui.topicLineEdit->setText(newtopic);
  switchPlain();
}

void TopicWidget::on_topicLineEdit_returnPressed() {
  QModelIndex currentIdx = currentIndex();
  if(currentIdx.isValid() && currentIdx.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer) {
    BufferInfo bufferInfo = currentIdx.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    Client::userInput(bufferInfo, QString("/topic %1").arg(ui.topicLineEdit->text()));
  }
  switchPlain();
}

void TopicWidget::on_topicEditButton_clicked() {
  switchEditable();
}

void TopicWidget::switchEditable() {
  ui.topicLabel->hide();
  ui.topicEditButton->hide();
  ui.topicLineEdit->show();
  ui.topicLineEdit->setFocus();
}

void TopicWidget::switchPlain() {
  ui.topicLineEdit->hide();
  ui.topicLabel->show();
  ui.topicEditButton->show();
  ui.topicLineEdit->setText(_topic);
}

// filter for the input widget to switch back to normal mode
bool TopicWidget::eventFilter(QObject *obj, QEvent *event) {
  if(event->type() == QEvent::FocusOut) {
    switchPlain();
    return true;
  }

  if(event->type() != QEvent::KeyPress)
    return QObject::eventFilter(obj, event);

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

  if(keyEvent->key() == Qt::Key_Escape) {
    switchPlain();
    return true;
  }
  
  return false;
}

