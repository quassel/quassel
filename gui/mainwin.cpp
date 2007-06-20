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

#include "gui.h"
#include "util.h"
#include "global.h"
#include "message.h"
#include "guiproxy.h"

#include "mainwin.h"
#include "buffer.h"
#include "bufferviewwidget.h"
#include "serverlist.h"
#include "coreconnectdlg.h"
#include "settingsdlg.h"
#include "settingspages.h"

MainWin::MainWin() : QMainWindow() {
  ui.setupUi(this);
  //widget = 0;
  //qDebug() << "Available DB drivers: " << QSqlDatabase::drivers ();
  setWindowTitle("Quassel IRC");
  //setWindowTitle("Κυασελ Εγαρζη");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  //workspace = new QWorkspace(this);
  //setCentralWidget(workspace);
  statusBar()->showMessage(tr("Waiting for core..."));
}

void MainWin::init() {
/*
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
*/
  ui.bufferWidget->init();

  show();
  //syncToCore();
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
  //foreach(Buffer *buf, buffers.values()) delete buf;
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
  connect(this, SIGNAL(importOldBacklog()), ClientProxy::instance(), SLOT(gsImportBacklog()));
}

void MainWin::setupViews() {
  
  BufferTreeModel *model = Client::bufferModel(); // FIXME Where is the delete for that? :p
  connect(model, SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));
  //connect(this, SIGNAL(bufferSelected(Buffer *)), model, SLOT(selectBuffer(Buffer *)));
  //connect(this, SIGNAL(bufferUpdated(Buffer *)), model, SLOT(bufferUpdated(Buffer *)));
  //connect(this, SIGNAL(bufferActivity(Buffer::ActivityLevel, Buffer *)), model, SLOT(bufferActivity(Buffer::ActivityLevel, Buffer *)));
  
  BufferViewDock *all = new BufferViewDock(model, tr("All Buffers"), BufferViewFilter::AllNets);
  registerBufferViewDock(all);
  
  BufferViewDock *allchans = new BufferViewDock(model, tr("All Channels"), BufferViewFilter::AllNets|BufferViewFilter::NoQueries|BufferViewFilter::NoServers);
  registerBufferViewDock(allchans);
  
  BufferViewDock *allqrys = new BufferViewDock(model, tr("All Queries"), BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoServers);
  registerBufferViewDock(allqrys);

  
  BufferViewDock *allnets = new BufferViewDock(model, tr("All Networks"), BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoQueries);
  registerBufferViewDock(allnets);


  ui.menuViews->addSeparator();
}

void MainWin::registerBufferViewDock(BufferViewDock *dock) {
  addDockWidget(Qt::LeftDockWidgetArea, dock);
  dock->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
  netViews.append(dock);
  ui.menuViews->addAction(dock->toggleViewAction());
  
  /*
  connect(this, SIGNAL(bufferSelected(Buffer *)), view, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferDestroyed(Buffer *)), view, SLOT(bufferDestroyed(Buffer *)));
  connect(view, SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));
  view->setBuffers(buffers.values());
   */
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
  showBuffer(Client::buffer(id));
}

void MainWin::showBuffer(Buffer *b) {
  currentBuffer = b->bufferId().groupId();
  //emit bufferSelected(b);
  //qApp->processEvents();
  ui.bufferWidget->setBuffer(b);
  //emit bufferSelected(b);
}

void MainWin::importBacklog() {
  if(QMessageBox::warning(this, "Import old backlog?", "Do you want to import your old file-based backlog into new the backlog database?<br>"
                                "<b>This will permanently delete the contents of your database!</b>",
                                QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
    emit importOldBacklog();
  }
}
