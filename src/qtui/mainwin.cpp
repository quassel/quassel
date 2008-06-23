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
#include "mainwin.h"

#include "aboutdlg.h"
#include "chatwidget.h"
#include "bufferview.h"
#include "bufferviewconfig.h"
#include "bufferviewfilter.h"
#include "bufferviewmanager.h"
#include "channellistdlg.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "coreconnectdlg.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nicklistwidget.h"
#include "settingsdlg.h"
#include "settingspagedlg.h"
#include "signalproxy.h"
#include "topicwidget.h"
#include "inputwidget.h"
#include "irclistmodel.h"
#include "verticaldock.h"
#include "uisettings.h"
#include "qtuisettings.h"
#include "jumpkeyhandler.h"

#include "uisettings.h"

#include "selectionmodelsynchronizer.h"
#include "mappedselectionmodel.h"

#include "settingspages/appearancesettingspage.h"
#include "settingspages/bufferviewsettingspage.h"
#include "settingspages/colorsettingspage.h"
#include "settingspages/fontssettingspage.h"
#include "settingspages/generalsettingspage.h"
#include "settingspages/highlightsettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/networkssettingspage.h"


#include "debugconsole.h"
#include "global.h"
#include "qtuistyle.h"


MainWin::MainWin(QtUi *_gui, QWidget *parent)
  : QMainWindow(parent),
    gui(_gui),
    sslLabel(new QLabel()),
    _titleSetter(this),
    systray(new QSystemTrayIcon(this)),
    activeTrayIcon(":/icons/quassel-icon-active.png"),
    onlineTrayIcon(":/icons/quassel-icon.png"),
    offlineTrayIcon(":/icons/quassel-icon-offline.png"),
    trayIconActive(false),
    timer(new QTimer(this)),
    channelListDlg(new ChannelListDlg(this)),
    settingsDlg(new SettingsDlg(this)),
    debugConsole(new DebugConsole(this))
{
  ui.setupUi(this);
  setWindowTitle("Quassel IRC");
  setWindowIcon(offlineTrayIcon);
  qApp->setWindowIcon(offlineTrayIcon);
  systray->setIcon(offlineTrayIcon);
  setWindowIconText("Quassel IRC");

  statusBar()->showMessage(tr("Waiting for core..."));

  installEventFilter(new JumpKeyHandler(this));

  UiSettings uiSettings;
  QString style = uiSettings.value("Style", QString("")).toString();
  if(style != "") {
    QApplication::setStyle(style);
  }
}

void MainWin::init() {
  QtUiSettings s;
  if(s.value("MainWinSize").isValid())
    resize(s.value("MainWinSize").toSize());
  else
    resize(QSize(800, 500));

  Client::signalProxy()->attachSignal(this, SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)));

  connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(saveLayout()));

  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientNetworkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));
  //ui.bufferWidget->init();

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
  setupTopicWidget();
  setupChatMonitor();
  setupInputWidget();
  setupStatusBar();
  setupSystray();

  setupSettingsDlg();

  // restore mainwin state
  restoreState(s.value("MainWinState").toByteArray());

  setDisconnectedState();  // Disable menus and stuff
  showCoreConnectionDlg(true); // autoconnect if appropriate

  // attach the BufferWidget to the BufferModel and the default selection
  ui.bufferWidget->setModel(Client::bufferModel());
  ui.bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  _titleSetter.setModel(Client::bufferModel());
  _titleSetter.setSelectionModel(Client::bufferModel()->standardSelectionModel());
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
  connect(ui.actionManageViews, SIGNAL(triggered()), this, SLOT(showManageViewsDlg()));
}

void MainWin::setupViews() {
  QAction *separator = ui.menuViews->addSeparator();
  separator->setData("__EOBV__");
  addBufferView();
}

void MainWin::addBufferView(int bufferViewConfigId) {
  addBufferView(Client::bufferViewManager()->bufferViewConfig(bufferViewConfigId));
}

void MainWin::addBufferView(BufferViewConfig *config) {
  BufferViewDock *dock;
  if(config)
    dock = new BufferViewDock(config, this);
  else
    dock = new BufferViewDock(this);

  //create the view and initialize it's filter
  BufferView *view = new BufferView(dock);
  view->setFilteredModel(Client::bufferModel(), config);
  view->show();

  connect(&view->showChannelList, SIGNAL(triggered()), this, SLOT(showChannelList()));
  
  Client::bufferModel()->synchronizeView(view);

  dock->setWidget(view);
  dock->show();

  addDockWidget(Qt::LeftDockWidgetArea, dock);

  QAction *endOfBufferViews = 0;
  foreach(QAction *action, ui.menuViews->actions()) {
    if(action->data().toString() == "__EOBV__") {
      endOfBufferViews = action;
      break;
    }
  }
  Q_CHECK_PTR(endOfBufferViews);
  ui.menuViews->insertAction(endOfBufferViews, dock->toggleViewAction());

  _netViews.append(dock);
}

void MainWin::removeBufferView(int bufferViewConfigId) {
  QVariant actionData;
  BufferViewDock *dock;
  foreach(QAction *action, ui.menuViews->actions()) {
    actionData = action->data();
    if(!actionData.isValid())
      continue;
    
    if(actionData.toString() == "__EOBV__")
      break;

    dock = qobject_cast<BufferViewDock *>(action->parent());
    if(dock && actionData.toInt() == bufferViewConfigId) {
      removeAction(action);
      dock->deleteLater();
    }
  }
}

void MainWin::setupSettingsDlg() {
  //Category: Appearance
  settingsDlg->registerSettingsPage(new ColorSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new FontsSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new AppearanceSettingsPage(settingsDlg)); //General
  //Category: Behaviour
  settingsDlg->registerSettingsPage(new GeneralSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new HighlightSettingsPage(settingsDlg));
  //Category: General
  settingsDlg->registerSettingsPage(new IdentitiesSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new NetworksSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new BufferViewSettingsPage(settingsDlg));
}

void MainWin::showNetworkDlg() {
  SettingsPageDlg dlg(new NetworksSettingsPage(this), this);
  dlg.exec();
}

void MainWin::showManageViewsDlg() {
  SettingsPageDlg dlg(new BufferViewSettingsPage(this), this);
  dlg.exec();
}

void MainWin::setupNickWidget() {
  // create nick dock
  NickListDock *nickDock = new NickListDock(tr("Nicks"), this);
  nickDock->setObjectName("NickDock");
  nickDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  nickListWidget = new NickListWidget(nickDock);
  nickDock->setWidget(nickListWidget);

  addDockWidget(Qt::RightDockWidgetArea, nickDock);
  ui.menuViews->addAction(nickDock->toggleViewAction());
  // See NickListDock::NickListDock();
  // connect(nickDock->toggleViewAction(), SIGNAL(triggered(bool)), nickListWidget, SLOT(showWidget(bool)));

  // attach the NickListWidget to the BufferModel and the default selection
  nickListWidget->setModel(Client::bufferModel());
  nickListWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
}

void MainWin::setupChatMonitor() {
#ifndef SPUTDEV
  VerticalDock *dock = new VerticalDock(tr("Chat Monitor"), this);
  dock->setObjectName("ChatMonitorDock");

  ChatWidget *chatWidget = new ChatWidget(0, this);
  chatWidget->show();
  dock->setWidget(chatWidget);
  dock->show();

  Buffer *buf = Client::monitorBuffer();
  if(!buf)
    return;

  chatWidget->setContents(buf->contents());
  connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
  connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), chatWidget, SLOT(prependMsg(AbstractUiMsg *)));

  addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);
  ui.menuViews->addAction(dock->toggleViewAction());
#endif /* SPUTDEV */
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
  connect(topicwidget, SIGNAL(topicChanged(const QString &)), this, SLOT(changeTopic(const QString &)));

  dock->setWidget(topicwidget);

  topicwidget->setModel(Client::bufferModel());
  topicwidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  addDockWidget(Qt::TopDockWidgetArea, dock);

  ui.menuViews->addAction(dock->toggleViewAction());
}

void MainWin::setupStatusBar() {
  connect(Client::instance(), SIGNAL(securedConnection()), this, SLOT(securedConnection()));
  sslLabel->setPixmap(QPixmap());
  statusBar()->addPermanentWidget(sslLabel);

  ui.menuViews->addSeparator();
  QAction *showStatusbar = ui.menuViews->addAction(tr("Statusbar"));
  showStatusbar->setCheckable(true);

  UiSettings uiSettings;

  bool enabled = uiSettings.value("ShowStatusBar", QVariant(true)).toBool();
  showStatusbar->setChecked(enabled);
  enabled ? statusBar()->show() : statusBar()->hide();

  connect(showStatusbar, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
  connect(showStatusbar, SIGNAL(toggled(bool)), this, SLOT(saveStatusBarStatus(bool)));
}

void MainWin::saveStatusBarStatus(bool enabled) {
  UiSettings uiSettings;
  uiSettings.setValue("ShowStatusBar", enabled);
}

void MainWin::setupSystray() {
  connect(timer, SIGNAL(timeout()), this, SLOT(makeTrayIconBlink()));
  connect(Client::instance(), SIGNAL(messageReceived(const Message &)), this, SLOT(receiveMessage(const Message &)));

  systrayMenu = new QMenu(this);
  systrayMenu->addAction(ui.actionAboutQuassel);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionConnectCore);
  systrayMenu->addAction(ui.actionDisconnectCore);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionQuit);

  systray->setContextMenu(systrayMenu);

  UiSettings s;
  if(s.value("UseSystemTrayIcon", QVariant(true)).toBool()) {
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
      UiSettings s;
      if(s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnMinimize").toBool()) {
        toggleVisibility();
        event->ignore();
      }
    }
  }
}

// FIXME this should be made prettier...
void MainWin::changeTopic(const QString &topic) {
  BufferId id = ui.bufferWidget->currentBuffer();
  if(!id.isValid()) return;
  Buffer *buffer = Client::buffer(id);
  if(buffer) Client::userInput(buffer->bufferInfo(), QString("/topic %1").arg(topic));
}

void MainWin::connectedToCore() {
  Q_CHECK_PTR(Client::bufferViewManager());
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigAdded(int)), this, SLOT(addBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigDeleted(int)), this, SLOT(removeBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(initDone()), this, SLOT(loadLayout()));
  
  foreach(BufferInfo id, Client::allBufferInfos()) {
    Client::backlogManager()->requestBacklog(id.bufferId(), 500, -1);
  }
  setConnectedState();
}

void MainWin::setConnectedState() {
  ui.menuViews->setEnabled(true);
  //ui.menuCore->setEnabled(true);
  ui.actionConnectCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(true);
  //ui.actionNetworkList->setEnabled(true);
  ui.bufferWidget->show();
  statusBar()->showMessage(tr("Connected to core."));
  setWindowIcon(onlineTrayIcon);
  qApp->setWindowIcon(onlineTrayIcon);
  systray->setIcon(onlineTrayIcon);
  if(sslLabel->width() == 0)
    sslLabel->setPixmap(QPixmap::fromImage(QImage(":/16x16/status/no-ssl")));
}

void MainWin::loadLayout() {
  QtUiSettings s;
  int accountId = Client::currentCoreAccount().toInt();
  restoreState(s.value(QString("MainWinState-%1").arg(accountId)).toByteArray(), accountId);
}

void MainWin::saveLayout() {
  QtUiSettings s;
  int accountId = Client::currentCoreAccount().toInt();
  if(accountId > 0) s.setValue(QString("MainWinState-%1").arg(accountId) , saveState(accountId));
}

void MainWin::securedConnection() {
  // todo: make status bar entry
  qDebug() << "secured the connection";

  sslLabel->setPixmap(QPixmap::fromImage(QImage(":/16x16/status/ssl")));
}

void MainWin::disconnectedFromCore() {
  // save core specific layout and remove bufferviews;
  saveLayout();
  QVariant actionData;
  BufferViewDock *dock;
  foreach(QAction *action, ui.menuViews->actions()) {
    actionData = action->data();
    if(!actionData.isValid())
      continue;
    
    if(actionData.toString() == "__EOBV__")
      break;

    dock = qobject_cast<BufferViewDock *>(action->parent());
    if(dock && actionData.toInt() != -1) {
      removeAction(action);
      dock->deleteLater();
    }
  }
  QtUiSettings s;
  restoreState(s.value("MainWinState").toByteArray());
  setDisconnectedState();
}

void MainWin::setDisconnectedState() {
  ui.menuViews->setEnabled(false);
  //ui.menuCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(false);
  //ui.actionNetworkList->setEnabled(false);
  ui.bufferWidget->hide();
  ui.actionConnectCore->setEnabled(true);
  // nickListWidget->reset();
  statusBar()->showMessage(tr("Not connected to core."));
  setWindowIcon(offlineTrayIcon);
  qApp->setWindowIcon(offlineTrayIcon);
  systray->setIcon(offlineTrayIcon);
  sslLabel->setPixmap(QPixmap());
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

void MainWin::showChannelList(NetworkId netId) {
  if(!netId.isValid()) {
    QAction *action = qobject_cast<QAction *>(sender());
    if(action)
      netId = action->data().value<NetworkId>();
  }
  channelListDlg->setNetwork(netId);
  channelListDlg->show();
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
  UiSettings s;
  if(s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnClose").toBool()) {
    toggleVisibility();
    event->ignore();
  } else {
    event->accept();
  }
}

void MainWin::systrayActivated( QSystemTrayIcon::ActivationReason activationReason) {
  if(activationReason == QSystemTrayIcon::Trigger) {
    toggleVisibility();
  }
}

void MainWin::toggleVisibility() {
  if(isHidden() /*|| !isActiveWindow()*/) {
    show();
    if(isMinimized()) {
      if(isMaximized())
        showMaximized();
      else
        showNormal();
    }

    raise();
    activateWindow();
    // setFocus(); //Qt::ActiveWindowFocusReason

  } else {
    if(systray->isSystemTrayAvailable ()) {
      clearFocus();
      hide();
      if(!systray->isVisible()) {
        systray->show();
      }
    } else {
      lower();
    }
  }
}

void MainWin::receiveMessage(const Message &msg) {
  if(QApplication::activeWindow() != 0)
    return;

  if(msg.flags() & Message::Highlight || msg.bufferInfo().type() == BufferInfo::QueryBuffer) {
    QString title = msg.bufferInfo().bufferName();;
    if(msg.bufferInfo().type() != BufferInfo::QueryBuffer) {
      QString sender = msg.sender();
      int i = sender.indexOf("!");
      if(i != -1)
        sender = sender.left(i);
      title += QString(" - %1").arg(sender);
    }

    UiSettings uiSettings;

#ifndef SPUTDEV
    if(uiSettings.value("DisplayPopupMessages", QVariant(true)).toBool()) {
      // FIXME don't invoke style engine for this!
      QString text = QtUi::style()->styleString(Message::mircToInternal(msg.contents())).plainText;
      displayTrayIconMessage(title, text);
    }
#endif
    if(uiSettings.value("AnimateTrayIcon", QVariant(true)).toBool()) {
      QApplication::alert(this);
      setTrayIconActivity(true);
    }
  }
}

bool MainWin::event(QEvent *event) {
  if(event->type() == QEvent::WindowActivate)
    setTrayIconActivity(false);
  return QMainWindow::event(event);
}

void MainWin::displayTrayIconMessage(const QString &title, const QString &message) {
  systray->showMessage(title, message);
}

void MainWin::setTrayIconActivity(bool active) {
  if(active) {
    if(!timer->isActive())
      timer->start(500);
  } else {
    timer->stop();
    systray->setIcon(onlineTrayIcon);
  }
}

void MainWin::makeTrayIconBlink() {
  if(trayIconActive) {
    systray->setIcon(onlineTrayIcon);
    trayIconActive = false;
  } else {
    systray->setIcon(activeTrayIcon);
    trayIconActive = true;
  }
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
        //a->setEnabled(true);
      } else if(net->connectionState() == Network::Disconnected) {
        a->setIcon(QIcon(":/16x16/actions/network-disconnect"));
        //a->setEnabled(true);
      } else {
        a->setIcon(QIcon(":/16x16/actions/gear"));
        //a->setEnabled(false);
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

