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
#include "global.h"
#include "util.h"

#include <QtGui>
#include <iostream>

ChannelWidget::ChannelWidget(QString netname, QString bufname, QString own, QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  _networkName = netname;
  _bufferName = bufname;
  ui.ownNick->clear();
  ui.ownNick->addItem(own);
  if(bufname.isEmpty()) {
    // Server Buffer
    ui.nickTree->hide();
    ui.topicEdit->hide();
    ui.chanSettingsButton->hide();
  }
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  //ui.inputEdit->setFocus();

  // Define standard colors
  stdCol = "black";
  noticeCol = "darkblue";
  serverCol = "darkblue";
  errorCol = "red";
  joinCol = "green";
  quitCol = "firebrick";
  partCol = "firebrick";
  kickCol = "firebrick";
  nickCol = "magenta";

}

void ChannelWidget::enterPressed() {
  emit sendInput(networkName(), bufferName(), ui.inputEdit->text());
  ui.inputEdit->clear();
}

void ChannelWidget::recvMessage(Message msg) {
  QString s, n;
  QString c = stdCol;
  QString user = userFromMask(msg.sender);
  QString host = hostFromMask(msg.sender);
  QString nick = nickFromMask(msg.sender);
  switch(msg.type) {
    case Message::Msg:
      c = stdCol; n = QString("&lt;%1&gt;").arg(nick); s = msg.msg;
      break;
    case Message::Server:
      c = serverCol; s = msg.msg;
      break;
    case Message::Error:
      c = errorCol; s = msg.msg;
      break;
    case Message::Join:
      c = joinCol;
      s = QString(tr("--&gt; %1 (%2@%3) has joined %4")).arg(nick).arg(user).arg(host).arg(bufferName());
      break;
    case Message::Part:
      c = partCol;
      s = QString(tr("&lt;-- %1 (%2@%3) has left %4")).arg(nick).arg(user).arg(host).arg(bufferName());
      if(!msg.msg.isEmpty()) s = QString("%1 (%2)").arg(s).arg(msg.msg);
      break;
    case Message::Kick:
      { c = kickCol;
        QString victim = msg.msg.section(" ", 0, 0);
        QString kickmsg = msg.msg.section(" ", 1);
        s = QString(tr("--&gt; %1 has kicked %2 from %3")).arg(nick).arg(victim).arg(bufferName());
        if(!kickmsg.isEmpty()) s = QString("%1 (%2)").arg(s).arg(kickmsg);
      }
      break;
    case Message::Quit:
      c = quitCol;
      s = QString(tr("&lt;-- %1 (%2@%3) has quit")).arg(nick).arg(user).arg(host);
      if(!msg.msg.isEmpty()) s = QString("%1 (%2)").arg(s).arg(msg.msg);
      break;
    case Message::Nick:
      c = nickCol;
      if(nick == msg.msg) s = QString(tr("&lt;-&gt; You are now known as %1")).arg(msg.msg);
      else s = QString(tr("&lt;-&gt; %1 is now known as %2")).arg(nick).arg(msg.msg);
      break;
    default:
      c = stdCol; n = QString("[%1]").arg(msg.sender); s = msg.msg;
      break;
  }
  QString html = QString("<table cellspacing=0 cellpadding=0><tr>"
      "<td width=50><div style=\"color:%2;\">[%1]</div></td>")
      .arg(msg.timeStamp.toLocalTime().toString("hh:mm:ss")).arg("darkblue");
  if(!n.isEmpty())
    html += QString("<td width=100><div align=right style=\"white-space:nowrap;margin-left:6px;color:%2;\">%1</div></td>")
        .arg(n).arg("mediumseagreen");
  html += QString("<td><div style=\"margin-left:6px;color:%2;\">%1</div></td>""</tr></table>")
      .arg(s).arg(c);
  ui.chatWidget->append(html);
  //ui.chatWidget->append(QString("<table border=1 cellspacing=0 cellpadding=0><tr><td>%1</td><td width=100 style=border-right-width:1px;><div style=margin-left:8px; margin-right:8px;>%2</div></td><td style=color:firebrick>&nbsp;%3</td></tr></table>")
      //.arg(msg.timeStamp.toLocalTime().toString("hh:mm:ss")).arg(nick).arg(s));
  //ui.chatWidget->setTextColor(stdCol);
  //ui.chatWidget->append(QString("[%1] ").arg(msg.timeStamp.toLocalTime().toString("hh:mm:ss")));
  //ui.chatWidget->setTextColor(c);
  //ui.chatWidget->append(s + "\n");
  //ui.chatWidget->insertHtml(QString("<table><tr><td>[12:13]</td><td width=20><div align=right>[nickname]</div></td><td>This is the Message!</td></tr>"
  //    "<tr><td>[12:13]</td><td><div align=right>[nick]</div></td><td>This is the Message!</td></tr>"
  //    "<tr><td>[12:13]</td><td><div align=right>[looongnickname]</div></td><td>This is the Message!</td></tr>"
  //    "<tr><td>[12:13]</td><td><div align=right>[nickname]</div></td><td>This is the Message!</td></tr></table>"
  //                                 ));
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

void ChannelWidget::addNick(QString nick, VarMap props) {
  nicks[nick] = props;
  updateNickList();
}

void ChannelWidget::updateNick(QString nick, VarMap props) {
  nicks[nick] = props;
  updateNickList();
}

void ChannelWidget::renameNick(QString oldnick, QString newnick) {
  QVariant v = nicks.take(oldnick);
  nicks[newnick] = v;
  updateNickList();
}

void ChannelWidget::removeNick(QString nick) {
  nicks[nick].toMap().remove(nick);
  updateNickList();
}

void ChannelWidget::setOwnNick(QString nick) {
  ui.ownNick->clear();
  ui.ownNick->addItem(nick);
}

void ChannelWidget::updateNickList() {
  ui.nickTree->clear();
  if(nicks.count() != 1) ui.nickTree->setHeaderLabel(tr("%1 Users").arg(nicks.count()));
  else ui.nickTree->setHeaderLabel(tr("1 User"));
  QTreeWidgetItem *ops = new QTreeWidgetItem();
  QTreeWidgetItem *voiced = new QTreeWidgetItem();
  QTreeWidgetItem *users = new QTreeWidgetItem();
  // To sort case-insensitive, we have to put all nicks in a map which is sorted by (lowercase) key...
  QMap<QString, QString> sorted;
  foreach(QString n, nicks.keys()) { sorted[n.toLower()] = n; }
  foreach(QString n, sorted.keys()) {
    QString nick = sorted[n];
    QString mode = nicks[nick].toMap()["Channels"].toMap()[bufferName()].toMap()["Mode"].toString();
    if(mode.contains('o')) { new QTreeWidgetItem(ops, QStringList(QString("@%1").arg(nick))); }
    else if(mode.contains('v')) { new QTreeWidgetItem(voiced, QStringList(QString("+%1").arg(nick))); }
    else new QTreeWidgetItem(users, QStringList(nick));
  }
  if(ops->childCount()) {
    ops->setText(0, tr("%1 Operators").arg(ops->childCount()));
   ui.nickTree->addTopLevelItem(ops);
    ops->setExpanded(true);
  } else delete ops;
  if(voiced->childCount()) {
    voiced->setText(0, tr("%1 Voiced").arg(voiced->childCount()));
    ui.nickTree->addTopLevelItem(voiced);
    voiced->setExpanded(true);
  } else delete voiced;
  if(users->childCount()) {
    users->setText(0, tr("%1 Users").arg(users->childCount()));
    ui.nickTree->addTopLevelItem(users);
    users->setExpanded(true);
  } else delete users;
}
/**********************************************************************************************/


IrcWidget::IrcWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  ui.tabWidget->removeTab(0);

  connect(guiProxy, SIGNAL(csDisplayMsg(QString, QString, Message)), this, SLOT(recvMessage(QString, QString, Message)));
  connect(guiProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(guiProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csSetNicks(QString, QString, QStringList)), this, SLOT(setNicks(QString, QString, QStringList)));
  connect(guiProxy, SIGNAL(csNickAdded(QString, QString, VarMap)), this, SLOT(addNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  connect(guiProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csNickUpdated(QString, QString, VarMap)), this, SLOT(updateNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  connect(this, SIGNAL(sendInput( QString, QString, QString )), guiProxy, SLOT(gsUserInput(QString, QString, QString)));
}

ChannelWidget * IrcWidget::getBuffer(QString net, QString buf) {
  QString key = net + buf;
  if(!buffers.contains(key)) {
    ChannelWidget *cw = new ChannelWidget(net, buf, ownNick[net]);
    connect(cw, SIGNAL(sendInput(QString, QString, QString)), this, SLOT(userInput(QString, QString, QString)));
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
  recvMessage(net, "", Message(Message::Server, QString("[STATUS] %1").arg(msg)));

}

void IrcWidget::userInput(QString net, QString buf, QString msg) {
  emit sendInput(net, buf, msg);
}

void IrcWidget::setTopic(QString net, QString buf, QString topic) {
  ChannelWidget *cw = getBuffer(net, buf);
  cw->setTopic(topic);
}

void IrcWidget::setNicks(QString net, QString buf, QStringList nicks) {
  ChannelWidget *cw = getBuffer(net, buf);
  cw->setNicks(nicks);
}

void IrcWidget::addNick(QString net, QString nick, VarMap props) {
  VarMap netnicks = nicks[net].toMap();
  netnicks[nick] = props;
  nicks[net] = netnicks;
  VarMap chans = props["Channels"].toMap();
  QStringList c = chans.keys();
  foreach(QString bufname, c) {
    getBuffer(net, bufname)->addNick(nick, props);
  }
}

void IrcWidget::renameNick(QString net, QString oldnick, QString newnick) {
  VarMap netnicks = nicks[net].toMap();
  qDebug() << "renNICK:"<<oldnick<<newnick;
  Q_ASSERT(netnicks.contains(oldnick));
  QStringList chans = netnicks[oldnick].toMap()["Channels"].toMap().keys(); qDebug() << "l:" << chans;
  foreach(QString c, chans) {
    qDebug() << net << c;
    getBuffer(net, c)->renameNick(oldnick, newnick);
  }
  QVariant v = netnicks.take(oldnick);
  netnicks[newnick] = v;
  nicks[net] = netnicks;
}

void IrcWidget::updateNick(QString net, QString nick, VarMap props) {
  QStringList oldchans = nicks[net].toMap()[nick].toMap()["Channels"].toMap().keys();
  QStringList newchans = props["Channels"].toMap().keys();
  foreach(QString c, newchans) {
    if(oldchans.contains(c)) getBuffer(net, c)->updateNick(nick, props);
    else getBuffer(net, c)->addNick(nick, props);
  }
  foreach(QString c, oldchans) {
    if(!newchans.contains(c)) getBuffer(net, c)->removeNick(nick);
  }
  VarMap netnicks = nicks[net].toMap();
  netnicks[nick] = props;
  nicks[net] = netnicks;
}

void IrcWidget::removeNick(QString net, QString nick) {
  VarMap chans = nicks[net].toMap()[nick].toMap()["Channels"].toMap();
  foreach(QString bufname, chans.keys()) {
    getBuffer(net, bufname)->removeNick(nick);
  }
  VarMap netnicks = nicks[net].toMap();
  netnicks.remove(nick);
  nicks[net] = netnicks;
}

void IrcWidget::setOwnNick(QString net, QString nick) {
  ownNick[net] = nick;
  foreach(ChannelWidget *cw, buffers.values()) {
    if(cw->networkName() == net) cw->setOwnNick(nick);
  }
}

