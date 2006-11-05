/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#include "buffer.h"
#include "util.h"

Buffer::Buffer(QString netname, QString bufname) {
  networkName = netname;
  bufferName = bufname;

  widget = 0;
  active = false;
}

Buffer::~Buffer() {
  delete widget;
}

void Buffer::setActive(bool a) {
  if(a != active) {
    active = a;
    if(widget) widget->setActive(a);
  }
}

void Buffer::displayMsg(Message msg) {
  contents.append(msg);
  if(widget) widget->displayMsg(msg);
}

void Buffer::userInput(QString msg) {
  emit userInput(networkName, bufferName, msg);
}

void Buffer::scrollToEnd() {
  if(!widget) return;
  widget->scrollToEnd();
}

QWidget * Buffer::showWidget(QWidget *parent) {
  if(widget) return qobject_cast<QWidget*>(widget);
  widget = new BufferWidget(networkName, bufferName, isActive(), ownNick, contents, parent); 
  widget->setTopic(topic);
  widget->updateNickList(nicks);
  //widget->renderContents();
  widget->scrollToEnd();
  connect(widget, SIGNAL(userInput(QString)), this, SLOT(userInput(QString)));
  return qobject_cast<QWidget*>(widget);
}

void Buffer::hideWidget() {
  delete widget;
  widget = 0;
}

QWidget * Buffer::getWidget() {
  return qobject_cast<QWidget*>(widget);
}

void Buffer::setTopic(QString t) {
  topic = t;
  if(widget) widget->setTopic(t);
}

void Buffer::addNick(QString nick, VarMap props) {
  if(nick == ownNick) setActive(true);
  nicks[nick] = props;
  if(widget) widget->updateNickList(nicks);
}

void Buffer::updateNick(QString nick, VarMap props) {
  nicks[nick] = props;
  if(widget) widget->updateNickList(nicks);
}

void Buffer::renameNick(QString oldnick, QString newnick) {
  QVariant v = nicks.take(oldnick);
  nicks[newnick] = v;
  if(widget) widget->updateNickList(nicks);
}

void Buffer::removeNick(QString nick) {
  if(nick == ownNick) setActive(false);
  nicks.remove(nick);
  if(widget) widget->updateNickList(nicks);
}

void Buffer::setOwnNick(QString nick) {
  ownNick = nick;
  if(widget) widget->setOwnNick(nick);
}

/****************************************************************************************/

BufferWidget::BufferWidget(QString netname, QString bufname, bool act, QString own, QList<Message> cont, QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  networkName = netname;
  bufferName = bufname;
  active = act;
  contents = cont;
  ui.ownNick->clear();
  ui.ownNick->addItem(own);
  if(bufname.isEmpty()) {
    // Server Buffer
    ui.nickTree->hide();
    ui.topicEdit->hide();
    ui.chanSettingsButton->hide();
  }
  connect(ui.nickTree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(itemExpansionChanged(QTreeWidgetItem*)));
  connect(ui.nickTree, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(itemExpansionChanged(QTreeWidgetItem*)));
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));

  ui.chatWidget->setFocusProxy(ui.inputEdit);

  opsExpanded = voicedExpanded = usersExpanded = true;

  // Define standard colors
  stdCol = "black";
  inactiveCol = "grey";
  noticeCol = "darkblue";
  serverCol = "darkblue";
  errorCol = "red";
  joinCol = "green";
  quitCol = "firebrick";
  partCol = "firebrick";
  kickCol = "firebrick";
  nickCol = "magenta";

  int i = contents.count() - 100;
  if(i < 0) i = 0;
  for(int j = 0; j < i; j++) contents.removeAt(0);
  renderContents();
  updateTitle();
  show();
}

void BufferWidget::updateTitle() {
  QString title = QString("%1 in %2 [%3]: %4").arg(ui.ownNick->currentText()).arg(bufferName).arg(networkName).arg(ui.topicEdit->text());
  setWindowTitle(title);
}

void BufferWidget::enterPressed() {
  QStringList lines = ui.inputEdit->text().split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    emit userInput(msg);
  }
  ui.inputEdit->clear();
}

void BufferWidget::setActive(bool act) {
  if(act != active) {
    active = act;
    renderContents();
  }
}

void BufferWidget::renderContents() {
  QString html;
  for(int i = 0; i < contents.count(); i++) {
    html += htmlFromMsg(contents[i]);
  }
  ui.chatWidget->clear();
  ui.chatWidget->setHtml(html);
  scrollToEnd();
}

void BufferWidget::scrollToEnd() {
  QScrollBar *sb = ui.chatWidget->verticalScrollBar();
  sb->setValue(sb->maximum());
  qDebug() << bufferName << "scrolled" << sb->value() << sb->maximum();
}

QString BufferWidget::htmlFromMsg(Message msg) {
  QString s, n;
  QString c = stdCol;
  QString user = userFromMask(msg.sender);
  QString host = hostFromMask(msg.sender);
  QString nick = nickFromMask(msg.sender);
  switch(msg.type) {
    case Message::Plain:
      c = stdCol; n = QString("&lt;%1&gt;").arg(nick); s = msg.text;
      break;
    case Message::Server:
      c = serverCol; s = msg.text;
      break;
    case Message::Error:
      c = errorCol; s = msg.text;
      break;
    case Message::Join:
      c = joinCol;
      s = QString(tr("--> %1 (%2@%3) has joined %4")).arg(nick).arg(user).arg(host).arg(bufferName);
      break;
    case Message::Part:
      c = partCol;
      s = QString(tr("<-- %1 (%2@%3) has left %4")).arg(nick).arg(user).arg(host).arg(bufferName);
      if(!msg.text.isEmpty()) s = QString("%1 (%2)").arg(s).arg(msg.text);
      break;
    case Message::Kick:
      { c = kickCol;
        QString victim = msg.text.section(" ", 0, 0);
        if(victim == ui.ownNick->currentText()) victim = tr("you");
        QString kickmsg = msg.text.section(" ", 1);
        s = QString(tr("--> %1 has kicked %2 from %3")).arg(nick).arg(victim).arg(bufferName);
        if(!kickmsg.isEmpty()) s = QString("%1 (%2)").arg(s).arg(kickmsg);
      }
      break;
    case Message::Quit:
      c = quitCol;
      s = QString(tr("<-- %1 (%2@%3) has quit")).arg(nick).arg(user).arg(host);
      if(!msg.text.isEmpty()) s = QString("%1 (%2)").arg(s).arg(msg.text);
      break;
    case Message::Nick:
      c = nickCol;
      if(nick == msg.text) s = QString(tr("<-> You are now known as %1")).arg(msg.text);
      else s = QString(tr("<-> %1 is now known as %2")).arg(nick).arg(msg.text);
      break;
    case Message::Mode:
      c = serverCol;
      if(nick.isEmpty()) s = tr("*** User mode: %1").arg(msg.text);
      else s = tr("*** Mode %1 by %2").arg(msg.text).arg(nick);
      break;
    default:
      c = stdCol; n = QString("[%1]").arg(msg.sender); s = msg.text;
      break;
  }
  if(!active) c = inactiveCol;
  s.replace('&', "&amp;"); s.replace('<', "&lt;"); s.replace('>', "&gt;");
  QString html = QString("<table cellspacing=0 cellpadding=0><tr>"
      "<td width=50><div style=\"color:%2;\">[%1]</div></td>")
      .arg(msg.timeStamp.toLocalTime().toString("hh:mm:ss")).arg("darkblue");
  if(!n.isEmpty())
    html += QString("<td width=100><div align=right style=\"white-space:pre;margin-left:6px;color:%2;\">%1</div></td>")
        .arg(n).arg("royalblue");
  html += QString("<td><div style=\"white-space:pre-wrap;margin-left:6px;color:%2;\">%1</div></td>""</tr></table>").arg(s).arg(c);
  return html;
}

void BufferWidget::displayMsg(Message msg) {
  contents.append(msg);
  ui.chatWidget->append(htmlFromMsg(msg));
}

void BufferWidget::setOwnNick(QString nick) {
  ui.ownNick->clear();
  ui.ownNick->addItem(nick);
  updateTitle();
}

void BufferWidget::setTopic(QString topic) {
  ui.topicEdit->setText(topic);
  updateTitle();
}

void BufferWidget::updateNickList(VarMap nicks) {
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
    QString mode = nicks[nick].toMap()["Channels"].toMap()[bufferName].toMap()["Mode"].toString();
    if(mode.contains('o')) { new QTreeWidgetItem(ops, QStringList(QString("@%1").arg(nick))); }
    else if(mode.contains('v')) { new QTreeWidgetItem(voiced, QStringList(QString("+%1").arg(nick))); }
    else new QTreeWidgetItem(users, QStringList(nick));
  }
  if(ops->childCount()) {
    ops->setText(0, tr("%1 Operators").arg(ops->childCount()));
    ui.nickTree->addTopLevelItem(ops);
    ops->setExpanded(opsExpanded);
  } else delete ops;
  if(voiced->childCount()) {
    voiced->setText(0, tr("%1 Voiced").arg(voiced->childCount()));
    ui.nickTree->addTopLevelItem(voiced);
    voiced->setExpanded(voicedExpanded);
  } else delete voiced;
  if(users->childCount()) {
    users->setText(0, tr("%1 Users").arg(users->childCount()));
    ui.nickTree->addTopLevelItem(users);
    users->setExpanded(usersExpanded);
  } else delete users;
}

void BufferWidget::itemExpansionChanged(QTreeWidgetItem *item) {
  if(item->child(0)->text(0).startsWith('@')) opsExpanded = item->isExpanded();
  else if(item->child(0)->text(0).startsWith('+')) voicedExpanded = item->isExpanded();
  else usersExpanded = item->isExpanded();
}

