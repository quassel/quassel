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

#include <QtGui>
#include <QtCore>
#include <QSqlDatabase>

#include "util.h"
#include "global.h"
#include "message.h"
#include "guiproxy.h"

#include "mainwin.h"
#include "buffer.h"
#include "networkview.h"
#include "serverlist.h"
#include "coreconnectdlg.h"
#include "settingsdlg.h"
#include "settingspages.h"

LayoutThread *layoutThread;

MainWin::MainWin() : QMainWindow() {
  ui.setupUi(this);
  //widget = 0;
  qDebug() << "Available DB drivers: " << QSqlDatabase::drivers ();
  setWindowTitle("Quassel IRC");
  //setWindowTitle("Κυασελ Εγαρζη");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  layoutTimer = new QTimer(this);
  layoutTimer->setInterval(0);
  layoutTimer->setSingleShot(false);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(layoutMsg()));
  //workspace = new QWorkspace(this);
  //setCentralWidget(workspace);
  statusBar()->showMessage(tr("Waiting for core..."));
}

void MainWin::init() {

  connect(guiProxy, SIGNAL(csServerState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  connect(guiProxy, SIGNAL(csServerConnected(QString)), this, SLOT(networkConnected(QString)));
  connect(guiProxy, SIGNAL(csServerDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  connect(guiProxy, SIGNAL(csDisplayMsg(Message)), this, SLOT(recvMessage(Message)));
  connect(guiProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(guiProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csNickAdded(QString, QString, VarMap)), this, SLOT(addNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  connect(guiProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csNickUpdated(QString, QString, VarMap)), this, SLOT(updateNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  connect(guiProxy, SIGNAL(csBacklogData(BufferId, QList<QVariant>, bool)), this, SLOT(recvBacklogData(BufferId, QList<QVariant>, bool)));
  connect(guiProxy, SIGNAL(csUpdateBufferId(BufferId)), this, SLOT(updateBufferId(BufferId)));
  connect(this, SIGNAL(sendInput(BufferId, QString)), guiProxy, SLOT(gsUserInput(BufferId, QString)));
  connect(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), guiProxy, SLOT(gsRequestBacklog(BufferId, QVariant, QVariant)));

  //layoutThread = new LayoutThread();
  //layoutThread->start();
  //while(!layoutThread->isRunning()) {};
  ui.bufferWidget->init();

  show();
  syncToCore();
  statusBar()->showMessage(tr("Ready."));
  systray = new QSystemTrayIcon(this);
  systray->setIcon(QIcon(":/qirc-icon.png"));
  systray->show();

  serverListDlg = new ServerListDlg(this);
  serverListDlg->setVisible(serverListDlg->showOnStartup());

  setupSettingsDlg();

  setupMenus();
  setupViews();

  QSettings s;
  s.beginGroup("Geometry");
  //resize(s.value("MainWinSize", QSize(500, 400)).toSize());
  //move(s.value("MainWinPos", QPoint(50, 50)).toPoint());
  if(s.contains("MainWinState")) restoreState(s.value("MainWinState").toByteArray());
  s.endGroup();

  /* make lookups by id faster */
  foreach(BufferId id, coreBuffers) {
    bufferIds[id.uid()] = id;  // make lookups by id faster
    getBuffer(id);             // create all buffers, so we see them in the network views
    emit requestBacklog(id, -1, -1);  // TODO: use custom settings for backlog request
  }

  s.beginGroup("Buffers");
  QString net = s.value("CurrentNetwork", "").toString();
  QString buf = s.value("CurrentBuffer", "").toString();
  s.endGroup();
  /*
  if(!net.isEmpty()) {
    if(buffers.contains(net)) {
      if(buffers[net].contains(buf)) {
        showBuffer(net, buf);
      } else {
        showBuffer(net, "");
      }
    }
  }
  */
}

MainWin::~MainWin() {
  //typedef QHash<QString, Buffer*> BufHash;
  //foreach(BufHash h, buffers.values()) {
  //  foreach(Buffer *b, h.values()) {
  //    delete b;
  //  }
  //}
  foreach(Buffer *buf, buffers.values()) delete buf;
}

/* This is implemented in settingspages.cpp */
/*
void MainWin::setupSettingsDlg() {

}
*/

void MainWin::setupMenus() {
  connect(ui.actionNetworkList, SIGNAL(triggered()), this, SLOT(showServerList()));
  connect(ui.actionEditIdentities, SIGNAL(triggered()), serverListDlg, SLOT(editIdentities()));
  connect(ui.actionSettingsDlg, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
  //ui.actionSettingsDlg->setEnabled(false);
  connect(ui.actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
  // for debugging
  connect(ui.actionImportBacklog, SIGNAL(triggered()), this, SLOT(importBacklog()));
  connect(this, SIGNAL(importOldBacklog()), guiProxy, SLOT(gsImportBacklog()));
}

void MainWin::setupViews() {
  NetworkView *all = new NetworkView(tr("All Buffers"), NetworkView::AllNets);
  registerNetView(all);
  addDockWidget(Qt::LeftDockWidgetArea, all);
  NetworkView *allchans = new NetworkView(tr("All Channels"), NetworkView::AllNets|NetworkView::NoQueries|NetworkView::NoServers);
  registerNetView(allchans);
  addDockWidget(Qt::LeftDockWidgetArea, allchans);
  NetworkView *allqrys = new NetworkView(tr("All Queries"), NetworkView::AllNets|NetworkView::NoChannels|NetworkView::NoServers);
  registerNetView(allqrys);
  addDockWidget(Qt::RightDockWidgetArea, allqrys);
  NetworkView *allnets = new NetworkView(tr("All Networks"), NetworkView::AllNets|NetworkView::NoChannels|NetworkView::NoQueries);
  registerNetView(allnets);
  addDockWidget(Qt::RightDockWidgetArea, allnets);

  ui.menuViews->addSeparator();
}

void MainWin::registerNetView(NetworkView *view) {
  connect(this, SIGNAL(bufferSelected(Buffer *)), view, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferUpdated(Buffer *)), view, SLOT(bufferUpdated(Buffer *)));
  connect(this, SIGNAL(bufferDestroyed(Buffer *)), view, SLOT(bufferDestroyed(Buffer *)));
  connect(view, SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));
  view->setBuffers(buffers.values());
  view->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
  netViews.append(view);
  ui.menuViews->addAction(view->toggleViewAction());
}

void MainWin::showServerList() {
//  if(!serverListDlg) {
//    serverListDlg = new ServerListDlg(this);
//  }
  serverListDlg->show();
  serverListDlg->raise();
}

void MainWin::showSettingsDlg() {
  settingsDlg->show();
}

void MainWin::closeEvent(QCloseEvent *event)
{
  //if (userReallyWantsToQuit()) {
    ui.bufferWidget->saveState();
    QSettings s;
    s.beginGroup("Geometry");
    s.setValue("MainWinSize", size());
    s.setValue("MainWinPos", pos());
    s.setValue("MainWinState", saveState());
    s.endGroup();
    s.beginGroup("Buffers");
    //s.setValue("CurrentNetwork", currentNetwork);
    s.setValue("CurrentBuffer", currentBuffer);
    s.endGroup();
    delete systray;
    event->accept();
  //} else {
    //event->ignore();
  //}
}

void MainWin::showBuffer(BufferId id) {
  showBuffer(getBuffer(id));
}

void MainWin::showBuffer(Buffer *b) {
  currentBuffer = b->bufferId().groupId();
  //emit bufferSelected(b);
  //qApp->processEvents();
  ui.bufferWidget->setBuffer(b);
  emit bufferSelected(b);
}

void MainWin::networkConnected(QString net) {
  connected[net] = true;
  //BufferId id = getStatusBufferId(net);
  //Buffer *b = getBuffer(id);
  //b->setActive(true);
  //b->displayMsg(Message(id, Message::Server, tr("Connected.")));  FIXME
  // TODO buffersUpdated();
}

void MainWin::networkDisconnected(QString net) {
  //getBuffer(net, "")->setActive(false);
  foreach(BufferId id, buffers.keys()) {
    if(id.network() != net) continue;
    Buffer *b = getBuffer(id);
    //b->displayMsg(Message(id, Message::Server, tr("Server disconnected."))); FIXME
    b->setActive(false);
  }
  connected[net] = false;
}

void MainWin::updateBufferId(BufferId id) {
  bufferIds[id.uid()] = id;  // make lookups by id faster
  getBuffer(id);
}

BufferId MainWin::getBufferId(QString net, QString buf) {
  foreach(BufferId id, buffers.keys()) {
    if(id.network() == net && id.buffer() == buf) return id;
  }
  Q_ASSERT(false);
  return BufferId();
}

BufferId MainWin::getStatusBufferId(QString net) {
  return getBufferId(net, "");
}


Buffer * MainWin::getBuffer(BufferId id) {
  if(!buffers.contains(id)) {
    Buffer *b = new Buffer(id);
    b->setOwnNick(ownNick[id.network()]);
    connect(b, SIGNAL(userInput(BufferId, QString)), this, SLOT(userInput(BufferId, QString)));
    connect(b, SIGNAL(bufferUpdated(Buffer *)), this, SIGNAL(bufferUpdated(Buffer *)));
    connect(b, SIGNAL(bufferDestroyed(Buffer *)), this, SIGNAL(bufferDestroyed(Buffer *)));
    buffers[id] = b;
    emit bufferUpdated(b);
  }
  return buffers[id];
}

void MainWin::recvNetworkState(QString net, QVariant state) {
  connected[net] = true;
  setOwnNick(net, state.toMap()["OwnNick"].toString());
  getBuffer(getStatusBufferId(net))->setActive(true);
  VarMap t = state.toMap()["Topics"].toMap();
  VarMap n = state.toMap()["Nicks"].toMap();
  foreach(QVariant v, t.keys()) {
    QString buf = v.toString();
    BufferId id = getBufferId(net, buf);
    getBuffer(id)->setActive(true);
    setTopic(net, buf, t[buf].toString());
  }
  foreach(QString nick, n.keys()) {
    addNick(net, nick, n[nick].toMap());
  }
}

void MainWin::recvMessage(Message msg) {
  /*
  Buffer *b;
  if(msg.flags & Message::PrivMsg) {
  // query
    if(msg.flags & Message::Self) b = getBuffer(net, msg.target);
    else b = getBuffer(net, nickFromMask(msg.sender));
  } else {
    b = getBuffer(net, msg.target);
  }
  */
  Buffer *b = getBuffer(msg.buffer);
  //b->displayMsg(msg);
  b->appendChatLine(new ChatLine(msg));
}

void MainWin::recvStatusMsg(QString net, QString msg) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));

}

void MainWin::recvBacklogData(BufferId id, QList<QVariant> msgs, bool done) {
  foreach(QVariant v, msgs) {
    layoutQueue.append(v.value<Message>());
  }
  if(!layoutTimer->isActive()) layoutTimer->start();
}


void MainWin::layoutMsg() {
  if(layoutQueue.count()) {
    ChatLine *line = new ChatLine(layoutQueue.takeFirst());
    getBuffer(line->bufferId())->prependChatLine(line);
  }
  if(!layoutQueue.count()) layoutTimer->stop();
}

void MainWin::userInput(BufferId id, QString msg) {
  emit sendInput(id, msg);
}

void MainWin::setTopic(QString net, QString buf, QString topic) {
  BufferId id = getBufferId(net, buf);
  if(!connected[id.network()]) return;
  Buffer *b = getBuffer(id);
  b->setTopic(topic);
  //if(!b->isActive()) {
  //  b->setActive(true);
  //  buffersUpdated();
  //}
}

void MainWin::addNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
  nicks[net][nick] = props;
  VarMap chans = props["Channels"].toMap();
  QStringList c = chans.keys();
  foreach(QString bufname, c) {
    getBuffer(getBufferId(net, bufname))->addNick(nick, props);
  }
}

void MainWin::renameNick(QString net, QString oldnick, QString newnick) {
  if(!connected[net]) return;
  QStringList chans = nicks[net][oldnick]["Channels"].toMap().keys();
  foreach(QString c, chans) {
    getBuffer(getBufferId(net, c))->renameNick(oldnick, newnick);
  }
  nicks[net][newnick] = nicks[net].take(oldnick);
}

void MainWin::updateNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
  QStringList oldchans = nicks[net][nick]["Channels"].toMap().keys();
  QStringList newchans = props["Channels"].toMap().keys();
  foreach(QString c, newchans) {
    if(oldchans.contains(c)) getBuffer(getBufferId(net, c))->updateNick(nick, props);
    else getBuffer(getBufferId(net, c))->addNick(nick, props);
  }
  foreach(QString c, oldchans) {
    if(!newchans.contains(c)) getBuffer(getBufferId(net, c))->removeNick(nick);
  }
  nicks[net][nick] = props;
}

void MainWin::removeNick(QString net, QString nick) {
  if(!connected[net]) return;
  VarMap chans = nicks[net][nick]["Channels"].toMap();
  foreach(QString bufname, chans.keys()) {
    getBuffer(getBufferId(net, bufname))->removeNick(nick);
  }
  nicks[net].remove(nick);
}

void MainWin::setOwnNick(QString net, QString nick) {
  if(!connected[net]) return;
  ownNick[net] = nick;
  foreach(BufferId id, buffers.keys()) {
    if(id.network() == net) {
      buffers[id]->setOwnNick(nick);
    }
  }
}

void MainWin::importBacklog() {
  if(QMessageBox::warning(this, "Import old backlog?", "Do you want to import your old file-based backlog into new the backlog database?<br>"
                                "<b>This will permanently delete the contents of your database!</b>",
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
    emit importOldBacklog();
  }
}
