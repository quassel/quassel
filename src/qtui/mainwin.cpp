/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifdef HAVE_KDE
#  include <KAction>
#  include <KActionCollection>
#  include <KHelpMenu>
#  include <KMenuBar>
#  include <KShortcutsDialog>
#  include <KStatusBar>
#  include <KToolBar>
#endif

#ifdef Q_WS_X11
#  include <QX11Info>
#endif

#include "aboutdlg.h"
#include "awaylogfilter.h"
#include "awaylogview.h"
#include "action.h"
#include "actioncollection.h"
#include "bufferhotlistfilter.h"
#include "buffermodel.h"
#include "bufferview.h"
#include "bufferviewoverlay.h"
#include "bufferviewoverlayfilter.h"
#include "bufferwidget.h"
#include "channellistdlg.h"
#include "chatlinemodel.h"
#include "chatmonitorfilter.h"
#include "chatmonitorview.h"
#include "chatview.h"
#include "client.h"
#include "clientsyncer.h"
#include "clientbacklogmanager.h"
#include "clientbufferviewconfig.h"
#include "clientbufferviewmanager.h"
#include "coreinfodlg.h"
#include "coreconnectdlg.h"
#include "contextmenuactionprovider.h"
#include "debugbufferviewoverlay.h"
#include "debuglogwidget.h"
#include "debugmessagemodelfilter.h"
#include "flatproxymodel.h"
#include "iconloader.h"
#include "inputwidget.h"
#include "inputline.h"
#include "irclistmodel.h"
#include "ircconnectionwizard.h"
#include "jumpkeyhandler.h"
#include "msgprocessorstatuswidget.h"
#include "nicklistwidget.h"
#include "qtuiapplication.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "settingsdlg.h"
#include "settingspagedlg.h"
#include "systemtray.h"
#include "toolbaractionprovider.h"
#include "topicwidget.h"
#include "verticaldock.h"

#ifndef HAVE_KDE
#  ifdef HAVE_DBUS
#    include "desktopnotificationbackend.h"
#  endif
#  ifdef HAVE_PHONON
#    include "phononnotificationbackend.h"
#  endif
#  include "systraynotificationbackend.h"
#  include "taskbarnotificationbackend.h"
#else /* HAVE_KDE */
#  include "knotificationbackend.h"
#endif /* HAVE_KDE */

#include "settingspages/aliasessettingspage.h"
#include "settingspages/appearancesettingspage.h"
#include "settingspages/backlogsettingspage.h"
#include "settingspages/bufferviewsettingspage.h"
#include "settingspages/chatmonitorsettingspage.h"
#include "settingspages/colorsettingspage.h"
#include "settingspages/connectionsettingspage.h"
#include "settingspages/generalsettingspage.h"
#include "settingspages/highlightsettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/networkssettingspage.h"
#include "settingspages/notificationssettingspage.h"

MainWin::MainWin(QWidget *parent)
#ifdef HAVE_KDE
  : KMainWindow(parent),
  _kHelpMenu(new KHelpMenu(this, KGlobal::mainComponent().aboutData())),
#else
  : QMainWindow(parent),
#endif
    coreLagLabel(new QLabel()),
    sslLabel(new QLabel()),
    msgProcessorStatusWidget(new MsgProcessorStatusWidget()),
    _titleSetter(this),
    _awayLog(0)
{
#ifdef Q_WS_WIN
  dwTickCount = 0;
#endif

  QtUiSettings uiSettings;
  QString style = uiSettings.value("Style", QString()).toString();
  if(!style.isEmpty()) {
    QApplication::setStyle(style);
  }

  QApplication::setQuitOnLastWindowClosed(false);

  setWindowTitle("Quassel IRC");
  setWindowIconText("Quassel IRC");
  updateIcon();

  installEventFilter(new JumpKeyHandler(this));
}

void MainWin::init() {
  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), SLOT(clientNetworkCreated(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), SLOT(clientNetworkRemoved(NetworkId)));
  connect(Client::messageModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
           SLOT(messagesInserted(const QModelIndex &, int, int)));
  connect(GraphicalUi::contextMenuActionProvider(), SIGNAL(showChannelList(NetworkId)), SLOT(showChannelList(NetworkId)));

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
  setupTopicWidget();
  setupChatMonitor();
  setupNickWidget();
  setupInputWidget();
  setupStatusBar();
  setupToolBars();
  setupSystray();
  setupTitleSetter();
  setupHotList();

#ifndef HAVE_KDE
  QtUi::registerNotificationBackend(new TaskbarNotificationBackend(this));
  QtUi::registerNotificationBackend(new SystrayNotificationBackend(this));
#  ifdef HAVE_PHONON
  QtUi::registerNotificationBackend(new PhononNotificationBackend(this));
#  endif
#  ifdef HAVE_DBUS
  QtUi::registerNotificationBackend(new DesktopNotificationBackend(this));
#  endif

#else /* HAVE_KDE */
  QtUi::registerNotificationBackend(new KNotificationBackend(this));
#endif /* HAVE_KDE */

  setDisconnectedState();  // Disable menus and stuff

#ifdef HAVE_KDE
  setAutoSaveSettings();
#endif

  // restore mainwin state
  QtUiSettings s;
  restoreStateFromSettings(s);

  // restore locked state of docks
  QtUi::actionCollection("General")->action("LockLayout")->setChecked(s.value("LockLayout", false).toBool());

  if(Quassel::runMode() != Quassel::Monolithic) {
    showCoreConnectionDlg(true); // autoconnect if appropriate
  } else {
    startInternalCore();
  }
}

MainWin::~MainWin() {

}

void MainWin::quit() {
  QtUiSettings s;
  saveStateToSettings(s);
  saveLayout();
  QApplication::quit();
}

void MainWin::saveStateToSettings(UiSettings &s) {
  s.setValue("MainWinSize", _normalSize);
  s.setValue("MainWinPos", _normalPos);
  s.setValue("MainWinState", saveState());
  s.setValue("MainWinGeometry", saveGeometry());
  s.setValue("MainWinMinimized", isMinimized());
  s.setValue("MainWinMaximized", isMaximized());
  s.setValue("MainWinHidden", !isVisible());

#ifdef HAVE_KDE
  saveAutoSaveSettings();
#endif
}

void MainWin::restoreStateFromSettings(UiSettings &s) {
  _normalSize = s.value("MainWinSize", size()).toSize();
  _normalPos = s.value("MainWinPos", pos()).toPoint();
  bool maximized = s.value("MainWinMaximized", false).toBool();

#ifndef HAVE_KDE
  restoreGeometry(s.value("MainWinGeometry").toByteArray());

  if(maximized) {
    // restoreGeometry() fails if the windows was maximized, so we resize and position explicitly
    resize(_normalSize);
    move(_normalPos);
  }

  restoreState(s.value("MainWinState").toByteArray());

#else
  move(_normalPos);
#endif

  if(s.value("MainWinHidden").toBool())
    hideToTray();
  else if(s.value("MainWinMinimized").toBool())
    showMinimized();
  else if(maximized)
    showMaximized();
  else
    show();
}

void MainWin::updateIcon() {
#ifdef Q_WS_MAC
  const int size = 128;
#else
  const int size = 48;
#endif

  QPixmap icon;
  if(Client::isConnected())
    icon = DesktopIcon("quassel", size);
  else
    icon = DesktopIcon("quassel_inactive", size);
  setWindowIcon(icon);
  qApp->setWindowIcon(icon);
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
  coll->addAction("ConfigureNetworks", new Action(SmallIcon("configure"), tr("Configure &Networks..."), coll,
                                              this, SLOT(on_actionConfigureNetworks_triggered())));
  coll->addAction("Quit", new Action(SmallIcon("application-exit"), tr("&Quit"), coll,
                                      this, SLOT(quit()), tr("Ctrl+Q")));

  // View
  coll->addAction("ConfigureBufferViews", new Action(tr("&Configure Buffer Views..."), coll,
                                             this, SLOT(on_actionConfigureViews_triggered())));

  QAction *lockAct = coll->addAction("LockLayout", new Action(tr("&Lock Layout"), coll));
  lockAct->setCheckable(true);
  connect(lockAct, SIGNAL(toggled(bool)), SLOT(on_actionLockLayout_toggled(bool)));

  coll->addAction("ToggleSearchBar", new Action(SmallIcon("edit-find"), tr("Show &Search Bar"), coll,
						0, 0, QKeySequence::Find))->setCheckable(true);
  coll->addAction("ShowAwayLog", new Action(tr("Show Away Log"), coll,
					    this, SLOT(showAwayLog())));
  coll->addAction("ToggleStatusBar", new Action(tr("Show Status &Bar"), coll,
                                                 0, 0))->setCheckable(true);

  // Settings
  coll->addAction("ConfigureQuassel", new Action(SmallIcon("configure"), tr("&Configure Quassel..."), coll,
                                                  this, SLOT(showSettingsDlg()), tr("F7")));

  // Help
  coll->addAction("AboutQuassel", new Action(SmallIcon("quassel"), tr("&About Quassel"), coll,
                                              this, SLOT(showAboutDlg())));
  coll->addAction("AboutQt", new Action(QIcon(":/pics/qt-logo.png"), tr("About &Qt"), coll,
                                         qApp, SLOT(aboutQt())));
  coll->addAction("DebugNetworkModel", new Action(SmallIcon("tools-report-bug"), tr("Debug &NetworkModel"), coll,
                                       this, SLOT(on_actionDebugNetworkModel_triggered())));
  coll->addAction("DebugBufferViewOverlay", new Action(SmallIcon("tools-report-bug"), tr("Debug &BufferViewOverlay"), coll,
                                       this, SLOT(on_actionDebugBufferViewOverlay_triggered())));
  coll->addAction("DebugMessageModel", new Action(SmallIcon("tools-report-bug"), tr("Debug &MessageModel"), coll,
                                       this, SLOT(on_actionDebugMessageModel_triggered())));
  coll->addAction("DebugHotList", new Action(SmallIcon("tools-report-bug"), tr("Debug &HotList"), coll,
                                       this, SLOT(on_actionDebugHotList_triggered())));
  coll->addAction("DebugLog", new Action(SmallIcon("tools-report-bug"), tr("Debug &Log"), coll,
                                       this, SLOT(on_actionDebugLog_triggered())));

  // Navigation
  coll->addAction("JumpHotBuffer", new Action(tr("Jump to hot buffer"), coll,
                                              this, SLOT(on_jumpHotBuffer_triggered()), QKeySequence(Qt::META + Qt::Key_A)));
}

void MainWin::setupMenus() {
  ActionCollection *coll = QtUi::actionCollection("General");

  _fileMenu = menuBar()->addMenu(tr("&File"));

  static const QStringList coreActions = QStringList()
    << "ConnectCore" << "DisconnectCore" << "CoreInfo";

  QAction *coreAction;
  foreach(QString actionName, coreActions) {
    coreAction = coll->action(actionName);
    _fileMenu->addAction(coreAction);
    flagRemoteCoreOnly(coreAction);
  }
  flagRemoteCoreOnly(_fileMenu->addSeparator());

  _networksMenu = _fileMenu->addMenu(tr("&Networks"));
  _networksMenu->addAction(coll->action("ConfigureNetworks"));
  _networksMenu->addSeparator();
  _fileMenu->addSeparator();
  _fileMenu->addAction(coll->action("Quit"));

  _viewMenu = menuBar()->addMenu(tr("&View"));
  _bufferViewsMenu = _viewMenu->addMenu(tr("&Buffer Views"));
  _bufferViewsMenu->addAction(coll->action("ConfigureBufferViews"));
  _toolbarMenu = _viewMenu->addMenu(tr("&Toolbars"));
  _viewMenu->addSeparator();
  _viewMenu->addAction(coll->action("ToggleSearchBar"));

  coreAction = coll->action("ShowAwayLog");
  flagRemoteCoreOnly(coreAction);
  _viewMenu->addAction(coreAction);

  _viewMenu->addAction(coll->action("ToggleStatusBar"));
  _viewMenu->addSeparator();
  _viewMenu->addAction(coll->action("LockLayout"));

  _settingsMenu = menuBar()->addMenu(tr("&Settings"));
#ifdef HAVE_KDE
  _settingsMenu->addAction(KStandardAction::keyBindings(this, SLOT(showShortcutsDlg()), this));
  _settingsMenu->addAction(KStandardAction::configureNotifications(this, SLOT(showNotificationsDlg()), this));
#endif
  _settingsMenu->addAction(coll->action("ConfigureQuassel"));

  _helpMenu = menuBar()->addMenu(tr("&Help"));
  _helpMenu->addAction(coll->action("AboutQuassel"));
#ifndef HAVE_KDE
  _helpMenu->addAction(coll->action("AboutQt"));
#else
  _helpMenu->addAction(KStandardAction::aboutKDE(_kHelpMenu, SLOT(aboutKDE()), this));
#endif
  _helpMenu->addSeparator();
  _helpDebugMenu = _helpMenu->addMenu(SmallIcon("tools-report-bug"), tr("Debug"));
  _helpDebugMenu->addAction(coll->action("DebugNetworkModel"));
  _helpDebugMenu->addAction(coll->action("DebugBufferViewOverlay"));
  _helpDebugMenu->addAction(coll->action("DebugMessageModel"));
  _helpDebugMenu->addAction(coll->action("DebugHotList"));
  _helpDebugMenu->addAction(coll->action("DebugLog"));
}

void MainWin::setupBufferWidget() {
  _bufferWidget = new BufferWidget(this);
  _bufferWidget->setModel(Client::bufferModel());
  _bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
  setCentralWidget(_bufferWidget);
}

void MainWin::addBufferView(int bufferViewConfigId) {
  addBufferView(Client::bufferViewManager()->clientBufferViewConfig(bufferViewConfigId));
}

void MainWin::addBufferView(ClientBufferViewConfig *config) {
  if(!config)
    return;

  config->setLocked(QtUiSettings().value("LockLayout", false).toBool());
  BufferViewDock *dock = new BufferViewDock(config, this);

  //create the view and initialize it's filter
  BufferView *view = new BufferView(dock);
  view->setFilteredModel(Client::bufferModel(), config);
  view->installEventFilter(_inputWidget->inputLine()); // for key presses
  view->show();

  Client::bufferModel()->synchronizeView(view);

  dock->setWidget(view);
  dock->show();

  addDockWidget(Qt::LeftDockWidgetArea, dock);
  _bufferViewsMenu->addAction(dock->toggleViewAction());

  connect(dock->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(bufferViewToggled(bool)));
  _bufferViews.append(dock);
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

void MainWin::bufferViewToggled(bool enabled) {
  QAction *action = qobject_cast<QAction *>(sender());
  Q_ASSERT(action);
  BufferViewDock *dock = qobject_cast<BufferViewDock *>(action->parent());
  Q_ASSERT(dock);
  if(enabled) {
    Client::bufferViewOverlay()->addView(dock->bufferViewId());
    BufferViewConfig *config = dock->config();
    if(config && config->isInitialized()) {
      BufferIdList buffers;
      if(config->networkId().isValid()) {
        foreach(BufferId bufferId, config->bufferList()) {
          if(Client::networkModel()->networkId(bufferId) == config->networkId())
            buffers << bufferId;
        }
        foreach(BufferId bufferId, config->temporarilyRemovedBuffers().toList()) {
          if(Client::networkModel()->networkId(bufferId) == config->networkId())
            buffers << bufferId;
        }
      } else {
        buffers = BufferIdList::fromSet(config->bufferList().toSet() + config->temporarilyRemovedBuffers());
      }
      Client::backlogManager()->checkForBacklog(buffers);
    }
  } else {
    Client::bufferViewOverlay()->removeView(dock->bufferViewId());
  }
}

BufferView *MainWin::allBuffersView() const {
  // "All Buffers" is always the first dock created
  if(_bufferViews.count() > 0)
    return _bufferViews[0]->bufferView();
  return 0;
}

void MainWin::showNotificationsDlg() {
  SettingsPageDlg dlg(new NotificationsSettingsPage(this), this);
  dlg.exec();
}

void MainWin::on_actionConfigureNetworks_triggered() {
  SettingsPageDlg dlg(new NetworksSettingsPage(this), this);
  dlg.exec();
}

void MainWin::on_actionConfigureViews_triggered() {
  SettingsPageDlg dlg(new BufferViewSettingsPage(this), this);
  dlg.exec();
}

void MainWin::on_actionLockLayout_toggled(bool lock) {
  QList<VerticalDock *> docks = findChildren<VerticalDock *>();
  foreach(VerticalDock *dock, docks) {
    dock->showTitle(!lock);
  }
  if(Client::bufferViewManager()) {
    foreach(ClientBufferViewConfig *config, Client::bufferViewManager()->clientBufferViewConfigs()) {
      config->setLocked(lock);
    }
  }
  QtUiSettings().setValue("LockLayout", lock);
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
  dock->hide();

  addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);
  _viewMenu->addAction(dock->toggleViewAction());
  dock->toggleViewAction()->setText(tr("Show Chat Monitor"));
}

void MainWin::setupInputWidget() {
  VerticalDock *dock = new VerticalDock(tr("Inputline"), this);
  dock->setObjectName("InputDock");

  _inputWidget = new InputWidget(dock);
  dock->setWidget(_inputWidget);

  addDockWidget(Qt::BottomDockWidgetArea, dock);

  _viewMenu->addAction(dock->toggleViewAction());
  dock->toggleViewAction()->setText(tr("Show Input Line"));

  _inputWidget->setModel(Client::bufferModel());
  _inputWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

  _bufferWidget->setFocusProxy(_inputWidget);

  _inputWidget->inputLine()->installEventFilter(_bufferWidget);
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

  // Core Lag:
  updateLagIndicator();
  statusBar()->addPermanentWidget(coreLagLabel);
  coreLagLabel->hide();
  connect(Client::signalProxy(), SIGNAL(lagUpdated(int)), this, SLOT(updateLagIndicator(int)));

  // SSL indicator
  sslLabel->setPixmap(QPixmap());
  statusBar()->addPermanentWidget(sslLabel);
  sslLabel->hide();

  QAction *showStatusbar = QtUi::actionCollection("General")->action("ToggleStatusBar");

  QtUiSettings uiSettings;

  bool enabled = uiSettings.value("ShowStatusBar", QVariant(true)).toBool();
  showStatusbar->setChecked(enabled);
  enabled ? statusBar()->show() : statusBar()->hide();

  connect(showStatusbar, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
  connect(showStatusbar, SIGNAL(toggled(bool)), this, SLOT(saveStatusBarStatus(bool)));
}

void MainWin::setupHotList() {
  FlatProxyModel *flatProxy = new FlatProxyModel(this);
  flatProxy->setSourceModel(Client::bufferModel());
  _bufferHotList = new BufferHotListFilter(flatProxy);
}

void MainWin::saveStatusBarStatus(bool enabled) {
  QtUiSettings uiSettings;
  uiSettings.setValue("ShowStatusBar", enabled);
}

void MainWin::setupSystray() {
  _systemTray = new SystemTray(this);
}

void MainWin::setupToolBars() {
  connect(_bufferWidget, SIGNAL(currentChanged(QModelIndex)),
          QtUi::toolBarActionProvider(), SLOT(currentBufferChanged(QModelIndex)));
  connect(_nickListWidget, SIGNAL(nickSelectionChanged(QModelIndexList)),
          QtUi::toolBarActionProvider(), SLOT(nickSelectionChanged(QModelIndexList)));

#ifdef Q_WS_MAC
  setUnifiedTitleAndToolBarOnMac(true);
#endif

#ifdef HAVE_KDE
  _mainToolBar = new KToolBar("MainToolBar", this, Qt::TopToolBarArea, false, true, true);
#else
  _mainToolBar = new QToolBar(this);
  _mainToolBar->setObjectName("MainToolBar");
#endif
  _mainToolBar->setWindowTitle(tr("Main Toolbar"));
  addToolBar(_mainToolBar);

  QtUi::toolBarActionProvider()->addActions(_mainToolBar, ToolBarActionProvider::MainToolBar);
  _toolbarMenu->addAction(_mainToolBar->toggleViewAction());
}

void MainWin::connectedToCore() {
  Q_CHECK_PTR(Client::bufferViewManager());
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigAdded(int)), this, SLOT(addBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigDeleted(int)), this, SLOT(removeBufferView(int)));
  connect(Client::bufferViewManager(), SIGNAL(initDone()), this, SLOT(loadLayout()));

  setConnectedState();
}

void MainWin::setConnectedState() {
  ActionCollection *coll = QtUi::actionCollection("General");

  coll->action("ConnectCore")->setEnabled(false);
  coll->action("DisconnectCore")->setEnabled(true);
  coll->action("CoreInfo")->setEnabled(true);

  foreach(QAction *action, _fileMenu->actions()) {
    if(isRemoteCoreOnly(action))
      action->setVisible(!Client::internalCore());
  }

  disconnect(Client::backlogManager(), SIGNAL(updateProgress(int, int)), msgProcessorStatusWidget, SLOT(setProgress(int, int)));
  disconnect(Client::backlogManager(), SIGNAL(messagesRequested(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
  disconnect(Client::backlogManager(), SIGNAL(messagesProcessed(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
  if(!Client::internalCore()) {
    connect(Client::backlogManager(), SIGNAL(updateProgress(int, int)), msgProcessorStatusWidget, SLOT(setProgress(int, int)));
    connect(Client::backlogManager(), SIGNAL(messagesRequested(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
    connect(Client::backlogManager(), SIGNAL(messagesProcessed(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
  }

  // _viewMenu->setEnabled(true);
  if(!Client::internalCore())
    statusBar()->showMessage(tr("Connected to core."));
  else
    statusBar()->clearMessage();

  if(Client::signalProxy()->isSecure()) {
    sslLabel->setPixmap(SmallIcon("security-high"));
  } else {
    sslLabel->setPixmap(SmallIcon("security-low"));
  }

  sslLabel->setVisible(!Client::internalCore());
  coreLagLabel->setVisible(!Client::internalCore());
  updateIcon();
  systemTray()->setState(SystemTray::Active);

  if(Client::networkIds().isEmpty()) {
    IrcConnectionWizard *wizard = new IrcConnectionWizard(this, Qt::Sheet);
    wizard->show();
  }
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
  QString text = tr("Core Lag: %1");
  if(lag == -1)
    text = text.arg('-');
  else
    text = text.arg("%1 msec").arg(lag);
  coreLagLabel->setText(text);
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
  sslLabel->hide();
  updateLagIndicator();
  coreLagLabel->hide();
  if(msgProcessorStatusWidget)
    msgProcessorStatusWidget->setProgress(0, 0);
  updateIcon();
  systemTray()->setState(SystemTray::Inactive);
}

void MainWin::startInternalCore() {
  ClientSyncer *syncer = new ClientSyncer();
  Client::registerClientSyncer(syncer);
  connect(syncer, SIGNAL(syncFinished()), syncer, SLOT(deleteLater()), Qt::QueuedConnection);
  syncer->useInternalCore();
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

void MainWin::showAwayLog() {
  if(_awayLog)
    return;
  AwayLogFilter *filter = new AwayLogFilter(Client::messageModel());
  _awayLog = new AwayLogView(filter, 0);
  filter->setParent(_awayLog);
  connect(_awayLog, SIGNAL(destroyed()), this, SLOT(awayLogDestroyed()));
  _awayLog->setAttribute(Qt::WA_DeleteOnClose);
  _awayLog->show();
}

void MainWin::awayLogDestroyed() {
  _awayLog = 0;
}

void MainWin::showSettingsDlg() {
  SettingsDlg *dlg = new SettingsDlg();

  //Category: Appearance
  dlg->registerSettingsPage(new AppearanceSettingsPage(dlg)); //General
  dlg->registerSettingsPage(new ColorSettingsPage(dlg));
  dlg->registerSettingsPage(new HighlightSettingsPage(dlg));
  dlg->registerSettingsPage(new NotificationsSettingsPage(dlg));
  dlg->registerSettingsPage(new BacklogSettingsPage(dlg));
  dlg->registerSettingsPage(new BufferViewSettingsPage(dlg));
  dlg->registerSettingsPage(new ChatMonitorSettingsPage(dlg));

  //Category: Misc
  dlg->registerSettingsPage(new GeneralSettingsPage(dlg));
  dlg->registerSettingsPage(new ConnectionSettingsPage(dlg));
  dlg->registerSettingsPage(new IdentitiesSettingsPage(dlg));
  dlg->registerSettingsPage(new NetworksSettingsPage(dlg));
  dlg->registerSettingsPage(new AliasesSettingsPage(dlg));

  dlg->show();
}

void MainWin::showAboutDlg() {
  AboutDlg(this).exec();
}

#ifdef HAVE_KDE
void MainWin::showShortcutsDlg() {
  KShortcutsDialog::configure(QtUi::actionCollection("General"), KShortcutsEditor::LetterShortcutsDisallowed);
}
#endif

/********************************************************************************************************/

bool MainWin::event(QEvent *event) {
  if(event->type() == QEvent::WindowActivate)
    QtUi::closeNotifications();
  return QMainWindow::event(event);
}

void MainWin::moveEvent(QMoveEvent *event) {
  if(!(windowState() & Qt::WindowMaximized))
    _normalPos = event->pos();

  QMainWindow::moveEvent(event);
}

void MainWin::resizeEvent(QResizeEvent *event) {
  if(!(windowState() & Qt::WindowMaximized))
    _normalSize = event->size();

  QMainWindow::resizeEvent(event);
}

void MainWin::closeEvent(QCloseEvent *event) {
  QtUiSettings s;
  QtUiApplication* app = qobject_cast<QtUiApplication*> qApp;
  Q_ASSERT(app);
  if(!app->isAboutToQuit() && s.value("UseSystemTrayIcon").toBool() && s.value("MinimizeOnClose").toBool()) {
    hideToTray();
    event->ignore();
  } else {
    event->accept();
    quit();
  }
}

void MainWin::changeEvent(QEvent *event) {
#ifdef Q_WS_WIN
  if(event->type() == QEvent::ActivationChange)
    dwTickCount = GetTickCount();  // needed for toggleMinimizedToTray()
#endif

  QMainWindow::changeEvent(event);
}

void MainWin::hideToTray() {
  if(!systemTray()->isSystemTrayAvailable()) {
    qWarning() << Q_FUNC_INFO << "was called with no SystemTray available!";
    return;
  }
  hide();
  systemTray()->setIconVisible();
}

void MainWin::toggleMinimizedToTray() {
#ifdef Q_WS_WIN
  // the problem is that we lose focus when the systray icon is activated
  // and we don't know the former active window
  // therefore we watch for activation event and use our stopwatch :)
  // courtesy: KSystemTrayIcon
  if(GetTickCount() - dwTickCount >= 300)
    // we weren't active in the last 300ms -> activate
    forceActivated();
  else
    hideToTray();

#else

  if(!isVisible() || isMinimized())
    // restore
    forceActivated();
  else
    hideToTray();

#endif
}

void MainWin::forceActivated() {
#ifdef Q_WS_X11
  // Bypass focus stealing prevention
  QX11Info::setAppUserTime(QX11Info::appTime());
#endif

  if(windowState() & Qt::WindowMinimized) {
    // restore
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  }

  // this does not actually work on all platforms... and causes more evil than good
  // move(frameGeometry().topLeft()); // avoid placement policies
  show();
  raise();
  activateWindow();
}

void MainWin::messagesInserted(const QModelIndex &parent, int start, int end) {
  Q_UNUSED(parent);

  bool hasFocus = QApplication::activeWindow() != 0;

  for(int i = start; i <= end; i++) {
    QModelIndex idx = Client::messageModel()->index(i, ChatLineModel::ContentsColumn);
    if(!idx.isValid()) {
      qDebug() << "MainWin::messagesInserted(): Invalid model index!";
      continue;
    }
    Message::Flags flags = (Message::Flags)idx.data(ChatLineModel::FlagsRole).toInt();
    if(flags.testFlag(Message::Backlog) || flags.testFlag(Message::Self))
      continue;
    flags |= Message::Backlog;  // we only want to trigger a highlight once!
    Client::messageModel()->setData(idx, (int)flags, ChatLineModel::FlagsRole);

    BufferId bufId = idx.data(ChatLineModel::BufferIdRole).value<BufferId>();
    BufferInfo::Type bufType = Client::networkModel()->bufferType(bufId);

    if(hasFocus && bufId == _bufferWidget->currentBuffer())
      continue;

    if(flags & Message::Highlight || bufType == BufferInfo::QueryBuffer) {
      QModelIndex senderIdx = Client::messageModel()->index(i, ChatLineModel::SenderColumn);
      QString sender = senderIdx.data(ChatLineModel::EditRole).toString();
      QString contents = idx.data(ChatLineModel::DisplayRole).toString();
      AbstractNotificationBackend::NotificationType type;

      if(bufType == BufferInfo::QueryBuffer && !hasFocus)
        type = AbstractNotificationBackend::PrivMsg;
      else if(bufType == BufferInfo::QueryBuffer && hasFocus)
        type = AbstractNotificationBackend::PrivMsgFocused;
      else if(flags & Message::Highlight && !hasFocus)
        type = AbstractNotificationBackend::Highlight;
      else
        type = AbstractNotificationBackend::HighlightFocused;

      QtUi::invokeNotification(bufId, type, sender, contents);
    }
  }
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

void MainWin::on_jumpHotBuffer_triggered() {
  if(!_bufferHotList->rowCount())
    return;

  QModelIndex topIndex = _bufferHotList->index(0, 0);
  BufferId bufferId = _bufferHotList->data(topIndex, NetworkModel::BufferIdRole).value<BufferId>();
  Client::bufferModel()->switchToBuffer(bufferId);
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

void MainWin::on_actionDebugHotList_triggered() {
  QTreeView *view = new QTreeView;
  view->setAttribute(Qt::WA_DeleteOnClose);
  view->setModel(_bufferHotList);
  view->show();
}

void MainWin::on_actionDebugBufferViewOverlay_triggered() {
  DebugBufferViewOverlay *overlay = new DebugBufferViewOverlay(0);
  overlay->setAttribute(Qt::WA_DeleteOnClose);
  overlay->show();
}

void MainWin::on_actionDebugMessageModel_triggered() {
  QTableView *view = new QTableView(0);
  DebugMessageModelFilter *filter = new DebugMessageModelFilter(view);
  filter->setSourceModel(Client::messageModel());
  view->setModel(filter);
  view->setAttribute(Qt::WA_DeleteOnClose, true);
  view->verticalHeader()->hide();
  view->horizontalHeader()->setStretchLastSection(true);
  view->show();
}

void MainWin::on_actionDebugLog_triggered() {
  DebugLogWidget *logWidget = new DebugLogWidget(0);
  logWidget->show();
}

void MainWin::showStatusBarMessage(const QString &message) {
  statusBar()->showMessage(message, 10000);
}

