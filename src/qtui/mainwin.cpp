/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
//#define SPUTDEV

#include "mainwin.h"

#include "chatwidget.h"
#include "bufferview.h"
#include "chatline-old.h"
#include "client.h"
#include "coreconnectdlg.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nicklistwidget.h"
#include "settingsdlg.h"
#include "signalproxy.h"
#include "topicwidget.h"
#include "inputwidget.h"
#include "verticaldock.h"
#include "uisettings.h"

#include "selectionmodelsynchronizer.h"
#include "mappedselectionmodel.h"

#include "settingspages/fontssettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/networkssettingspage.h"

#include "debugconsole.h"

MainWin::MainWin(QtUi *_gui, QWidget *parent) : QMainWindow(parent), gui(_gui) {
  ui.setupUi(this);
  setWindowTitle("Quassel IRC");
  //setWindowTitle(QString::fromUtf8("Κυασελ Εγαρζη"));
  setWindowIcon(QIcon(":icons/quassel-icon.png"));
  setWindowIconText("Quassel IRC");

  statusBar()->showMessage(tr("Waiting for core..."));
  settingsDlg = new SettingsDlg(this);
  debugConsole = new DebugConsole(this);
}

void MainWin::init() {
  UiSettings s;
  resize(s.value("MainWinSize").toSize());

  Client::signalProxy()->attachSignal(this, SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)));
  Client::signalProxy()->attachSignal(this, SIGNAL(disconnectFromNetwork(NetworkId)));
  ui.bufferWidget->init();

  show();

  //QVariantMap connInfo;
  //connInfo["User"] = "Default";
  //connInfo["Password"] = "password";
  //connectToCore(connInfo);

  statusBar()->showMessage(tr("Not connected to core."));

  systray = new QSystemTrayIcon(this);
  systray->setIcon(QIcon(":/icons/quassel-icon.png"));
  
  QString toolTip("left click to minimize the quassel client to tray");
  systray->setToolTip(toolTip);
  
  QMenu *systrayMenu = new QMenu();
  systrayMenu->addAction(ui.actionAboutQuassel);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionConnectCore);
  systrayMenu->addAction(ui.actionDisconnectCore);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionQuit);
  
  systray->setContextMenu(systrayMenu);
  
  systray->show();
  connect(systray, SIGNAL(activated( QSystemTrayIcon::ActivationReason )), 
          this, SLOT(systrayActivated( QSystemTrayIcon::ActivationReason )));

  // DOCK OPTIONS
  setDockNestingEnabled(true);

  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // setup the docks etc...
  setupMenus();
  setupViews();
  setupNickWidget();
  setupChatMonitor();
  setupInputWidget();
  setupTopicWidget();
  
  setupSettingsDlg();

  // restore mainwin state
  restoreState(s.value("MainWinState").toByteArray());

  disconnectedFromCore();  // Disable menus and stuff
  showCoreConnectionDlg(true); // autoconnect if appropriate
  //ui.actionConnectCore->activate(QAction::Trigger);

  // attach the BufferWidget to the PropertyMapper
  ui.bufferWidget->setModel(Client::bufferModel());
  ui.bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  
#ifdef SPUTDEV
  showSettingsDlg();
#endif

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
  //connect(ui.actionNetworkList, SIGNAL(triggered()), this, SLOT(showServerList()));
  connect(ui.actionSettingsDlg, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
  connect(ui.actionDebug_Console, SIGNAL(triggered()), this, SLOT(showDebugConsole()));
  connect(ui.actionDisconnectNet, SIGNAL(triggered()), this, SLOT(disconnectFromNet()));
  connect(ui.actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));

}

void MainWin::setupViews() {
  BufferModel *model = Client::bufferModel();

  addBufferView(tr("All Buffers"), model, BufferViewFilter::AllNets, QList<NetworkId>());
  addBufferView(tr("All Channels"), model, BufferViewFilter::AllNets|BufferViewFilter::NoQueries|BufferViewFilter::NoServers, QList<NetworkId>());
  addBufferView(tr("All Queries"), model, BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoServers, QList<NetworkId>());
  addBufferView(tr("All Networks"), model, BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoQueries, QList<NetworkId>());
  addBufferView(tr("Full Custom"), model, BufferViewFilter::FullCustom, QList<NetworkId>());

  ui.menuViews->addSeparator();
}

QDockWidget *MainWin::addBufferView(const QString &viewname, QAbstractItemModel *model, const BufferViewFilter::Modes &mode, const QList<NetworkId> &nets) {
  QDockWidget *dock = new QDockWidget(viewname, this);
  dock->setObjectName(QString("ViewDock-" + viewname)); // should be unique for mainwindow state!
  dock->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);

  //create the view and initialize it's filter
  BufferView *view = new BufferView(dock);
  view->show();
  view->setFilteredModel(model, mode, nets);
  Client::bufferModel()->synchronizeView(view);
  dock->setWidget(view);
  dock->show();

  addDockWidget(Qt::LeftDockWidgetArea, dock);
  ui.menuViews->addAction(dock->toggleViewAction());

  netViews.append(dock);
  return dock;
}

void MainWin::setupSettingsDlg() {

  settingsDlg->registerSettingsPage(new FontsSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new IdentitiesSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new NetworksSettingsPage(settingsDlg));

#ifdef SPUTDEV
  connect(settingsDlg, SIGNAL(finished(int)), QApplication::instance(), SLOT(quit()));  // FIXME
#endif
}

void MainWin::setupNickWidget() {
  // create nick dock
  nickDock = new QDockWidget(tr("Nicks"), this);
  nickDock->setObjectName("NickDock");
  nickDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  nickListWidget = new NickListWidget(nickDock);
  nickDock->setWidget(nickListWidget);

  addDockWidget(Qt::RightDockWidgetArea, nickDock);
  ui.menuViews->addAction(nickDock->toggleViewAction());

  Client::bufferModel()->mapProperty(0, NetworkModel::BufferIdRole, nickListWidget, "currentBuffer");
}

void MainWin::setupChatMonitor() {
  VerticalDock *dock = new VerticalDock(tr("Chat Monitor"), this);
  dock->setObjectName("ChatMonitorDock");

  ChatWidget *chatWidget = new ChatWidget(this);
  chatWidget->show();
  dock->setWidget(chatWidget);
  dock->show();
  
  Buffer *buf = Client::monitorBuffer();
  if(!buf)
    return;
  
  chatWidget->init(BufferId(0));
  QList<ChatLine *> lines;
  QList<AbstractUiMsg *> msgs = buf->contents();
  foreach(AbstractUiMsg *msg, msgs) {
    lines.append(dynamic_cast<ChatLine*>(msg));
  }
  chatWidget->setContents(lines);
  connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
  connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), chatWidget, SLOT(prependMsg(AbstractUiMsg *)));

  addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);
  ui.menuViews->addAction(dock->toggleViewAction());
}

void MainWin::setupInputWidget() {
  VerticalDock *dock = new VerticalDock(tr("Inputline"), this);
  dock->setObjectName("InputDock");

  InputWidget *inputWidget = new InputWidget(dock);
  dock->setWidget(inputWidget);

  addDockWidget(Qt::BottomDockWidgetArea, dock);

  ui.menuViews->addAction(dock->toggleViewAction());

  inputWidget->setModel(Client::bufferModel());
  inputWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  ui.bufferWidget->setFocusProxy(inputWidget);
}

void MainWin::setupTopicWidget() {
  VerticalDock *dock = new VerticalDock(tr("Topic"), this);
  dock->setObjectName("TopicDock");
  TopicWidget *topicwidget = new TopicWidget(dock);
  dock->setWidget(topicwidget);

  Client::bufferModel()->mapProperty(1, Qt::DisplayRole, topicwidget, "topic");

  addDockWidget(Qt::TopDockWidgetArea, dock);

  ui.menuViews->addAction(dock->toggleViewAction());
}

void MainWin::connectedToCore() {
  foreach(BufferInfo id, Client::allBufferInfos()) {
    emit requestBacklog(id, 1000, -1);
  }

  ui.menuViews->setEnabled(true);
  ui.menuCore->setEnabled(true);
  ui.actionConnectCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(true);
  //ui.actionNetworkList->setEnabled(true);
  ui.bufferWidget->show();
  statusBar()->showMessage(tr("Connected to core."));
}

void MainWin::disconnectedFromCore() {
  ui.menuViews->setEnabled(false);
  ui.menuCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(false);
  //ui.actionNetworkList->setEnabled(false);
  ui.bufferWidget->hide();
  ui.actionConnectCore->setEnabled(true);
  nickListWidget->reset();
  statusBar()->showMessage(tr("Not connected to core."));
}

AbstractUiMsg *MainWin::layoutMsg(const Message &msg) {
  return new ChatLine(msg);
}

void MainWin::showCoreConnectionDlg(bool autoConnect) {
  coreConnectDlg = new CoreConnectDlg(this, autoConnect);
  connect(coreConnectDlg, SIGNAL(finished(int)), this, SLOT(coreConnectionDlgFinished(int)));
  coreConnectDlg->setModal(true);
  //if(!autoConnect || !coreConnectDlg->willDoInternalAutoConnect())
    coreConnectDlg->show(); // avoid flicker and show dlg only if we do remote connect, which needs a progress bar
  //if(autoConnect) coreConnectDlg->doAutoConnect();
}

void MainWin::coreConnectionDlgFinished(int /*code*/) {
  coreConnectDlg->close();
  //exit(1);
}

void MainWin::showSettingsDlg() {
  settingsDlg->show();
}

void MainWin::showDebugConsole() {
  debugConsole->show();
}

void MainWin::closeEvent(QCloseEvent *event)
{
  //if (userReallyWantsToQuit()) {
    UiSettings s;
    s.setValue("MainWinSize", size());
    s.setValue("MainWinPos", pos());
    s.setValue("MainWinState", saveState());
    event->accept();
  //} else {
    //event->ignore();
  //}
}

void MainWin::systrayActivated( QSystemTrayIcon::ActivationReason activationReason) {
  if (activationReason == QSystemTrayIcon::Trigger) {
    if (isHidden())
      show();
    else
      hide();
  }
}

void MainWin::disconnectFromNet() {
  int i = QInputDialog::getInteger(this, tr("Disconnect from Network"), tr("Enter network id:"));
  emit disconnectFromNetwork(NetworkId(i));
}

