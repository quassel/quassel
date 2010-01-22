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

#include "client.h"
#include "iconloader.h"
#include "networkmodel.h"
#include "uisettings.h"

TopicWidget::TopicWidget(QWidget *parent)
  : AbstractItemView(parent)
{
  ui.setupUi(this);
  ui.topicEditButton->setIcon(SmallIcon("edit-rename"));
  ui.topicLineEdit->setWordWrapEnabled(true);
  ui.topicLineEdit->installEventFilter(this);

  connect(ui.topicLabel, SIGNAL(clickableActivated(Clickable)), SLOT(clickableActivated(Clickable)));
  connect(ui.topicLineEdit, SIGNAL(noTextEntered()), SLOT(on_topicLineEdit_textEntered()));

  UiSettings s("TopicWidget");
  s.notify("DynamicResize", this, SLOT(updateResizeMode()));
  s.notify("ResizeOnHover", this, SLOT(updateResizeMode()));
  updateResizeMode();

  UiStyleSettings fs("Fonts");
  fs.notify("UseCustomTopicWidgetFont", this, SLOT(setUseCustomFont(QVariant)));
  fs.notify("TopicWidget", this, SLOT(setCustomFont(QVariant)));
  if(fs.value("UseCustomTopicWidgetFont", false).toBool())
    setCustomFont(fs.value("TopicWidget", QFont()));

  _mouseEntered = false;
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

void TopicWidget::setUseCustomFont(const QVariant &v) {
  if(v.toBool()) {
    UiStyleSettings fs("Fonts");
    setCustomFont(fs.value("TopicWidget").value<QFont>());
  } else
    setCustomFont(QFont());
}

void TopicWidget::setCustomFont(const QVariant &v) {
  UiStyleSettings fs("Fonts");
  if(!fs.value("UseCustomTopicWidgetFont", false).toBool())
    return;

  setCustomFont(v.value<QFont>());
}

void TopicWidget::setCustomFont(const QFont &f) {
  QFont font = f;
  if(font.family().isEmpty())
    font = QApplication::font();

  ui.topicLineEdit->setCustomFont(font);
  ui.topicLabel->setCustomFont(font);
}

void TopicWidget::setTopic(const QString &newtopic) {
  if(_topic == newtopic)
    return;

  _topic = newtopic;
  ui.topicLabel->setText(newtopic);
  ui.topicLineEdit->setText(newtopic);
  switchPlain();
}

void TopicWidget::updateResizeMode() {
  StyledLabel::ResizeMode mode = StyledLabel::NoResize;
  UiSettings s("TopicWidget");
  if(s.value("DynamicResize", true).toBool()) {
    if(s.value("ResizeOnHover", true).toBool())
      mode = StyledLabel::ResizeOnHover;
    else
      mode = StyledLabel::DynamicResize;
  }

  ui.topicLabel->setResizeMode(mode);
}

void TopicWidget::clickableActivated(const Clickable &click) {
  NetworkId networkId = selectionModel()->currentIndex().data(NetworkModel::NetworkIdRole).value<NetworkId>();
  click.activate(networkId, _topic);
}

void TopicWidget::on_topicLineEdit_textEntered() {
  QModelIndex currentIdx = currentIndex();
  if(currentIdx.isValid() && currentIdx.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer) {
    BufferInfo bufferInfo = currentIdx.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
    if(ui.topicLineEdit->text().isEmpty())
      Client::userInput(bufferInfo, QString("/quote TOPIC %1 :").arg(bufferInfo.bufferName()));
    else
      Client::userInput(bufferInfo, QString("/topic %1").arg(ui.topicLineEdit->text()));
  }
  switchPlain();
}

void TopicWidget::on_topicEditButton_clicked() {
  switchEditable();
}

void TopicWidget::switchEditable() {
  ui.stackedWidget->setCurrentIndex(1);
  ui.topicLineEdit->setFocus();
  ui.topicLineEdit->moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
  updateGeometry();
}

void TopicWidget::switchPlain() {
  ui.stackedWidget->setCurrentIndex(0);
  ui.topicLineEdit->setText(_topic);
  updateGeometry();
}

// filter for the input widget to switch back to normal mode
bool TopicWidget::eventFilter(QObject *obj, QEvent *event) {
  
  if(event->type() == QEvent::FocusOut && !_mouseEntered) {
    switchPlain();
    return true;
  }
  
  if(event->type() == QEvent::Enter) {
    _mouseEntered = true;
  }
  
  if(event->type() == QEvent::Leave) {
    _mouseEntered = false;
  }
  
  if(event->type() != QEvent::KeyRelease)
    return QObject::eventFilter(obj, event);

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

  if(keyEvent->key() == Qt::Key_Escape) {
    switchPlain();
    return true;
  }

  return false;
}
