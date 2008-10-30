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
#include "action.h"
#include "actioncollection.h"
#include "buffermodel.h"
#include "bufferviewmanager.h"
#include "bufferwidget.h"
#include "channellistdlg.h"
#include "chatlinemodel.h"
#include "chatmonitorfilter.h"
#include "chatmonitorview.h"
#include "chatview.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "coreinfodlg.h"
#include "coreconnectdlg.h"
#include "debuglogwidget.h"
#include "iconloader.h"
#include "inputwidget.h"
#include "inputline.h"
#include "irclistmodel.h"
#include "jumpkeyhandler.h"
#include "msgprocessorstatuswidget.h"
#include "nicklistwidget.h"
#include "qtuiapplication.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "sessionsettings.h"
#include "settingsdlg.h"
#include "settingspagedlg.h"
#include "topicwidget.h"
#include "verticaldock.h"

#ifdef HAVE_DBUS
#  include "desktopnotificationbackend.h"
#endif
#include "systraynotificationbackend.h"
#include "taskbarnotificationbackend.h"

#include "settingspages/aliasessettingspage.h"
#include "settingspages/appearancesettingspage.h"
#include "settingspages/bufferviewsettingspage.h"
#include "settingspages/colorsettingspage.h"
#include "settingspages/fontssettingspage.h"
#include "settingspages/generalsettingspage.h"
#include "settingspages/highlightsettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/networkssettingspage.h"
#include "settingspages/notificationssettingspage.h"

MainWin::MainWin(QWidget *parent)
  : QMainWindow(parent),
    coreLagLabel(new QLabel()),
    sslLabel(new QLabel()),
    msgProcessorStatusWidget(new MsgProcessorStatusWidget()),
    _titleSetter(this),
    _trayIcon(new QSystemTrayIcon(this))
{
  QtUiSettings uiSettings;
  QString style = uiSettings.value("Style", QString()).toString();
  if(!style.isEmpty()) {
    QApplication::setStyle(style);
  }

  setWindowTitle("Quassel IRC");
  setWindowIconText("Quassel IRC");
  updateIcon();

  installEventFilter(new JumpKeyHandler(this));

  QtUi::registerNotificationBackend(new TaskbarNotificationBackend(this));
  QtUi::registerNotificationBackend(new SystrayNotificationBackend(this));
#ifdef HAVE_DBUS
  QtUi::registerNotificationBackend(new DesktopNotificationBackend(this));
#endif

  QtUiApplication* app = qobject_cast<QtUiApplication*> qApp;
  connect(app, SIGNAL(saveStateToSession(const QString&)), SLOT(saveStateToSession(const QString&)));
  connect(app, SIGNAL(saveStateToSessionSettings(SessionSettings&)), SLOT(saveStateToSessionSettings(SessionSettings&)));
}

void MainWin::init() {
  QtUiSettings s;
  if(s.value("MainWinSize").isValid())
    resize(s.value("MainWinSize").toSize());
  else
    resize(QSize(800, 500));

  connect(QApplication::instance(), SIGNAL(aboutToQuit()), SLOT(saveLayout()));
  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), SLOT(clientNetworkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), SLOT(clientNetworkRemoved(NetworkId)));

  // Setup Dock Areas
  setDockNestingEnabled(true);
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Order is sometimes important
  setupActions();
  setupBufferWidget();
  setupMenus();
  setupViews();
  setupTopicWidget();
  setupChatMonitor();
  setupNickWidget();
  setupInputWidget();
  setupStatusBar();
  setupSystray();
  setupTitleSetter();

  // restore mainwin state
  restoreState(s.value("MainWinState").toByteArray());

  // restore locked state of docks
  QtUi::actionCollection("General")->action("LockDockPositions")->setChecked(s.value("LockDocks", false).toBool());

  setDisconnectedState();  // Disable menus and stuff

  show();

  showCoreConnectionDlg(true); // autoconnect if appropriate
}

MainWin::~MainWin() {
  QtUiSettings s;
  s.setValue("MainWinSize", size());
  s.setValue("MainWinPos", pos());
  s.setValue("MainWinState", saveState());
}

void MainWin::updateIcon() {
  QPixmap icon;
  if(Client::isConnected())
    icon = DesktopIcon("quassel", IconLoader::SizeEnormous);
  else
    icon = DesktopIcon("quassel_disconnected", IconLoader::SizeEnormous);
  setWindowIcon(icon);
  qApp->setWindowIcon(icon);
  systemTrayIcon()->setIcon(icon);
}

void MainWin::setupActions() {
  ActionCollection *coll = QtUi::actionCollection("General");
  // File
  coll->addAction("ConnectCore", new Action(SmallIcon("network-connect"), tr("&Connect to Core..."), coll,
                                             this, SLOT(showCoreConnectionDlg())));
  coll->addAction("DisconnectCore", new Action(SmallIcon("network-disconnect"), tr("&Disconnect from Core"), coll,
                                                Client::instance(), SLOT(disconnectFromCore())));
  coll->addAction("CoreInfo", new Action(SmallIcon("help-about"), tr("Core &Info..."), coll,
                                          this, SLOT(showCoreInfoDlg())));
  coll->addAction("EditNetworks", new Action(SmallIcon("configure"), tr("Edit &Networks..."), coll,
                                              this, SLOT(on_actionEditNetworks_triggered())));
  coll->addAction("Quit", new Action(SmallIcon("application-exit"), tr("&Quit"), coll,
                                      qApp, SLOT(quit()), tr("Ctrl+Q")));

  // View
  coll->addAction("ManageBufferViews", new Action(SmallIcon("view-tree"), tr("&Manage Buffer Views..."), coll,
                                             this, SLOT(on_actionManageViews_triggered())));
  Action *lockAct = coll->addAction("LockDockPositions", new Action(tr("&Lock Dock Positions"), coll));
  lockAct->setCheckable(true);
  connect(lockAct, SIGNAL(toggled(bool)), SLOT(on_actionLockDockPositions_toggled(bool)));

  coll->addAction("ToggleSearchBar", new Action(SmallIcon("edit-find"), tr("Show &Search Bar"), coll,
                                                 0, 0, tr("Ctrl+F")))->setCheckable(true);
  coll->addAction("ToggleStatusBar", new Action(tr("Show Status &Bar"), coll,
                                                 0, 0))->setCheckable(true);

  // Settings
  coll->addAction("ConfigureQuassel", new Action(SmallIcon("configure"), tr("&Configure Quassel..."), coll,
                                                  this, SLOT(showSettingsDlg()), tr("F7")));

  // Help
  coll->addAction("AboutQuassel", new Action(SmallIcon("quassel"), tr("&About Quassel..."), coll,
                                              this, SLOT(showAboutDlg())));
  coll->addAction("AboutQt", new Action(tr("About &Qt..."), coll,
                                         qApp, SLOT(aboutQt())));
  coll->addAction("DebugNetworkModel", new Action(SmallIcon("tools-report-bug"), tr("Debug &NetworkModel"), coll,
                                       this, SLOT(on_actionDebugNetworkModel_triggered())));
  coll->addAction("DebugLog", new Action(SmallIcon("tools-report-bug"), tr("Debug &Log"), coll,
                                       this, SLOT(on_actionDebugLog_triggered())));
}

void MainWin::setupMenus() {
  ActionCollection *coll = QtUi::actionCollection("General");

  _fileMenu = menuBar()->addMenu(tr("&File"));
  _fileMenu->addAction(coll->action("ConnectCore"));
  _fileMenu->addAction(coll->action("DisconnectCore"));
  _fileMenu->addAction(coll->action("CoreInfo"));
  _fileMenu->addSeparator();
  _networksMenu = _fileMenu->addMenu(tr("&Networks"));
  _networksMenu->addAction(coll->action("EditNetworks"));
  _networksMenu->addSeparator();
  _fileMenu->addSeparator();
  _fileMenu->addAction(coll->action("Quit"));

  _viewMenu = menuBar()->addMenu(tr("&View"));
  _bufferViewsMenu = _viewMenu->addMenu(tr("&Buffer Views"));
  _bufferViewsMenu->addAction(coll->action("ManageBufferViews"));
  _viewMenu->addSeparator();
  _viewMenu->addAction(coll->action("ToggleSearchBar"));
  _viewMenu->addAction(coll->action("ToggleStatusBar"));
  _viewMenu->addSeparator();
  _viewMenu->addAction(coll->action("LockDockPositions"));

  _settingsMenu = menuBar()->addMenu(tr("&Settings"));
  _settingsMenu->addAction(coll->action("ConfigureQuassel"));

  _helpMenu = menuBar()->addMenu(tr("&Help"));
  _helpMenu->addAction(coll->action("AboutQuassel"));
  _helpMenu->addAction(coll->action("AboutQt"));
  _helpMenu->addSeparator();
  _helpDebugMenu = _helpMenu->addMenu(SmallIcon("tools-report-bug"), tr("Debug"));
  _helpDebugMenu->addAction(coll->action("DebugNetworkModel"));
  _helpDebugMenu->addAction(coll->action("DebugLog"));
}

void MainWin::setupBufferWidget() {
  _bufferWidget = new BufferWidget(this);
  _bufferWidget->setModel(Client::bufferModel());
  _bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
  setCentralWidget(_bufferWidget);
}

void MainWin::setupViews() {
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
  _bufferViewsMenu->addAction(dock->toggleViewAction());

  _netViews.append(dock);
}

void MainWin::removeBufferView(int bufferViewConfigId) {
  QVariant actionData;
  BufferViewDock *dock;
  foreach(QAction *action, _bufferViewsMenu->actions()) {
    actionData = action->data();
    if(!actionData.isValid())
      continue;

    dock = qobject_cast<BufferViewDock *>(action->parent());
    if(dock && actionData.toInt() == bufferViewConfigId) {
      removeAction(action);
      dock->deleteLater();
    }
  }
}

void MainWin::on_actionEditNetworks_triggered() {
  SettingsPageDlg dlg(new NetworksSettingsPage(this), this);
  dlg.exec();
}

void MainWin::on_actionManageViews_triggered() {
  SettingsPageDlg dlg(new BufferViewSettingsPage(this), this);
  dlg.exec();
}

void MainWin::on_actionLockDockPositions_toggled(bool lock) {
  QList<VerticalDock *> docks = findChildren<VerticalDock *>();
  foreach(VerticalDock *dock, docks) {
    dock->showTitle(!lock);
  }
  QtUiSettings().setValue("LockDocks", lock);
}

void MainWin::setupNickWidget() {
  // create nick dock
  NickListDock *nickDock = new NickListDock(tr("Nicks"), this);
  nickDock->setObjectName("NickDock");
  nickDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  _nickListWidget = new NickListWidget(nickDock);
  nickDock->setWidget(_nickListWidget);

  addDockWidget(Qt::RightDockWidgetArea, nickDock);
  _viewMenu->addAction(nickDock->toggleViewAction());
  nickDock->toggleViewAction()->setText(tr("Show Nick List"));
  nickDock->toggleViewAction()->setIcon(SmallIcon("view-sidetree"));
  // See NickListDock::NickListDock();
  // connect(nickDock->toggleViewAction(), SIGNAL(triggered(bool)), nickListWidget, SLOT(showWidget(bool)));

  // attach the NickListWidget to the BufferModel and the default selection
  _nickListWidget->setModel(Client::bufferModel());
  _nickListWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
}

void MainWin::setupChatMonitor() {
  VerticalDock *dock = new VerticalDock(tr("Chat Monitor"), this);
  dock->setObjectName("ChatMonitorDock");

  ChatMonitorFilter *filter = new ChatMonitorFilter(Client::messageModel(), this);
  ChatMonitorView *chatView = new ChatMonitorView(filter, this);
  chatView->show();
  dock->setWidget(chatView);
  dock->show();

  addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);
  _viewMenu->addAction(dock->toggleViewAction());
  dock->toggleViewAction()->setText(tr("Show Chat Monitor"));
}

void MainWin::setupInputWidget() {
  VerticalDock *dock = new VerticalDock(tr("Inputline"), this);
  dock->setObjectName("InputDock");

  InputWidget *inputWidget = new InputWidget(dock);
  dock->setWidget(inputWidget);

  addDockWidget(Qt::BottomDockWidgetArea, dock);

  _viewMenu->addAction(dock->toggleViewAction());
  dock->toggleViewAction()->setText(tr("Show Input Line"));

  inputWidget->setModel(Client::bufferModel());
  inputWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  _bufferWidget->setFocusProxy(inputWidget);

  inputWidget->inputLine()->installEventFilter(_bufferWidget);
}

void MainWin::setupTopicWidget() {
  VerticalDock *dock = new VerticalDock(tr("Topic"), this);
  dock->setObjectName("TopicDock");
  TopicWidget *topicwidget = new TopicWidget(dock);

  dock->setWidget(topicwidget);

  topicwidget->setModel(Client::bufferModel());
  topicwidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);

  _viewMenu->addAction(dock->toggleViewAction());
  dock->toggleViewAction()->setText(tr("Show Topic Line"));
}

void MainWin::setupTitleSetter() {
  _titleSetter.setModel(Client::bufferModel());
  _titleSetter.setSelectionModel(Client::bufferModel()->standardSelectionModel());
}

void MainWin::setupStatusBar() {
  // MessageProcessor progress
  statusBar()->addPermanentWidget(msgProcessorStatusWidget);
  connect(Client::messageProcessor(), SIGNAL(progressUpdated(int, int)), msgProcessorStatusWidget, SLOT(setProgress(int, int)));

  // Core Lag:
  updateLagIndicator(0);
  statusBar()->addPermanentWidget(coreLagLabel);
  connect(Client::signalProxy(), SIGNAL(lagUpdated(int)), this, SLOT(updateLagIndicator(int)));

  // SSL indicator
  connect(Client::instance(), SIGNAL(securedConnection()), this, SLOT(securedConnection()));
  sslLabel->setPixmap(QPixmap());
  statusBar()->addPermanentWidget(sslLabel);

  _viewMenu->addSeparator();
  QAction *showStatusbar = QtUi::actionCollection("General")->action("ToggleStatusBar");

  QtUiSettings uiSettings;

  bool enabled = uiSettings.value("ShowStatusBar", QVariant(true)).toBool();
  showStatusbar->setChecked(enabled);
  enabled ? statusBar()->show() : statusBar()->hide();

  connect(showStatusbar, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
  connect(showStatusbar, SIGNAL(toggled(bool)), this, SLOT(saveStatusBarStatus(bool)));

  connect(Client::backlogManager(), SIGNAL(messagesRequested(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
  connect(Client::backlogManager(), SIGNAL(messagesProcessed(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
}

void MainWin::saveStatusBarStatus(bool enabled) {
  QtUiSettings uiSettings;
  uiSettings.setValue("ShowStatusBar", enabled);
}

void MainWin::setupSystray() {
  connect(Client::messageModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                                  SLOT(messagesInserted(const QModelIndex &, int, int)));

  ActionCollection *coll = QtUi::actionCollection("General");
  systrayMenu = new QMenu(this);
  systrayMenu->addAction(coll->action("AboutQuassel"));
  systrayMenu->addSeparator();
  systrayMenu->addAction(coll->action("ConnectCore"));
  systrayMenu->addAction(coll->action("DisconnectCore"));
  systrayMenu->addSeparator();
  systrayMenu->addAction(coll->action("Quit"));

  systemTrayIcon()->setContextMenu(systrayMenu);

  QtUiSettings s;
  if(s.value("UseSystemTrayIcon", QVariant(true)).toBool()) {
    systemTrayIcon()->show();
  }

#ifndef Q_WS_MAC
  connect(systemTrayIcon(), SIGNAL(activated( QSystemTrayIcon::ActivationReason )),
          this, SLOT(systrayActivated( QSystemTrayIcon::ActivationReason )));
#endif

}

void MainWin::changeEvent(QEvent *event) {
  if(event->type() == QEvent::WindowStateChange) {
    if(windowState() & Qt::WindowMinimized) {
      QtUiSettings s;
      if(s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnMinimize").toBool()) {
        toggleVisibility();
        event->ignore();
      }
    }
  }
}

void MainWin::connectedToCore() {
  Q_CHECK_PTR(Client::bufferViewManager());
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigAdded(int)), this, SLOT(addBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigDeleted(int)), this, SLOT(removeBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(initDone()), this, SLOT(loadLayout()));

  setConnectedState();
  Client::backlogManager()->requestInitialBacklog();
}

void MainWin::setConnectedState() {
  ActionCollection *coll = QtUi::actionCollection("General");
  //ui.menuCore->setEnabled(true);
  coll->action("ConnectCore")->setEnabled(false);
  coll->action("DisconnectCore")->setEnabled(true);
  coll->action("CoreInfo")->setEnabled(true);
  // _viewMenu->setEnabled(true);
  statusBar()->showMessage(tr("Connected to core."));
  if(sslLabel->width() == 0)
    sslLabel->setPixmap(SmallIcon("security-low"));
  updateIcon();
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

void MainWin::updateLagIndicator(int lag) {
  coreLagLabel->setText(QString(tr("Core Lag: %1 msec")).arg(lag));
}


void MainWin::securedConnection() {
  // todo: make status bar entry
  sslLabel->setPixmap(SmallIcon("security-high"));
}

void MainWin::disconnectedFromCore() {
  // save core specific layout and remove bufferviews;
  saveLayout();
  QVariant actionData;
  BufferViewDock *dock;
  foreach(QAction *action, _bufferViewsMenu->actions()) {
    actionData = action->data();
    if(!actionData.isValid())
      continue;

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
  ActionCollection *coll = QtUi::actionCollection("General");
  //ui.menuCore->setEnabled(false);
  coll->action("ConnectCore")->setEnabled(true);
  coll->action("DisconnectCore")->setEnabled(false);
  coll->action("CoreInfo")->setEnabled(false);
  //_viewMenu->setEnabled(false);
  statusBar()->showMessage(tr("Not connected to core."));
  sslLabel->setPixmap(QPixmap());
  updateIcon();
}

void MainWin::showCoreConnectionDlg(bool autoConnect) {
  CoreConnectDlg(autoConnect, this).exec();
}

void MainWin::showChannelList(NetworkId netId) {
  ChannelListDlg *channelListDlg = new ChannelListDlg();

  if(!netId.isValid()) {
    QAction *action = qobject_cast<QAction *>(sender());
    if(action)
      netId = action->data().value<NetworkId>();
  }

  channelListDlg->setAttribute(Qt::WA_DeleteOnClose);
  channelListDlg->setNetwork(netId);
  channelListDlg->show();
}

void MainWin::showCoreInfoDlg() {
  CoreInfoDlg(this).exec();
}

void MainWin::showSettingsDlg() {
  SettingsDlg *dlg = new SettingsDlg();

  //Category: Appearance
  dlg->registerSettingsPage(new ColorSettingsPage(dlg));
  dlg->registerSettingsPage(new FontsSettingsPage(dlg));
  dlg->registerSettingsPage(new AppearanceSettingsPage(dlg)); //General
  //Category: Behaviour
  dlg->registerSettingsPage(new GeneralSettingsPage(dlg));
  dlg->registerSettingsPage(new HighlightSettingsPage(dlg));
  dlg->registerSettingsPage(new AliasesSettingsPage(dlg));
  dlg->registerSettingsPage(new NotificationsSettingsPage(dlg));
  //Category: General
  dlg->registerSettingsPage(new IdentitiesSettingsPage(dlg));
  dlg->registerSettingsPage(new NetworksSettingsPage(dlg));
  dlg->registerSettingsPage(new BufferViewSettingsPage(dlg));

  dlg->show();
}

void MainWin::showAboutDlg() {
  AboutDlg(this).exec();
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
    if(systemTrayIcon()->isSystemTrayAvailable ()) {
      clearFocus();
      hide();
      if(!systemTrayIcon()->isVisible()) {
        systemTrayIcon()->show();
      }
    } else {
      lower();
    }
  }
}

void MainWin::messagesInserted(const QModelIndex &parent, int start, int end) {
  Q_UNUSED(parent);

  if(QApplication::activeWindow() != 0)
    return;

  for(int i = start; i <= end; i++) {
    QModelIndex idx = Client::messageModel()->index(i, ChatLineModel::ContentsColumn);
    if(!idx.isValid()) {
      qDebug() << "MainWin::messagesInserted(): Invalid model index!";
      continue;
    }
    Message::Flags flags = (Message::Flags)idx.data(ChatLineModel::FlagsRole).toInt();
    if(flags.testFlag(Message::Backlog)) continue;
    flags |= Message::Backlog;  // we only want to trigger a highlight once!
    Client::messageModel()->setData(idx, (int)flags, ChatLineModel::FlagsRole);

    BufferId bufId = idx.data(ChatLineModel::BufferIdRole).value<BufferId>();
    BufferInfo::Type bufType = Client::networkModel()->bufferType(bufId);

    if(flags & Message::Highlight || bufType == BufferInfo::QueryBuffer) {
      QModelIndex senderIdx = Client::messageModel()->index(i, ChatLineModel::SenderColumn);
      QString sender = senderIdx.data(ChatLineModel::EditRole).toString();
      QString contents = idx.data(ChatLineModel::DisplayRole).toString();
      QtUi::invokeNotification(bufId, sender, contents);
    }
  }
}

bool MainWin::event(QEvent *event) {
  if(event->type() == QEvent::WindowActivate)
    QtUi::closeNotifications();
  return QMainWindow::event(event);
}

void MainWin::clientNetworkCreated(NetworkId id) {
  const Network *net = Client::network(id);
  QAction *act = new QAction(net->networkName(), this);
  act->setObjectName(QString("NetworkAction-%1").arg(id.toInt()));
  act->setData(QVariant::fromValue<NetworkId>(id));
  connect(net, SIGNAL(updatedRemotely()), this, SLOT(clientNetworkUpdated()));
  connect(act, SIGNAL(triggered()), this, SLOT(connectOrDisconnectFromNet()));

  QAction *beforeAction = 0;
  foreach(QAction *action, _networksMenu->actions()) {
    if(!action->data().isValid())  // ignore stock actions
      continue;
    if(net->networkName().localeAwareCompare(action->text()) < 0) {
      beforeAction = action;
      break;
    }
  }
  _networksMenu->insertAction(beforeAction, act);
}

void MainWin::clientNetworkUpdated() {
  const Network *net = qobject_cast<const Network *>(sender());
  if(!net)
    return;

  QAction *action = findChild<QAction *>(QString("NetworkAction-%1").arg(net->networkId().toInt()));
  if(!action)
    return;

  action->setText(net->networkName());

  switch(net->connectionState()) {
  case Network::Initialized:
    action->setIcon(SmallIcon("network-connect"));
    break;
  case Network::Disconnected:
    action->setIcon(SmallIcon("network-disconnect"));
    break;
  default:
    action->setIcon(SmallIcon("network-wired"));
  }
}

void MainWin::clientNetworkRemoved(NetworkId id) {
  QAction *action = findChild<QAction *>(QString("NetworkAction-%1").arg(id.toInt()));
  if(!action)
    return;

  action->deleteLater();
}

void MainWin::connectOrDisconnectFromNet() {
  QAction *act = qobject_cast<QAction *>(sender());
  if(!act) return;
  const Network *net = Client::network(act->data().value<NetworkId>());
  if(!net) return;
  if(net->connectionState() == Network::Disconnected) net->requestConnect();
  else net->requestDisconnect();
}

void MainWin::on_actionDebugNetworkModel_triggered() {
  QTreeView *view = new QTreeView;
  view->setAttribute(Qt::WA_DeleteOnClose);
  view->setWindowTitle("Debug NetworkModel View");
  view->setModel(Client::networkModel());
  view->setColumnWidth(0, 250);
  view->setColumnWidth(1, 250);
  view->setColumnWidth(2, 80);
  view->resize(610, 300);
  view->show();
}

void MainWin::on_actionDebugLog_triggered() {
  DebugLogWidget *logWidget = new DebugLogWidget(0);
  logWidget->show();
}

void MainWin::saveStateToSession(const QString &sessionId) {
  return;
  SessionSettings s(sessionId);

  s.setValue("MainWinSize", size());
  s.setValue("MainWinPos", pos());
  s.setValue("MainWinState", saveState());
}

void MainWin::saveStateToSessionSettings(SessionSettings & s)
{
  s.setValue("MainWinSize", size());
  s.setValue("MainWinPos", pos());
  s.setValue("MainWinState", saveState());
}

void MainWin::showStatusBarMessage(const QString &message) {
  statusBar()->showMessage(message, 10000);
}
