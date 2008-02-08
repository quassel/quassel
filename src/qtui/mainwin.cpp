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

#include "aboutdlg.h"
#include "chatwidget.h"
#include "bufferview.h"
#include "chatline-old.h"
#include "client.h"
#include "coreconnectdlg.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nicklistwidget.h"
#include "settingsdlg.h"
#include "settingspagedlg.h"
#include "signalproxy.h"
#include "topicwidget.h"
#include "inputwidget.h"
#include "verticaldock.h"
#include "uisettings.h"
#include "qtuisettings.h"

#include "selectionmodelsynchronizer.h"
#include "mappedselectionmodel.h"

#include "settingspages/fontssettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/networkssettingspage.h"
#include "settingspages/generalsettingspage.h"

#include "debugconsole.h"

MainWin::MainWin(QtUi *_gui, QWidget *parent) : QMainWindow(parent), gui(_gui) {
  ui.setupUi(this);
  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":icons/quassel-icon.png"));
  setWindowIconText("Quassel IRC");

  statusBar()->showMessage(tr("Waiting for core..."));
  settingsDlg = new SettingsDlg(this);
  debugConsole = new DebugConsole(this);
}

void MainWin::init() {
  QtUiSettings s;
  resize(s.value("MainWinSize").toSize());

  Client::signalProxy()->attachSignal(this, SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)));

  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientNetworkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));
  ui.bufferWidget->init();
  
  show();

  statusBar()->showMessage(tr("Not connected to core."));

  // DOCK OPTIONS
  setDockNestingEnabled(true);

  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // setup stuff...
  setupMenus();
  setupViews();
  setupNickWidget();
  setupChatMonitor();
  setupInputWidget();
  setupTopicWidget();
  setupSystray();

  
  setupSettingsDlg();

  // restore mainwin state
  restoreState(s.value("MainWinState").toByteArray());

  disconnectedFromCore();  // Disable menus and stuff
  showCoreConnectionDlg(true); // autoconnect if appropriate

  // attach the BufferWidget to the PropertyMapper
  ui.bufferWidget->setModel(Client::bufferModel());
  ui.bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

#ifdef SPUTDEV
  //showSettingsDlg();
  //showAboutDlg();
#endif

}

MainWin::~MainWin() {
  QtUiSettings s;
  s.setValue("MainWinSize", size());
  s.setValue("MainWinPos", pos());
  s.setValue("MainWinState", saveState());
}

void MainWin::setupMenus() {
  connect(ui.actionConnectCore, SIGNAL(triggered()), this, SLOT(showCoreConnectionDlg()));
  connect(ui.actionDisconnectCore, SIGNAL(triggered()), Client::instance(), SLOT(disconnectFromCore()));
  connect(ui.actionQuit, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(quit()));
  //connect(ui.actionNetworkList, SIGNAL(triggered()), this, SLOT(showServerList()));
  connect(ui.actionSettingsDlg, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
  connect(ui.actionDebug_Console, SIGNAL(triggered()), this, SLOT(showDebugConsole()));
  connect(ui.actionAboutQuassel, SIGNAL(triggered()), this, SLOT(showAboutDlg()));
  connect(ui.actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));

  actionEditNetworks = new QAction(QIcon(":/22x22/actions/configure"), tr("Edit &Networks..."), this);
  ui.menuNetworks->addAction(actionEditNetworks);
  connect(actionEditNetworks, SIGNAL(triggered()), this, SLOT(showNetworkDlg()));
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
  settingsDlg->registerSettingsPage(new GeneralSettingsPage(settingsDlg));
  
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

void MainWin::setupSystray() {
  systray = new QSystemTrayIcon(this);
  systray->setIcon(QIcon(":/icons/quassel-icon.png"));

  QString toolTip("left click to minimize the quassel client to tray");
  systray->setToolTip(toolTip);

  systrayMenu = new QMenu(this);
  systrayMenu->addAction(ui.actionAboutQuassel);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionConnectCore);
  systrayMenu->addAction(ui.actionDisconnectCore);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionQuit);

  systray->setContextMenu(systrayMenu);

  QtUiSettings s;
  if(s.value("UseSystemTrayIcon").toBool()) {
    systray->show();
  }
  
  #ifndef Q_WS_MAC
  connect(systray, SIGNAL(activated( QSystemTrayIcon::ActivationReason )),
          this, SLOT(systrayActivated( QSystemTrayIcon::ActivationReason )));
  #endif

}

void MainWin::changeEvent(QEvent *event) {
  if(event->type() == QEvent::WindowStateChange) {
    if(windowState() & Qt::WindowMinimized) {
      QtUiSettings s;
      if(s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnMinimize").toBool()) {
        toggleVisibility();
      }
    }
  }
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
  coreConnectDlg->show();
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

void MainWin::showAboutDlg() {
  AboutDlg dlg(this);
  dlg.exec();
}

void MainWin::closeEvent(QCloseEvent *event) {
  QtUiSettings s;
  if(s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnClose").toBool()) {
    toggleVisibility();
    event->ignore();
  } else {
    event->accept();
  }
}

void MainWin::systrayActivated( QSystemTrayIcon::ActivationReason activationReason) {
  if (activationReason == QSystemTrayIcon::Trigger) {
    toggleVisibility();
  }
}

void MainWin::toggleVisibility() {
  if(isHidden()) {
    show();
    if(isMinimized()) {
      if(isMaximized()) {
        showMaximized();
      } else {
        showNormal();
      }
    }
    raise();
    activateWindow();
  } else {
   hide();
  }
}

void MainWin::showNetworkDlg() {
  SettingsPageDlg dlg(new NetworksSettingsPage(this), this);
  dlg.exec();
}

void MainWin::clientNetworkCreated(NetworkId id) {
  const Network *net = Client::network(id);
  QAction *act = new QAction(net->networkName(), this);
  act->setData(QVariant::fromValue<NetworkId>(id));
  connect(net, SIGNAL(updatedRemotely()), this, SLOT(clientNetworkUpdated()));
  connect(act, SIGNAL(triggered()), this, SLOT(connectOrDisconnectFromNet()));
  bool inserted = false;
  for(int i = 0; i < networkActions.count(); i++) {
    if(net->networkName().localeAwareCompare(networkActions[i]->text()) < 0) {
      networkActions.insert(i, act);
      inserted = true;
      break;
    }
  }
  if(!inserted) networkActions.append(act);
  ui.menuNetworks->clear();  // why the f*** isn't there a QMenu::insertAction()???
  foreach(QAction *a, networkActions) ui.menuNetworks->addAction(a);
  ui.menuNetworks->addSeparator();
  ui.menuNetworks->addAction(actionEditNetworks);
}

void MainWin::clientNetworkUpdated() {
  const Network *net = qobject_cast<const Network *>(sender());
  if(!net) return;
  foreach(QAction *a, networkActions) {
    if(a->data().value<NetworkId>() == net->networkId()) {
      a->setText(net->networkName());
      if(net->connectionState() == Network::Initialized) {
        a->setIcon(QIcon(":/16x16/actions/network-connect"));
        a->setEnabled(true);
      } else if(net->connectionState() == Network::Disconnected) {
        a->setIcon(QIcon(":/16x16/actions/network-disconnect"));
        a->setEnabled(true);
      } else {
        a->setIcon(QIcon(":/16x16/actions/gear"));
        a->setEnabled(false);
      }
      return;
    }
  }
}

void MainWin::clientNetworkRemoved(NetworkId id) {
  QList<QAction *>::iterator actionIter = networkActions.begin();;
  QAction *action;
  while(actionIter != networkActions.end()) {
    action = *actionIter;
    if(action->data().value<NetworkId>() == id) {
      action->deleteLater();
      actionIter = networkActions.erase(actionIter);
    } else
      actionIter++;
  }
}

void MainWin::connectOrDisconnectFromNet() {
  QAction *act = qobject_cast<QAction *>(sender());
  if(!act) return;
  const Network *net = Client::network(act->data().value<NetworkId>());
  if(!net) return;
  if(net->connectionState() == Network::Disconnected) net->requestConnect();
  else net->requestDisconnect();
}

void MainWin::keyPressEvent(QKeyEvent *event) {
#ifdef Q_WS_MAC
  int bindModifier = Qt::ControlModifier | Qt::AltModifier;
  int jumpModifier = Qt::ControlModifier;
#else
  int bindModifier = Qt::ControlModifier;
  int jumpModifier = Qt::AltModifier;
#endif

  if(event->modifiers() ==  bindModifier) {
    event->accept();
    return bindKey(event->key());
  }
  
  if(event->modifiers() == jumpModifier) {
    event->accept();
    return jumpKey(event->key());
  }
  
  event->ignore();
}

void MainWin::bindKey(int key) {
  if(key < Qt::Key_0 || Qt::Key_9 < key)
    return;
  
  QModelIndex bufferIdx = Client::bufferModel()->standardSelectionModel()->currentIndex();
  NetworkId netId = bufferIdx.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  const Network *net = Client::network(netId);
  if(!net)
    return;
  
  QString bufferName = bufferIdx.sibling(bufferIdx.row(), 0).data().toString();
  BufferId bufferId = bufferIdx.data(NetworkModel::BufferIdRole).value<BufferId>();
  
  _keyboardJump[key] = bufferId;
  CoreAccountSettings().setJumpKeyMap(_keyboardJump);
  statusBar()->showMessage(tr("Bound Buffer %1::%2 to Key %3").arg(net->networkName()).arg(bufferName).arg(key - Qt::Key_0), 10000);
}

void MainWin::jumpKey(int key) {
  if(key < Qt::Key_0 || Qt::Key_9 < key)
    return;

  if(_keyboardJump.isEmpty())
    _keyboardJump = CoreAccountSettings().jumpKeyMap();

  if(!_keyboardJump.contains(key))
    return;

  QModelIndex source_bufferIdx = Client::networkModel()->bufferIndex(_keyboardJump[key]);
  QModelIndex bufferIdx = Client::bufferModel()->mapFromSource(source_bufferIdx);

  if(bufferIdx.isValid())
    Client::bufferModel()->standardSelectionModel()->setCurrentIndex(bufferIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  
}
