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

#include <QtGui>
#include <QtCore>

#include "global.h"
#include "message.h"
#include "guiproxy.h"

#include "mainwin.h"
#include "buffer.h"
#include "networkview.h"
#include "serverlist.h"
#include "coreconnectdlg.h"

MainWin::MainWin() : QMainWindow() {
  ui.setupUi(this);
  widget = 0;

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  QSettings s;
  s.beginGroup("Geometry");
  resize(s.value("MainWinSize", QSize(500, 400)).toSize());
  move(s.value("MainWinPos", QPoint(50, 50)).toPoint());
  s.endGroup();

  workspace = new QWorkspace(this);
  setCentralWidget(workspace);
  statusBar()->showMessage(tr("Waiting for core..."));

  netView = new NetworkView("", this);
  netView->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
  addDockWidget(Qt::LeftDockWidgetArea, netView);
  connect(netView, SIGNAL(bufferSelected(QString, QString)), this, SLOT(showBuffer(QString, QString)));
  connect(this, SIGNAL(bufferSelected(QString, QString)), netView, SLOT(selectBuffer(QString, QString)));

  connect(guiProxy, SIGNAL(csServerState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  connect(guiProxy, SIGNAL(csServerConnected(QString)), this, SLOT(networkConnected(QString)));
  connect(guiProxy, SIGNAL(csServerDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  connect(guiProxy, SIGNAL(csDisplayMsg(QString, Message)), this, SLOT(recvMessage(QString, Message)));
  connect(guiProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(guiProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csSetNicks(QString, QString, QStringList)), this, SLOT(setNicks(QString, QString, QStringList)));
  connect(guiProxy, SIGNAL(csNickAdded(QString, QString, VarMap)), this, SLOT(addNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  connect(guiProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  connect(guiProxy, SIGNAL(csNickUpdated(QString, QString, VarMap)), this, SLOT(updateNick(QString, QString, VarMap)));
  connect(guiProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  connect(this, SIGNAL(sendInput( QString, QString, QString )), guiProxy, SLOT(gsUserInput(QString, QString, QString)));

  show();
  syncToCore();
  statusBar()->showMessage(tr("Ready."));

  buffersUpdated();

  serverListDlg = new ServerListDlg(this);
  serverListDlg->setVisible(serverListDlg->showOnStartup());
  setupMenus();

  // replay backlog
  foreach(QString net, coreBackLog.keys()) {
    while(coreBackLog[net].count()) {
      recvMessage(net, coreBackLog[net].takeFirst());
    }
  }
  /*
  foreach(QString key, buffers.keys()) {
    foreach(Buffer *b, buffers[key].values()) {
      QWidget *widget = b->showWidget(this);
      workspace->addWindow(widget);
      widget->show();
    }
  }
  */
  s.beginGroup("Buffers");
  QString net = s.value("CurrentNetwork", "").toString();
  QString buf = s.value("CurrentBuffer", "").toString();
  s.endGroup();
  if(!net.isEmpty()) {
    if(buffers.contains(net)) {
      if(buffers[net].contains(buf)) {
        showBuffer(net, buf);
      } else {
        showBuffer(net, "");
      }
    }
  }
}

void MainWin::setupMenus() {
  connect(ui.actionNetworkList, SIGNAL(triggered()), this, SLOT(showServerList()));
  connect(ui.actionEditIdentities, SIGNAL(triggered()), serverListDlg, SLOT(editIdentities()));
}

void MainWin::showServerList() {
//  if(!serverListDlg) {
//    serverListDlg = new ServerListDlg(this);
//  }
  serverListDlg->show();
}

void MainWin::closeEvent(QCloseEvent *event)
{
  //if (userReallyWantsToQuit()) {
    QSettings s;
    s.beginGroup("Geometry");
    s.setValue("MainWinSize", size());
    s.setValue("MainWinPos", pos());
    s.endGroup();
    s.beginGroup("Buffers");
    s.setValue("CurrentNetwork", currentNetwork);
    s.setValue("CurrentBuffer", currentBuffer);
    s.endGroup();
    event->accept();
  //} else {
    //event->ignore();
  //}
}

void MainWin::showBuffer(QString net, QString buf) {
  currentBuffer = buf; currentNetwork = net;
  Buffer *b = getBuffer(net, buf);
  QWidget *old = widget;
  widget = b->showWidget(this);
  if(widget == old) return;
  workspace->addWindow(widget);
  widget->showMaximized();
  if(old) { old->close(); old->setParent(this); }
  workspace->setActiveWindow(widget);
  //widget->setFocus();
  //workspace->setFocus();
  //widget->activateWindow();
  widget->setFocus(Qt::MouseFocusReason);
  focusNextChild();
  //workspace->tile();
  emit bufferSelected(net, buf);
}

void MainWin::networkConnected(QString net) {
  connected[net] = true;
  Buffer *b = getBuffer(net, "");
  b->setActive(true);
  b->displayMsg(Message::server("", tr("Connected.")));
  buffersUpdated();
}

void MainWin::networkDisconnected(QString net) {
  //getBuffer(net, "")->setActive(false);
  foreach(QString buf, buffers[net].keys()) {
    Buffer *b = getBuffer(net, buf);
    b->displayMsg(Message::server(buf, tr("Server disconnected.")));
    b->setActive(false);
    
  }
  connected[net] = false;
}

Buffer * MainWin::getBuffer(QString net, QString buf) {
  if(!buffers[net].contains(buf)) {
    Buffer *b = new Buffer(net, buf);
    b->setOwnNick(ownNick[net]);
    connect(b, SIGNAL(userInput(QString, QString, QString)), this, SLOT(userInput(QString, QString, QString)));
    buffers[net][buf] = b;
    buffersUpdated();
  }
  return buffers[net][buf];
}

void MainWin::buffersUpdated() {
  netView->buffersUpdated(buffers);
}

void MainWin::recvNetworkState(QString net, QVariant state) {
  connected[net] = true;
  setOwnNick(net, state.toMap()["OwnNick"].toString());
  getBuffer(net, "")->setActive(true);
  VarMap t = state.toMap()["Topics"].toMap();
  VarMap n = state.toMap()["Nicks"].toMap();
  foreach(QString buf, t.keys()) {
    getBuffer(net, buf)->setActive(true);
    setTopic(net, buf, t[buf].toString());
  }
  foreach(QString nick, n.keys()) {
    addNick(net, nick, n[nick].toMap());
  }
  buffersUpdated();
}

void MainWin::recvMessage(QString net, Message msg) {
  Buffer *b = getBuffer(net, msg.target);
  b->displayMsg(msg);
}

void MainWin::recvStatusMsg(QString net, QString msg) {
  recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));

}

void MainWin::userInput(QString net, QString buf, QString msg) {
  emit sendInput(net, buf, msg);
}

void MainWin::setTopic(QString net, QString buf, QString topic) {
  if(!connected[net]) return;
  Buffer *b = getBuffer(net, buf);
  b->setTopic(topic);
  buffersUpdated();
  //if(!b->isActive()) {
  //  b->setActive(true);
  //  buffersUpdated();
  //}
}

void MainWin::setNicks(QString net, QString buf, QStringList nicks) {
  Q_ASSERT(false);
}

void MainWin::addNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
  nicks[net][nick] = props;
  VarMap chans = props["Channels"].toMap();
  QStringList c = chans.keys();
  foreach(QString bufname, c) {
    getBuffer(net, bufname)->addNick(nick, props);
  }
  buffersUpdated();
}

void MainWin::renameNick(QString net, QString oldnick, QString newnick) {
  if(!connected[net]) return;
  QStringList chans = nicks[net][oldnick]["Channels"].toMap().keys();
  foreach(QString c, chans) {
    getBuffer(net, c)->renameNick(oldnick, newnick);
  }
  nicks[net][newnick] = nicks[net].take(oldnick);
}

void MainWin::updateNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
  QStringList oldchans = nicks[net][nick]["Channels"].toMap().keys();
  QStringList newchans = props["Channels"].toMap().keys();
  foreach(QString c, newchans) {
    if(oldchans.contains(c)) getBuffer(net, c)->updateNick(nick, props);
    else getBuffer(net, c)->addNick(nick, props);
  }
  foreach(QString c, oldchans) {
    if(!newchans.contains(c)) getBuffer(net, c)->removeNick(nick);
  }
  nicks[net][nick] = props;
  buffersUpdated();
}

void MainWin::removeNick(QString net, QString nick) {
  if(!connected[net]) return;
  VarMap chans = nicks[net][nick]["Channels"].toMap();
  foreach(QString bufname, chans.keys()) {
    getBuffer(net, bufname)->removeNick(nick);
  }
  nicks[net].remove(nick);
  buffersUpdated();
}

void MainWin::setOwnNick(QString net, QString nick) {
  if(!connected[net]) return;
  ownNick[net] = nick;
  foreach(Buffer *b, buffers[net].values()) {
    b->setOwnNick(nick);
  }
}

