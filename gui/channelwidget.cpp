/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#include "channelwidget.h"
#include "guiproxy.h"

#include <QtGui>
#include <iostream>

ChannelWidget::ChannelWidget(QString netname, QString bufname, QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  _networkName = netname;
  _bufferName = bufname;
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  //ui.inputEdit->setFocus();

  // Define standard colors
  stdCol = QColor("black");
  noticeCol = QColor("darkblue");
  serverCol = QColor("darkblue");
  errorCol = QColor("red");
  joinCol = QColor("green");
  quitCol = QColor("firebrick");
  partCol = QColor("firebrick");
  
}

void ChannelWidget::enterPressed() {
  emit sendMessage(networkName(), bufferName(), ui.inputEdit->text());
  ui.inputEdit->clear();
}

void ChannelWidget::recvMessage(Message msg) {
  QString s;
  QColor c = stdCol;
  switch(msg.type) {
    case Message::Server:
      c = serverCol; s = msg.msg;
      break;
    case Message::Error:
      c = errorCol; s = msg.msg;
      break;
    default:
      c = stdCol; s = QString("[%1] %2").arg(msg.sender).arg(msg.msg);
      break;
  }
  ui.chatWidget->setTextColor(c);
  ui.chatWidget->insertPlainText(QString("%1\n").arg(s));
  ui.chatWidget->ensureCursorVisible();
}

void ChannelWidget::recvStatusMsg(QString msg) {
  ui.chatWidget->insertPlainText(QString("[STATUS] %1").arg(msg));
  ui.chatWidget->ensureCursorVisible();
}

void ChannelWidget::setTopic(QString topic) {
  ui.topicEdit->setText(topic);
}

void ChannelWidget::setNicks(QStringList nicks) {


}

/**********************************************************************************************/


IrcWidget::IrcWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  ui.tabWidget->removeTab(0);

  connect(guiProxy, SIGNAL(csSendMessage(QString, QString, Message)), this, SLOT(recvMessage(QString, QString, Message)));
  connect(guiProxy, SIGNAL(csSendStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(guiProxy, SIGNAL(csSetTopic(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csSetNicks(QString, QString, QStringList)), this, SLOT(setNicks(QString, QString, QStringList)));
  connect(this, SIGNAL(sendMessage( QString, QString, QString )), guiProxy, SLOT(gsUserInput(QString, QString, QString)));
}

ChannelWidget * IrcWidget::getBuffer(QString net, QString buf) {
  QString key = net + buf;
  if(!buffers.contains(key)) {
    ChannelWidget *cw = new ChannelWidget(net, buf);
    connect(cw, SIGNAL(sendMessage(QString, QString, QString)), this, SLOT(userInput(QString, QString, QString)));
    ui.tabWidget->addTab(cw, net+buf);
    ui.tabWidget->setCurrentWidget(cw);
    //cw->setFocus();
    buffers[key] = cw;
  }
  return buffers[key];
}


void IrcWidget::recvMessage(QString net, QString buf, Message msg) {
  ChannelWidget *cw = getBuffer(net, buf);
  cw->recvMessage(msg);
}

void IrcWidget::recvStatusMsg(QString net, QString msg) {
  recvMessage(net, "", QString("[STATUS] %1").arg(msg));

}

void IrcWidget::userInput(QString net, QString buf, QString msg) {
  emit sendMessage(net, buf, msg);
}

void IrcWidget::setTopic(QString net, QString buf, QString topic) {
  ChannelWidget *cw = getBuffer(net, buf);
  cw->setTopic(topic);
}

void IrcWidget::setNicks(QString net, QString buf, QStringList nicks) {
  ChannelWidget *cw = getBuffer(net, buf);
  cw->setNicks(nicks);
}

