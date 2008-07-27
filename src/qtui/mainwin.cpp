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
#include "coreinfodlg.h"
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
#include "util.h"
#include "qtuisettings.h"
#include "jumpkeyhandler.h"

#include "selectionmodelsynchronizer.h"
#include "mappedselectionmodel.h"

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

#include "debugconsole.h"
#include "global.h"
#include "qtuistyle.h"

MainWin::MainWin(QtUi *_gui, QWidget *parent)
  : QMainWindow(parent),
    gui(_gui),
    coreLagLabel(new QLabel()),
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
  UiSettings uiSettings;
  loadTranslation(uiSettings.value("Locale", QLocale::system()).value<QLocale>());
  
  QString style = uiSettings.value("Style", QString("")).toString();
  if(style != "") {
    QApplication::setStyle(style);
  }
  
  ui.setupUi(this);
  setWindowTitle("Quassel IRC");
  setWindowIcon(offlineTrayIcon);
  qApp->setWindowIcon(offlineTrayIcon);
  systray->setIcon(offlineTrayIcon);
  setWindowIconText("Quassel IRC");

  statusBar()->showMessage(tr("Waiting for core..."));

  installEventFilter(new JumpKeyHandler(this));

#ifdef HAVE_DBUS
  desktopNotifications = new org::freedesktop::Notifications(
                            "org.freedesktop.Notifications",
                            "/org/freedesktop/Notifications",
                            QDBusConnection::sessionBus(), this);
  notificationId = 0;
  connect(desktopNotifications, SIGNAL(NotificationClosed(uint, uint)), this, SLOT(desktopNotificationClosed(uint, uint)));
  connect(desktopNotifications, SIGNAL(ActionInvoked(uint, const QString&)), this, SLOT(desktopNotificationInvoked(uint, const QString&)));
#endif
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

  // restore locked state of docks  
  ui.actionLockDockPositions->setChecked(s.value("LockDocks", false).toBool());
  

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
  connect(ui.actionCoreInfo, SIGNAL(triggered()), this, SLOT(showCoreInfoDlg()));
  connect(ui.actionQuit, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(quit()));
  connect(ui.actionSettingsDlg, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
  // connect(ui.actionDebug_Console, SIGNAL(triggered()), this, SLOT(showDebugConsole()));
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

void MainWin::setupSettingsDlg() {
  //Category: Appearance
  settingsDlg->registerSettingsPage(new ColorSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new FontsSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new AppearanceSettingsPage(settingsDlg)); //General
  //Category: Behaviour
  settingsDlg->registerSettingsPage(new GeneralSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new HighlightSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new AliasesSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new NotificationsSettingsPage(settingsDlg));
  //Category: General
  settingsDlg->registerSettingsPage(new IdentitiesSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new NetworksSettingsPage(settingsDlg));
  settingsDlg->registerSettingsPage(new BufferViewSettingsPage(settingsDlg));
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
  //ui.menuCore->setEnabled(true);
  ui.actionConnectCore->setEnabled(false);
  ui.actionDisconnectCore->setEnabled(true);
  ui.actionCoreInfo->setEnabled(true);
  ui.menuViews->setEnabled(true);
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

void MainWin::updateLagIndicator(int lag) {
  coreLagLabel->setText(QString("Core Lag: %1 msec").arg(lag));
}


void MainWin::securedConnection() {
  // todo: make status bar entry
  sslLabel->setPixmap(QPixmap::fromImage(QImage(":/16x16/status/ssl")));
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

void MainWin::showCoreInfoDlg() {
  CoreInfoDlg dlg(this);
  dlg.exec();
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
    bool displayBubble = uiSettings.value("NotificationBubble", QVariant(true)).toBool();
    bool displayDesktop = uiSettings.value("NotificationDesktop", QVariant(true)).toBool();
    if(displayBubble || displayDesktop) {
      // FIXME don't invoke style engine for this!
      QString text = QtUi::style()->styleString(Message::mircToInternal(msg.contents())).plainText;
      if(displayBubble) displayTrayIconMessage(title, text);
#  ifdef HAVE_DBUS
      if(displayDesktop) sendDesktopNotification(title, text);
#  endif
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

#ifdef HAVE_DBUS

/*
Using the notification-daemon from Freedesktop's Galago project
http://www.galago-project.org/specs/notification/0.9/x408.html#command-notify
*/
void MainWin::sendDesktopNotification(const QString &title, const QString &message) {
  QStringList actions;
  QMap<QString, QVariant> hints;
  UiSettings uiSettings;

  hints["x"] = uiSettings.value("NotificationDesktopHintX", QVariant(0)).toInt(); // Standard hint: x location for the popup to show up
  hints["y"] = uiSettings.value("NotificationDesktopHintY", QVariant(0)).toInt(); // Standard hint: y location for the popup to show up

  actions << "click" << "Click Me!";

  QDBusReply<uint> reply = desktopNotifications->Notify(
                "Quassel", // Application name
                notificationId, // ID of previous notification to replace
                "", // Icon to display
                title, // Summary / Header of the message to display
                QString("%1: %2:\n%3").arg(QTime::currentTime().toString()).arg(title).arg(message), // Body of the message to display
                actions, // Actions from which the user may choose
                hints, // Hints to the server displaying the message
                uiSettings.value("NotificationDesktopTimeout", QVariant(5000)).toInt() // Timeout in milliseconds
        );

  if(!reply.isValid()) {
    /* ERROR */
    // could also happen if no notification service runs, so... whatever :)
    //qDebug() << "Error on sending notification..." << reply.error();
    return;
  }

  notificationId = reply.value();

  // qDebug() << "ID: " << notificationId << " Time: " << QTime::currentTime().toString();
}


void MainWin::desktopNotificationClosed(uint id, uint reason) {
  Q_UNUSED(id); Q_UNUSED(reason);
  // qDebug() << "OID: " << notificationId << " ID: " << id << " Reason: " << reason << " Time: " << QTime::currentTime().toString();
  notificationId = 0;
}


void MainWin::desktopNotificationInvoked(uint id, const QString & action) {
  Q_UNUSED(id); Q_UNUSED(action);
  // qDebug() << "OID: " << notificationId << " ID: " << id << " Action: " << action << " Time: " << QTime::currentTime().toString();
}

#endif /* HAVE_DBUS */

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
    action->setIcon(QIcon(":/16x16/actions/network-connect"));
    break;
  case Network::Disconnected:
    action->setIcon(QIcon(":/16x16/actions/network-disconnect"));
    break;
  default:
    action->setIcon(QIcon(":/16x16/actions/gear"));
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
