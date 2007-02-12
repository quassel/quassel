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
#include "chatwidget.h"

Buffer::Buffer(QString netname, QString bufname) {
  networkName = netname;
  bufferName = bufname;

  widget = 0;
  chatWidget = 0;
  contentsWidget = 0;
  active = false;
}

Buffer::~Buffer() {
  delete widget;
  delete chatWidget;
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

/* FIXME do we need this? */
void Buffer::scrollToEnd() {
  if(!widget) return;
  //widget->scrollToEnd();
}

QWidget * Buffer::showWidget(QWidget *parent) {

  if(widget) {
    return qobject_cast<QWidget*>(widget);
  }

  if(!contentsWidget) {
    contentsWidget = new ChatWidgetContents(networkName, bufferName, 0);
    contentsWidget->hide();
    /* FIXME do we need this? */
    for(int i = 0; i < contents.count(); i++) {
      contentsWidget->appendMsg(contents[i]);
    }
  }
  contentsWidget->hide();
  widget = new BufferWidget(networkName, bufferName, isActive(), ownNick, contentsWidget, this, parent);
  widget->setTopic(topic);
  widget->updateNickList(nicks);
  connect(widget, SIGNAL(userInput(QString)), this, SLOT(userInput(QString)));
  return qobject_cast<QWidget*>(widget);
}

void Buffer::hideWidget() {
  delete widget;
  widget = 0;
}

void Buffer::deleteWidget() {
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



/****************************************************************************************/

BufferWidget::BufferWidget(QString netname, QString bufname, bool act, QString own, ChatWidgetContents *contents, Buffer *pBuf, QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  networkName = netname;
  bufferName = bufname;
  active = act;
  parentBuffer = pBuf;

  ui.chatWidget->init(netname, bufname, contents);

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

  opsExpanded = voicedExpanded = usersExpanded = true;

  ui.chatWidget->setFocusProxy(ui.inputEdit);
  updateTitle();
}

BufferWidget::~BufferWidget() {
  ui.chatWidget->takeWidget();   /* remove ownership so the chatwidget contents does not get destroyed */
  parentBuffer->deleteWidget();  /* make sure the parent buffer knows we are gone */
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
    //renderContents();
    //scrollToEnd();
  }
}

void BufferWidget::displayMsg(Message msg) {
  ui.chatWidget->appendMsg(msg);
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

