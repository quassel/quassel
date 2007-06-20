/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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

#include "bufferwidget.h"
#include "buffer.h"
#include "chatline.h"
#include "chatwidget.h"
#include "settings.h"

BufferWidget::BufferWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);

  curBuf = 0;
  //setBaseSize(QSize(600,400));
  //setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  connect(this, SIGNAL(nickListUpdated(QStringList)), ui.inputEdit, SLOT(updateNickList(QStringList)));

}

void BufferWidget::init() {

}

BufferWidget::~BufferWidget() {
  foreach(BufferState *s, states.values()) {
    delete s;
  }
}

void BufferWidget::setBuffer(Buffer *buf) {
  BufferState *state;
  curBuf = buf;
  if(states.contains(buf)) {
    state = states[buf];
  } else {
    BufferState *s = new BufferState;
    s->currentLine = Settings::guiValue(QString("BufferStates/%1/%2/currentLine").arg(buf->networkName()).arg(buf->bufferName()), -1).toInt();
    if(buf->bufferType() == Buffer::ChannelBuffer) {
      s->splitterState = Settings::guiValue(QString("BufferStates/%1/%2/splitter").arg(buf->networkName()).arg(buf->bufferName())).toByteArray();
      s->splitter = new QSplitter(this);
      s->chatWidget = new ChatWidget(s->splitter);
      s->nickTree = new QTreeWidget(s->splitter);
      s->nickTree->headerItem()->setHidden(true);
      s->nickTree->setRootIsDecorated(false);
      s->page = s->splitter;
      s->ownNick = buf->ownNick();
      s->topic = buf->topic();
      updateNickList(s, buf->nickList());
      s->splitter->restoreState(s->splitterState);
      connect(buf, SIGNAL(nickListChanged(VarMap)), this, SLOT(updateNickList(VarMap)));
      connect(s->nickTree, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(itemExpansionChanged(QTreeWidgetItem*)));
      connect(s->nickTree, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(itemExpansionChanged(QTreeWidgetItem*)));
    } else {
      s->splitter = 0; s->nickTree = 0;
      s->chatWidget = new ChatWidget(this);
      s->page = s->chatWidget;
      s->ownNick = buf->ownNick();
      s->topic = buf->bufferName();
    }
    s->opsExpanded = Settings::guiValue(QString("BufferStates/%1/%2/opsExpanded").arg(buf->networkName()).arg(buf->bufferName()), true).toBool();
    s->voicedExpanded = Settings::guiValue(QString("BufferStates/%1/%2/voicedExpanded").arg(buf->networkName()).arg(buf->bufferName()), true).toBool();
    s->usersExpanded = Settings::guiValue(QString("BufferStates/%1/%2/usersExpanded").arg(buf->networkName()).arg(buf->bufferName()), true).toBool();
    states[buf] = s;
    state = s;
    state->chatWidget->init(networkName, bufferName);
    QList<ChatLine *> lines;
    QList<AbstractUiMsg *> msgs = buf->contents();
    foreach(AbstractUiMsg *msg, msgs) {
      lines.append(dynamic_cast<ChatLine*>(msg));
    }
    state->chatWidget->setContents(lines);
    connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), state->chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), state->chatWidget, SLOT(prependMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(topicSet(QString)), this, SLOT(setTopic(QString)));
    connect(buf, SIGNAL(ownNickSet(QString)), this, SLOT(setOwnNick(QString)));
    ui.stackedWidget->addWidget(s->page);
  }
  ui.stackedWidget->setCurrentWidget(state->page);
  ui.topicEdit->setText(buf->topic());
  chatWidget = state->chatWidget;
  nickTree = state->nickTree;
  splitter = state->splitter;
  //ui.ownNick->set
  disconnect(this, SIGNAL(userInput(QString)), 0, 0);
  connect(this, SIGNAL(userInput(QString)), buf, SLOT(processUserInput(QString)));
  state->chatWidget->setFocusProxy(ui.inputEdit);
  ui.inputEdit->setFocus();
  ui.topicEdit->setText(state->topic);
  ui.ownNick->clear();  // TODO add nick history
  ui.ownNick->addItem(state->ownNick);
  updateTitle();
}

void BufferWidget::saveState() {
  foreach(Buffer *buf, states.keys()) {
    BufferState *s = states[buf];
    if(s->splitter) Settings::setGuiValue(QString("BufferStates/%1/%2/splitter").arg(buf->networkName()).arg(buf->bufferName()), s->splitter->saveState());
    Settings::setGuiValue(QString("BufferStates/%1/%2/currentLine").arg(buf->networkName()).arg(buf->bufferName()), s->currentLine);
    Settings::setGuiValue(QString("BufferStates/%1/%2/opsExpanded").arg(buf->networkName()).arg(buf->bufferName()), s->opsExpanded);
    Settings::setGuiValue(QString("BufferStates/%1/%2/voicedExpanded").arg(buf->networkName()).arg(buf->bufferName()), s->voicedExpanded);
    Settings::setGuiValue(QString("BufferStates/%1/%2/usersExpanded").arg(buf->networkName()).arg(buf->bufferName()), s->usersExpanded);
  }
}

QSize BufferWidget::sizeHint() const {
  return QSize(800,400);
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

void BufferWidget::resizeEvent ( QResizeEvent * event ) {
  //qDebug() << "resizing:" << bufferName << event->size();
  QWidget::resizeEvent(event);

}

/*
void BufferWidget::displayMsg(Message msg) {
  chatWidget->appendMsg(msg);
}
*/

void BufferWidget::setOwnNick(QString nick) {
  Buffer *buf = qobject_cast<Buffer*>(sender());
  Q_ASSERT(buf);
  states[buf]->ownNick = nick;
  if(buf == curBuf) {
    ui.ownNick->clear();
    ui.ownNick->addItem(nick);
    updateTitle();
  }
}

void BufferWidget::setTopic(QString topic) {
  Buffer *buf = qobject_cast<Buffer*>(sender());
  Q_ASSERT(buf);
  states[buf]->topic = topic;
  if(buf == curBuf) {
    ui.topicEdit->setText(topic);
    updateTitle();
  }
}


void BufferWidget::updateNickList(VarMap nicks) {
  Buffer *buf = qobject_cast<Buffer*>(sender());
  Q_ASSERT(buf);
  updateNickList(states[buf], nicks);
}

// TODO Use 005
void BufferWidget::updateNickList(BufferState *state, VarMap nicks) {
  emit nickListUpdated(nicks.keys());
  QTreeWidget *tree = state->nickTree;
  if(!tree) return;
  tree->clear();
  if(nicks.count() != 1) tree->setHeaderLabel(tr("%1 Users").arg(nicks.count()));
  else tree->setHeaderLabel(tr("1 User"));
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
    tree->addTopLevelItem(ops);
    ops->setExpanded(state->opsExpanded);
  } else delete ops;
  if(voiced->childCount()) {
    voiced->setText(0, tr("%1 Voiced").arg(voiced->childCount()));
    tree->addTopLevelItem(voiced);
    voiced->setExpanded(state->voicedExpanded);
  } else delete voiced;
  if(users->childCount()) {
    users->setText(0, tr("%1 Users").arg(users->childCount()));
    tree->addTopLevelItem(users);
    users->setExpanded(state->usersExpanded);
  } else delete users;
}

// TODO Use 005 and additional user modes
void BufferWidget::itemExpansionChanged(QTreeWidgetItem *item) {
  if(item->child(0)->text(0).startsWith('@')) states[curBuf]->opsExpanded = item->isExpanded();
  else if(item->child(0)->text(0).startsWith('+')) states[curBuf]->voicedExpanded = item->isExpanded();
  else states[curBuf]->usersExpanded = item->isExpanded();
}

