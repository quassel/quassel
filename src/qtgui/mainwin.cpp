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

#include "mainwin.h"

#include "bufferview.h"
#include "chatline.h"
#include "client.h"
#include "clientproxy.h"
#include "coreconnectdlg.h"
#include "serverlist.h"
#include "settingsdlg.h"
#include "settingspages.h"

MainWin::MainWin(QtGui *_gui, QWidget *parent) : QMainWindow(parent), gui(_gui) {
  ui.setupUi(this);
  //setWindowTitle("Quassel IRC");
  setWindowTitle(QString::fromUtf8("Κυασελ Εγαρζη"));
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  statusBar()->showMessage(tr("Waiting for core..."));
  
}

void MainWin::init() {
  connect(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), ClientProxy::instance(), SLOT(gsRequestBacklog(BufferId, QVariant, QVariant)));
  ui.bufferWidget->init();

  show();

  //VarMap connInfo;
  //connInfo["User"] = "Default";
  //connInfo["Password"] = "password";
  //connectToCore(connInfo);

  statusBar()->showMessage(tr("Not connected to core."));
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

  //s.beginGroup("Buffers");
  //QString net = s.value("CurrentNetwork", "").toString();
  //QString buf = s.value("CurrentBuffer", "").toString();
  //s.endGroup();

  disconnectedFromCore();  // Disable menus and stuff
  showCoreConnectionDlg(true); // autoconnect if appropriate
  //ui.actionConnectCore->activate(QAction::Trigger);
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
  connect(ui.actionConnectCore, SIGNAL(triggered()), this, SLOT(showCoreConnectionDlg()));
  connect(ui.actionDisconnectCore, SIGNAL(triggered()), Client::instance(), SLOT(disconnectFromCore()));
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
  
  BufferTreeModel *model = Client::bufferModel();
  connect(model, SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));

  addBufferView(tr("All Buffers"), model, BufferViewFilter::AllNets, QStringList());
  //addBufferView(tr("All Channels"), model, BufferViewFilter::AllNets|BufferViewFilter::NoQueries|BufferViewFilter::NoServers, QStringList());
  //addBufferView(tr("All Queries"), model, BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoServers, QStringList());
  //addBufferView(tr("All Networks"), model, BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoQueries, QStringList());
  //addBufferView(tr("Full Custom"), model, BufferViewFilter::FullCustom, QStringList());
  
  ui.menuViews->addSeparator();
}

void MainWin::addBufferView(const QString &viewname, QAbstractItemModel *model, const BufferViewFilter::Modes &mode, const QStringList &nets) {
  QDockWidget *dock = new QDockWidget(viewname, this);
  dock->setObjectName(QString("ViewDock-" + viewname)); // should be unique for mainwindow state!
  dock->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
  //dock->setContentsMargins(4,4,4,4);

  //create the view and initialize it's filter
  BufferView *view = new BufferView(dock);
  view->setFilteredModel(model, mode, nets);
  dock->setWidget(view);
  
  addDockWidget(Qt::LeftDockWidgetArea, dock);
  ui.menuViews->addAction(dock->toggleViewAction());
  
  netViews.append(dock);
}

void MainWin::connectedToCore() {
  foreach(BufferId id, Client::allBufferIds()) {
    emit requestBacklog(id, 100, -1);
  }

  ui.menuViews->setEnabled(true);
  ui.menuCore->setEnabled(true);
  ui.actionDisconnectCore->setEnabled(false); // FIXME
  ui.actionNetworkList->setEnabled(true);
  ui.bufferWidget->show();
}

void MainWin::disconnectedFromCore() {
  ui.menuViews->setEnabled(false);
  ui.menuCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(false);
  ui.actionNetworkList->setEnabled(false);
  ui.bufferWidget->hide();
  ui.actionConnectCore->setEnabled(false); // FIXME
  //qDebug() << "mainwin disconnected";
}

AbstractUiMsg *MainWin::layoutMsg(const Message &msg) {
  return new ChatLine(msg);
}

void MainWin::showCoreConnectionDlg(bool autoConnect) {
  coreConnectDlg = new CoreConnectDlg(this, autoConnect);
  connect(coreConnectDlg, SIGNAL(finished(int)), this, SLOT(coreConnectionDlgFinished(int)));
  coreConnectDlg->setModal(true);
  if(!autoConnect || !coreConnectDlg->willDoInternalAutoConnect())
    coreConnectDlg->show(); // avoid flicker and show dlg only if we do remote connect, which needs a progress bar
  if(autoConnect) coreConnectDlg->doAutoConnect();
}

void MainWin::coreConnectionDlgFinished(int /*code*/) {

  delete coreConnectDlg;
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
