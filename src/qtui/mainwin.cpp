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
#include "channellistdlg.h"
#include "chatlinemodel.h"
#include "chatmonitorfilter.h"
#include "chatmonitorview.h"
#include "chatview.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "coreinfodlg.h"
#include "coreconnectdlg.h"
#include "iconloader.h"
#include "inputwidget.h"
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
    _trayIcon(new QSystemTrayIcon(this)),
    _actionCollection(new ActionCollection(this))
{
  QtUiSettings uiSettings;
  QString style = uiSettings.value("Style", QString("")).toString();
  if(style != "") {
    QApplication::setStyle(style);
  }
  ui.setupUi(this);
  setWindowTitle("Quassel IRC");
  setWindowIconText("Quassel IRC");
  updateIcon();

  QtUi::actionCollection()->addAssociatedWidget(this);

  statusBar()->showMessage(tr("Waiting for core..."));

  installEventFilter(new JumpKeyHandler(this));

  QtUi::registerNotificationBackend(new TaskbarNotificationBackend(this));
  QtUi::registerNotificationBackend(new SystrayNotificationBackend(this));
#ifdef HAVE_DBUS
  QtUi::registerNotificationBackend(new DesktopNotificationBackend(this));
#endif

  QtUiApplication* app = qobject_cast<QtUiApplication*> qApp;
  connect(app, SIGNAL(saveStateToSession(const QString&)), this, SLOT(saveStateToSession(const QString&)));
  connect(app, SIGNAL(saveStateToSessionSettings(SessionSettings&)), this, SLOT(saveStateToSessionSettings(SessionSettings&)));
}

void MainWin::init() {
  QtUiSettings s;
  if(s.value("MainWinSize").isValid())
    resize(s.value("MainWinSize").toSize());
  else
    resize(QSize(800, 500));

  connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(saveLayout()));

  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientNetworkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));

  show();

  statusBar()->showMessage(tr("Not connected to core."));

  // DOCK OPTIONS
  setDockNestingEnabled(true);

  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // setup stuff...
  setupActions();
  setupMenus();
  setupViews();
  setupNickWidget();
  setupTopicWidget();
  setupChatMonitor();
  setupInputWidget();
  setupStatusBar();
  setupSystray();

  // restore mainwin state
  restoreState(s.value("MainWinState").toByteArray());

  // restore locked state of docks
  ui.actionLockDockPositions->setChecked(s.value("LockDocks", false).toBool());

  setDisconnectedState();  // Disable menus and stuff
  showCoreConnectionDlg(true); // autoconnect if appropriate

  // attach the BufferWidget to the BufferModel and the default selection
  ui.bufferWidget->setModel(Client::bufferModel());
  ui.bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
  ui.menuViews->addAction(QtUi::actionCollection()->action("toggleSearchBar"));

  _titleSetter.setModel(Client::bufferModel());
  _titleSetter.setSelectionModel(Client::bufferModel()->standardSelectionModel());
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
  // TODO don't get these from *.ui anymore... we shouldn't need one
  ui.actionQuit->setIcon(SmallIcon("application-exit"));
  ui.actionSettingsDlg->setIcon(SmallIcon("configure"));
  ui.actionManageViews->setIcon(SmallIcon("view-tree"));
  ui.actionManageViews2->setIcon(SmallIcon("view-tree"));
  ui.actionAboutQt->setIcon(SmallIcon("qt"));
  ui.actionAboutQuassel->setIcon(SmallIcon("quassel"));
  ui.actionConnectCore->setIcon(SmallIcon("network-connect"));
  ui.actionDisconnectCore->setIcon(SmallIcon("network-disconnect"));
  ui.actionCoreInfo->setIcon(SmallIcon("help-about"));
}

void MainWin::setupMenus() {
  connect(ui.actionConnectCore, SIGNAL(triggered()), this, SLOT(showCoreConnectionDlg()));
  connect(ui.actionDisconnectCore, SIGNAL(triggered()), Client::instance(), SLOT(disconnectFromCore()));
  connect(ui.actionCoreInfo, SIGNAL(triggered()), this, SLOT(showCoreInfoDlg()));
  connect(ui.actionQuit, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(quit()));
  connect(ui.actionSettingsDlg, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
  connect(ui.actionAboutQuassel, SIGNAL(triggered()), this, SLOT(showAboutDlg()));
  connect(ui.actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
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
  ui.menuBufferViews->addAction(dock->toggleViewAction());

  _netViews.append(dock);
}

void MainWin::removeBufferView(int bufferViewConfigId) {
  QVariant actionData;
  BufferViewDock *dock;
  foreach(QAction *action, ui.menuBufferViews->actions()) {
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
  VerticalDock *dock = new VerticalDock(tr("Chat Monitor"), this);
  dock->setObjectName("ChatMonitorDock");

  ChatMonitorFilter *filter = new ChatMonitorFilter(Client::messageModel(), this);
  ChatMonitorView *chatView = new ChatMonitorView(filter, this);
  chatView->show();
  dock->setWidget(chatView);
  dock->show();

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

  topicwidget->setModel(Client::bufferModel());
  topicwidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  addDockWidget(Qt::TopDockWidgetArea, dock);

  ui.menuViews->addAction(dock->toggleViewAction());
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
  connect(Client::messageModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                                  SLOT(messagesInserted(const QModelIndex &, int, int)));

  systrayMenu = new QMenu(this);
  systrayMenu->addAction(ui.actionAboutQuassel);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionConnectCore);
  systrayMenu->addAction(ui.actionDisconnectCore);
  systrayMenu->addSeparator();
  systrayMenu->addAction(ui.actionQuit);

  systemTrayIcon()->setContextMenu(systrayMenu);

  UiSettings s;
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
      UiSettings s;
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

  Client::backlogManager()->requestInitialBacklog();
  setConnectedState();
}

void MainWin::setConnectedState() {
  //ui.menuCore->setEnabled(true);
  ui.actionConnectCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(true);
  ui.actionCoreInfo->setEnabled(true);
  ui.menuViews->setEnabled(true);
  ui.bufferWidget->show();
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
  foreach(QAction *action, ui.menuBufferViews->actions()) {
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
  //ui.menuCore->setEnabled(false);
  ui.actionConnectCore->setEnabled(true);
  ui.actionDisconnectCore->setEnabled(false);
  ui.actionCoreInfo->setEnabled(false);
  ui.menuViews->setEnabled(false);
  ui.bufferWidget->hide();
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
  foreach(QAction *action, ui.menuNetworks->actions()) {
    if(action->isSeparator()) {
      beforeAction = action;
      break;
    }
    if(net->networkName().localeAwareCompare(action->text()) < 0) {
      beforeAction = action;
      break;
    }
  }
  Q_CHECK_PTR(beforeAction);
  ui.menuNetworks->insertAction(beforeAction, act);
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

void MainWin::on_actionDebugNetworkModel_triggered(bool) {
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
