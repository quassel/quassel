/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "mainwin.h"

#include <QIcon>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QInputDialog>

#ifdef HAVE_KDE4
#  include <KHelpMenu>
#  include <KMenuBar>
#  include <KShortcutsDialog>
#  include <KStatusBar>
#  include <KToggleFullScreenAction>
#  include <KToolBar>
#endif

#ifdef HAVE_KF5
#  include <KConfigWidgets/KStandardAction>
#  include <KXmlGui/KHelpMenu>
#  include <KXmlGui/KShortcutsDialog>
#  include <KXmlGui/KToolBar>
#  include <KWidgetsAddons/KToggleFullScreenAction>
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
#include "clientbacklogmanager.h"
#include "clientbufferviewconfig.h"
#include "clientbufferviewmanager.h"
#include "clientignorelistmanager.h"
#include "clienttransfer.h"
#include "clienttransfermanager.h"
#include "coreconfigwizard.h"
#include "coreconnectdlg.h"
#include "coreconnection.h"
#include "coreconnectionstatuswidget.h"
#include "coreinfodlg.h"
#include "contextmenuactionprovider.h"
#include "debugbufferviewoverlay.h"
#include "debuglogwidget.h"
#include "debugmessagemodelfilter.h"
#include "extraicon.h"
#include "flatproxymodel.h"
#include "inputwidget.h"
#include "irclistmodel.h"
#include "ircconnectionwizard.h"
#include "legacysystemtray.h"
#include "msgprocessorstatuswidget.h"
#include "nicklistwidget.h"
#include "passwordchangedlg.h"
#include "qtuiapplication.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "receivefiledlg.h"
#include "settingsdlg.h"
#include "settingspagedlg.h"
#include "statusnotifieritem.h"
#include "toolbaractionprovider.h"
#include "topicwidget.h"
#include "transfermodel.h"
#include "verticaldock.h"

#ifndef HAVE_KDE
#  ifdef HAVE_QTMULTIMEDIA
#    include "qtmultimedianotificationbackend.h"
#  endif
#  ifdef HAVE_PHONON
#    include "phononnotificationbackend.h"
#  endif
#  include "systraynotificationbackend.h"
#  include "taskbarnotificationbackend.h"
#else /* HAVE_KDE */
#  include "knotificationbackend.h"
#endif /* HAVE_KDE */


#ifdef HAVE_LIBSNORE
#  include "snorenotificationbackend.h"
#endif

#ifdef HAVE_SSL
#  include "sslinfodlg.h"
#endif

#ifdef HAVE_INDICATEQT
  #include "indicatornotificationbackend.h"
#endif

#ifdef HAVE_NOTIFICATION_CENTER
  #include "osxnotificationbackend.h"
#endif

#ifdef HAVE_DBUS
  #include "dockmanagernotificationbackend.h"
#endif

#include "settingspages/aliasessettingspage.h"
#include "settingspages/appearancesettingspage.h"
#include "settingspages/backlogsettingspage.h"
#include "settingspages/bufferviewsettingspage.h"
#include "settingspages/chatmonitorsettingspage.h"
#include "settingspages/chatviewsettingspage.h"
#include "settingspages/chatviewcolorsettingspage.h"
#include "settingspages/connectionsettingspage.h"
#include "settingspages/coreaccountsettingspage.h"
#include "settingspages/coreconnectionsettingspage.h"
#include <settingspages/corehighlightsettingspage.h>
#include "settingspages/dccsettingspage.h"
#include "settingspages/highlightsettingspage.h"
#include "settingspages/identitiessettingspage.h"
#include "settingspages/ignorelistsettingspage.h"
#include "settingspages/inputwidgetsettingspage.h"
#include "settingspages/itemviewsettingspage.h"
#include "settingspages/networkssettingspage.h"
#include "settingspages/notificationssettingspage.h"
#include "settingspages/topicwidgetsettingspage.h"

#ifndef HAVE_KDE
#  include "settingspages/shortcutssettingspage.h"
#endif

#ifdef HAVE_SONNET
#  include "settingspages/sonnetsettingspage.h"
#endif


MainWin::MainWin(QWidget *parent)
#ifdef HAVE_KDE
    : KMainWindow(parent), _kHelpMenu(new KHelpMenu(this)),
#else
    : QMainWindow(parent),
#endif
    _msgProcessorStatusWidget(new MsgProcessorStatusWidget(this)),
    _coreConnectionStatusWidget(new CoreConnectionStatusWidget(Client::coreConnection(), this)),
    _titleSetter(this),
    _awayLog(0),
    _layoutLoaded(false),
    _activeBufferViewIndex(-1),
    _aboutToQuit(false)
{
    setAttribute(Qt::WA_DeleteOnClose, false); // we delete the mainwin manually

    QtUiSettings uiSettings;
    QString style = uiSettings.value("Style", QString()).toString();
    if (!style.isEmpty()) {
        QApplication::setStyle(style);
    }

    QApplication::setQuitOnLastWindowClosed(false);

    setWindowTitle("Quassel IRC");
    setWindowIconText("Quassel IRC");
    updateIcon();
}


void MainWin::init()
{
    connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), SLOT(clientNetworkCreated(NetworkId)));
    connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), SLOT(clientNetworkRemoved(NetworkId)));
    connect(Client::messageModel(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        SLOT(messagesInserted(const QModelIndex &, int, int)));
    connect(GraphicalUi::contextMenuActionProvider(), SIGNAL(showChannelList(NetworkId)), SLOT(showChannelList(NetworkId)));
    connect(GraphicalUi::contextMenuActionProvider(), SIGNAL(showIgnoreList(QString)), SLOT(showIgnoreList(QString)));
    connect(Client::instance(), SIGNAL(showIgnoreList(QString)), SLOT(showIgnoreList(QString)));

    connect(Client::coreConnection(), SIGNAL(startCoreSetup(QVariantList, QVariantList)), SLOT(showCoreConfigWizard(QVariantList, QVariantList)));
    connect(Client::coreConnection(), SIGNAL(connectionErrorPopup(QString)), SLOT(handleCoreConnectionError(QString)));
    connect(Client::coreConnection(), SIGNAL(userAuthenticationRequired(CoreAccount *, bool *, QString)), SLOT(userAuthenticationRequired(CoreAccount *, bool *, QString)));
    connect(Client::coreConnection(), SIGNAL(handleNoSslInClient(bool *)), SLOT(handleNoSslInClient(bool *)));
    connect(Client::coreConnection(), SIGNAL(handleNoSslInCore(bool *)), SLOT(handleNoSslInCore(bool *)));
#ifdef HAVE_SSL
    connect(Client::coreConnection(), SIGNAL(handleSslErrors(const QSslSocket *, bool *, bool *)), SLOT(handleSslErrors(const QSslSocket *, bool *, bool *)));
#endif

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
    // setupTransferWidget(); not ready yet
    setupChatMonitor();
    setupTopicWidget();
    setupInputWidget();
    setupNickWidget();
    setupViewMenuTail();
    setupStatusBar();
    setupToolBars();
    setupSystray();
    setupTitleSetter();
    setupHotList();

    _bufferWidget->setFocusProxy(_inputWidget);
    _chatMonitorView->setFocusProxy(_inputWidget);

#ifndef HAVE_KDE
#  ifdef HAVE_QTMULTIMEDIA
    QtUi::registerNotificationBackend(new QtMultimediaNotificationBackend(this));
#  endif
#  ifdef HAVE_PHONON
    QtUi::registerNotificationBackend(new PhononNotificationBackend(this));
#  endif
    QtUi::registerNotificationBackend(new TaskbarNotificationBackend(this));
#else /* HAVE_KDE */
    QtUi::registerNotificationBackend(new KNotificationBackend(this));
#endif /* HAVE_KDE */


#ifdef HAVE_LIBSNORE
    QtUi::registerNotificationBackend(new SnoreNotificationBackend(this));
#elif !defined(QT_NO_SYSTEMTRAYICON) && !defined(HAVE_KDE)
    QtUi::registerNotificationBackend(new SystrayNotificationBackend(this));
#endif

#ifdef HAVE_INDICATEQT
    QtUi::registerNotificationBackend(new IndicatorNotificationBackend(this));
#endif

#ifdef HAVE_NOTIFICATION_CENTER
    QtUi::registerNotificationBackend(new OSXNotificationBackend(this));
#endif

#ifdef HAVE_DBUS
    QtUi::registerNotificationBackend(new DockManagerNotificationBackend(this));
#endif

    // we assume that at this point, all configurable actions are defined!
    QtUi::loadShortcuts();

    connect(bufferWidget(), SIGNAL(currentChanged(BufferId)), SLOT(currentBufferChanged(BufferId)));

    setDisconnectedState(); // Disable menus and stuff

#ifdef HAVE_KDE
    setAutoSaveSettings();
#endif

    // restore mainwin state
    QtUiSettings s;
    restoreStateFromSettings(s);

    // restore locked state of docks
    QtUi::actionCollection("General")->action("LockLayout")->setChecked(s.value("LockLayout", false).toBool());

    CoreConnection *conn = Client::coreConnection();
    if (!conn->connectToCore()) {
        // No autoconnect selected (or no accounts)
        showCoreConnectionDlg();
    }
}


MainWin::~MainWin()
{
}


void MainWin::quit()
{
    QtUiSettings s;
    saveStateToSettings(s);
    saveLayout();
    QApplication::quit();
}


void MainWin::saveStateToSettings(UiSettings &s)
{
    s.setValue("MainWinSize", _normalSize);
    s.setValue("MainWinPos", _normalPos);
    s.setValue("MainWinState", saveState());
    s.setValue("MainWinGeometry", saveGeometry());
    s.setValue("MainWinMinimized", isMinimized());
    s.setValue("MainWinMaximized", isMaximized());
    s.setValue("MainWinHidden", !isVisible());
    BufferId lastBufId = Client::bufferModel()->currentBuffer();
    if (lastBufId.isValid())
        s.setValue("LastUsedBufferId", lastBufId.toInt());

#ifdef HAVE_KDE
    saveAutoSaveSettings();
#endif
}


void MainWin::restoreStateFromSettings(UiSettings &s)
{
    _normalSize = s.value("MainWinSize", size()).toSize();
    _normalPos = s.value("MainWinPos", pos()).toPoint();
    bool maximized = s.value("MainWinMaximized", false).toBool();

#ifndef HAVE_KDE
    restoreGeometry(s.value("MainWinGeometry").toByteArray());

    if (maximized) {
        // restoreGeometry() fails if the windows was maximized, so we resize and position explicitly
        resize(_normalSize);
        move(_normalPos);
    }

    restoreState(s.value("MainWinState").toByteArray());

#else
    move(_normalPos);
#endif

    if ((Quassel::isOptionSet("hidewindow")
            || s.value("MainWinHidden").toBool())
            && _systemTray->isSystemTrayAvailable())
        QtUi::hideMainWidget();
    else if (s.value("MainWinMinimized").toBool())
        showMinimized();
    else if (maximized)
        showMaximized();
    else
        show();
}

QMenu *MainWin::createPopupMenu()
{
    QMenu *popupMenu = QMainWindow::createPopupMenu();
    popupMenu->addSeparator();
    ActionCollection *coll = QtUi::actionCollection("General");
    popupMenu->addAction(coll->action("ToggleMenuBar"));
    return popupMenu;
}


void MainWin::updateIcon()
{
    QIcon icon;
    if (Client::isConnected())
        icon = ExtraIcon::load("quassel");
    else
        icon = ExtraIcon::load("inactive-quassel");
    setWindowIcon(icon);
    qApp->setWindowIcon(icon);
}


void MainWin::setupActions()
{
    ActionCollection *coll = QtUi::actionCollection("General", tr("General"));
    // File
    
    coll->addAction("ConnectCore", new Action(ExtraIcon::load("connect-quassel"), tr("&Connect to Core..."), coll,
            this, SLOT(showCoreConnectionDlg())));
    coll->addAction("DisconnectCore", new Action(ExtraIcon::load("disconnect-quassel"), tr("&Disconnect from Core"), coll,
            Client::instance(), SLOT(disconnectFromCore())));
    coll->addAction("ChangePassword", new Action(QIcon::fromTheme("dialog-password"), tr("Change &Password..."), coll,
            this, SLOT(showPasswordChangeDlg())));
    coll->addAction("CoreInfo", new Action(QIcon::fromTheme("help-about"), tr("Core &Info..."), coll,
            this, SLOT(showCoreInfoDlg())));
    coll->addAction("ConfigureNetworks", new Action(QIcon::fromTheme("configure"), tr("Configure &Networks..."), coll,
            this, SLOT(on_actionConfigureNetworks_triggered())));
    // FIXME: use QKeySequence::Quit once we depend on Qt 4.6
    coll->addAction("Quit", new Action(QIcon::fromTheme("application-exit"), tr("&Quit"), coll,
            this, SLOT(quit()), Qt::CTRL + Qt::Key_Q));

    // View
    coll->addAction("ConfigureBufferViews", new Action(tr("&Configure Chat Lists..."), coll,
            this, SLOT(on_actionConfigureViews_triggered())));

    QAction *lockAct = coll->addAction("LockLayout", new Action(tr("&Lock Layout"), coll));
    lockAct->setCheckable(true);
    connect(lockAct, SIGNAL(toggled(bool)), SLOT(on_actionLockLayout_toggled(bool)));

    coll->addAction("ToggleSearchBar", new Action(QIcon::fromTheme("edit-find"), tr("Show &Search Bar"), coll,
            0, 0, QKeySequence::Find))->setCheckable(true);
    coll->addAction("ShowAwayLog", new Action(tr("Show Away Log"), coll,
            this, SLOT(showAwayLog())));
    coll->addAction("ToggleMenuBar", new Action(QIcon::fromTheme("show-menu"), tr("Show &Menubar"), coll,
            0, 0))->setCheckable(true);

    coll->addAction("ToggleStatusBar", new Action(tr("Show Status &Bar"), coll,
            0, 0))->setCheckable(true);

#ifdef HAVE_KDE
    _fullScreenAction = KStandardAction::fullScreen(this, SLOT(onFullScreenToggled()), this, coll);
#else
    _fullScreenAction = new Action(QIcon::fromTheme("view-fullscreen"), tr("&Full Screen Mode"), coll,
        this, SLOT(onFullScreenToggled()), QKeySequence(Qt::Key_F11));
    _fullScreenAction->setCheckable(true);
    coll->addAction("ToggleFullScreen", _fullScreenAction);
#endif

    // Settings
    QAction *configureShortcutsAct = new Action(QIcon::fromTheme("configure-shortcuts"), tr("Configure &Shortcuts..."), coll,
        this, SLOT(showShortcutsDlg()));
    configureShortcutsAct->setMenuRole(QAction::NoRole);
    coll->addAction("ConfigureShortcuts", configureShortcutsAct);

#ifdef Q_OS_MAC
    QAction *configureQuasselAct = new Action(QIcon::fromTheme("configure"), tr("&Configure Quassel..."), coll,
        this, SLOT(showSettingsDlg()));
    configureQuasselAct->setMenuRole(QAction::PreferencesRole);
#else
    QAction *configureQuasselAct = new Action(QIcon::fromTheme("configure"), tr("&Configure Quassel..."), coll,
        this, SLOT(showSettingsDlg()), QKeySequence(Qt::Key_F7));
#endif
    coll->addAction("ConfigureQuassel", configureQuasselAct);

    // Help
    QAction *aboutQuasselAct = new Action(QIcon(":/icons/quassel.png"), tr("&About Quassel"), coll,
        this, SLOT(showAboutDlg()));
    aboutQuasselAct->setMenuRole(QAction::AboutRole);
    coll->addAction("AboutQuassel", aboutQuasselAct);

    QAction *aboutQtAct = new Action(QIcon(":/pics/qt-logo.png"), tr("About &Qt"), coll,
        qApp, SLOT(aboutQt()));
    aboutQtAct->setMenuRole(QAction::AboutQtRole);
    coll->addAction("AboutQt", aboutQtAct);
    coll->addAction("DebugNetworkModel", new Action(QIcon::fromTheme("tools-report-bug"), tr("Debug &NetworkModel"), coll,
            this, SLOT(on_actionDebugNetworkModel_triggered())));
    coll->addAction("DebugBufferViewOverlay", new Action(QIcon::fromTheme("tools-report-bug"), tr("Debug &BufferViewOverlay"), coll,
            this, SLOT(on_actionDebugBufferViewOverlay_triggered())));
    coll->addAction("DebugMessageModel", new Action(QIcon::fromTheme("tools-report-bug"), tr("Debug &MessageModel"), coll,
            this, SLOT(on_actionDebugMessageModel_triggered())));
    coll->addAction("DebugHotList", new Action(QIcon::fromTheme("tools-report-bug"), tr("Debug &HotList"), coll,
            this, SLOT(on_actionDebugHotList_triggered())));
    coll->addAction("DebugLog", new Action(QIcon::fromTheme("tools-report-bug"), tr("Debug &Log"), coll,
            this, SLOT(on_actionDebugLog_triggered())));
    coll->addAction("ReloadStyle", new Action(QIcon::fromTheme("view-refresh"), tr("Reload Stylesheet"), coll,
            QtUi::style(), SLOT(reload()), QKeySequence::Refresh));

    coll->addAction("HideCurrentBuffer", new Action(tr("Hide Current Buffer"), coll,
            this, SLOT(hideCurrentBuffer()), QKeySequence::Close));

    // Navigation
    coll = QtUi::actionCollection("Navigation", tr("Navigation"));

    coll->addAction("JumpHotBuffer", new Action(tr("Jump to hot chat"), coll,
            this, SLOT(on_jumpHotBuffer_triggered()), QKeySequence(Qt::META + Qt::Key_A)));

    coll->addAction("ActivateBufferFilter", new Action(tr("Activate the buffer search"), coll,
            this, SLOT(on_bufferSearch_triggered()), QKeySequence(Qt::CTRL + Qt::Key_S)));

    // Jump keys
#ifdef Q_OS_MAC
    const int bindModifier = Qt::ControlModifier | Qt::AltModifier;
    const int jumpModifier = Qt::ControlModifier;
#else
    const int bindModifier = Qt::ControlModifier;
    const int jumpModifier = Qt::AltModifier;
#endif

    coll->addAction("BindJumpKey0", new Action(tr("Set Quick Access #0"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_0)))->setProperty("Index", 0);
    coll->addAction("BindJumpKey1", new Action(tr("Set Quick Access #1"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_1)))->setProperty("Index", 1);
    coll->addAction("BindJumpKey2", new Action(tr("Set Quick Access #2"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_2)))->setProperty("Index", 2);
    coll->addAction("BindJumpKey3", new Action(tr("Set Quick Access #3"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_3)))->setProperty("Index", 3);
    coll->addAction("BindJumpKey4", new Action(tr("Set Quick Access #4"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_4)))->setProperty("Index", 4);
    coll->addAction("BindJumpKey5", new Action(tr("Set Quick Access #5"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_5)))->setProperty("Index", 5);
    coll->addAction("BindJumpKey6", new Action(tr("Set Quick Access #6"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_6)))->setProperty("Index", 6);
    coll->addAction("BindJumpKey7", new Action(tr("Set Quick Access #7"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_7)))->setProperty("Index", 7);
    coll->addAction("BindJumpKey8", new Action(tr("Set Quick Access #8"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_8)))->setProperty("Index", 8);
    coll->addAction("BindJumpKey9", new Action(tr("Set Quick Access #9"), coll, this, SLOT(bindJumpKey()),
            QKeySequence(bindModifier + Qt::Key_9)))->setProperty("Index", 9);

    coll->addAction("JumpKey0", new Action(tr("Quick Access #0"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_0)))->setProperty("Index", 0);
    coll->addAction("JumpKey1", new Action(tr("Quick Access #1"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_1)))->setProperty("Index", 1);
    coll->addAction("JumpKey2", new Action(tr("Quick Access #2"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_2)))->setProperty("Index", 2);
    coll->addAction("JumpKey3", new Action(tr("Quick Access #3"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_3)))->setProperty("Index", 3);
    coll->addAction("JumpKey4", new Action(tr("Quick Access #4"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_4)))->setProperty("Index", 4);
    coll->addAction("JumpKey5", new Action(tr("Quick Access #5"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_5)))->setProperty("Index", 5);
    coll->addAction("JumpKey6", new Action(tr("Quick Access #6"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_6)))->setProperty("Index", 6);
    coll->addAction("JumpKey7", new Action(tr("Quick Access #7"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_7)))->setProperty("Index", 7);
    coll->addAction("JumpKey8", new Action(tr("Quick Access #8"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_8)))->setProperty("Index", 8);
    coll->addAction("JumpKey9", new Action(tr("Quick Access #9"), coll, this, SLOT(onJumpKey()),
            QKeySequence(jumpModifier + Qt::Key_9)))->setProperty("Index", 9);

    // Buffer navigation
    coll->addAction("NextBufferView", new Action(QIcon::fromTheme("go-next-view"), tr("Activate Next Chat List"), coll,
            this, SLOT(nextBufferView()), QKeySequence(QKeySequence::Forward)));
    coll->addAction("PreviousBufferView", new Action(QIcon::fromTheme("go-previous-view"), tr("Activate Previous Chat List"), coll,
            this, SLOT(previousBufferView()), QKeySequence::Back));
    coll->addAction("NextBuffer", new Action(QIcon::fromTheme("go-down"), tr("Go to Next Chat"), coll,
            this, SLOT(nextBuffer()), QKeySequence(Qt::ALT + Qt::Key_Down)));
    coll->addAction("PreviousBuffer", new Action(QIcon::fromTheme("go-up"), tr("Go to Previous Chat"), coll,
            this, SLOT(previousBuffer()), QKeySequence(Qt::ALT + Qt::Key_Up)));
}


void MainWin::setupMenus()
{
    ActionCollection *coll = QtUi::actionCollection("General");

    _fileMenu = menuBar()->addMenu(tr("&File"));

    static const QStringList coreActions = QStringList()
        << "ConnectCore" << "DisconnectCore" << "ChangePassword" << "CoreInfo";

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
    _bufferViewsMenu = _viewMenu->addMenu(tr("&Chat Lists"));
    _bufferViewsMenu->addAction(coll->action("ConfigureBufferViews"));
    _toolbarMenu = _viewMenu->addMenu(tr("&Toolbars"));
    _viewMenu->addSeparator();

    _viewMenu->addAction(coll->action("ToggleMenuBar"));
    _viewMenu->addAction(coll->action("ToggleStatusBar"));
    _viewMenu->addAction(coll->action("ToggleSearchBar"));

    coreAction = coll->action("ShowAwayLog");
    flagRemoteCoreOnly(coreAction);
    _viewMenu->addAction(coreAction);

    _viewMenu->addSeparator();
    _viewMenu->addAction(coll->action("LockLayout"));

    _settingsMenu = menuBar()->addMenu(tr("&Settings"));
#ifdef HAVE_KDE
    _settingsMenu->addAction(KStandardAction::configureNotifications(this, SLOT(showNotificationsDlg()), this));
    _settingsMenu->addAction(KStandardAction::keyBindings(this, SLOT(showShortcutsDlg()), this));
#else
    _settingsMenu->addAction(coll->action("ConfigureShortcuts"));
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
    _helpDebugMenu = _helpMenu->addMenu(QIcon::fromTheme("tools-report-bug"), tr("Debug"));
    _helpDebugMenu->addAction(coll->action("DebugNetworkModel"));
    _helpDebugMenu->addAction(coll->action("DebugBufferViewOverlay"));
    _helpDebugMenu->addAction(coll->action("DebugMessageModel"));
    _helpDebugMenu->addAction(coll->action("DebugHotList"));
    _helpDebugMenu->addAction(coll->action("DebugLog"));
    _helpDebugMenu->addSeparator();
    _helpDebugMenu->addAction(coll->action("ReloadStyle"));

    // Toggle visibility
    QAction *showMenuBar = QtUi::actionCollection("General")->action("ToggleMenuBar");

    QtUiSettings uiSettings;
    bool enabled = uiSettings.value("ShowMenuBar", QVariant(true)).toBool();
    showMenuBar->setChecked(enabled);
    enabled ? menuBar()->show() : menuBar()->hide();

    connect(showMenuBar, SIGNAL(toggled(bool)), menuBar(), SLOT(setVisible(bool)));
    connect(showMenuBar, SIGNAL(toggled(bool)), this, SLOT(saveMenuBarStatus(bool)));
}


void MainWin::setupBufferWidget()
{
    _bufferWidget = new BufferWidget(this);
    _bufferWidget->setModel(Client::bufferModel());
    _bufferWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
    setCentralWidget(_bufferWidget);
}


void MainWin::addBufferView(int bufferViewConfigId)
{
    addBufferView(Client::bufferViewManager()->clientBufferViewConfig(bufferViewConfigId));
}


void MainWin::addBufferView(ClientBufferViewConfig *config)
{
    if (!config)
        return;

    config->setLocked(QtUiSettings().value("LockLayout", false).toBool());
    BufferViewDock *dock = new BufferViewDock(config, this);

    //create the view and initialize it's filter
    BufferView *view = new BufferView(dock);
    view->setFilteredModel(Client::bufferModel(), config);
    view->installEventFilter(_inputWidget); // for key presses

    Client::bufferModel()->synchronizeView(view);

    dock->setWidget(view);
    dock->setVisible(_layoutLoaded); // don't show before state has been restored

    addDockWidget(Qt::LeftDockWidgetArea, dock);
    _bufferViewsMenu->addAction(dock->toggleViewAction());

    connect(dock->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(bufferViewToggled(bool)));
    connect(dock, SIGNAL(visibilityChanged(bool)), SLOT(bufferViewVisibilityChanged(bool)));
    _bufferViews.append(dock);

    if (!activeBufferView())
        nextBufferView();
}


void MainWin::removeBufferView(int bufferViewConfigId)
{
    QVariant actionData;
    BufferViewDock *dock;
    foreach(QAction *action, _bufferViewsMenu->actions()) {
        actionData = action->data();
        if (!actionData.isValid())
            continue;

        dock = qobject_cast<BufferViewDock *>(action->parent());
        if (dock && actionData.toInt() == bufferViewConfigId) {
            removeAction(action);
            Client::bufferViewOverlay()->removeView(dock->bufferViewId());
            _bufferViews.removeAll(dock);

            if (dock->isActive()) {
                dock->setActive(false);
                _activeBufferViewIndex = -1;
                nextBufferView();
            }

            dock->deleteLater();
        }
    }
}


void MainWin::bufferViewToggled(bool enabled)
{
    if (!enabled && !isMinimized()) {
        // hiding the mainwindow triggers a toggle of the bufferview (which pretty much sucks big time)
        // since this isn't our fault and we can't do anything about it, we suppress the resulting calls
        return;
    }
    QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action);
    BufferViewDock *dock = qobject_cast<BufferViewDock *>(action->parent());
    Q_ASSERT(dock);

    // Make sure we don't toggle backlog fetch for a view we've already removed
    if (!_bufferViews.contains(dock))
        return;

    if (enabled)
        Client::bufferViewOverlay()->addView(dock->bufferViewId());
    else
        Client::bufferViewOverlay()->removeView(dock->bufferViewId());
}


void MainWin::bufferViewVisibilityChanged(bool visible)
{
    Q_UNUSED(visible);
    BufferViewDock *dock = qobject_cast<BufferViewDock *>(sender());
    Q_ASSERT(dock);
    if ((!dock->isHidden() && !activeBufferView()) || (dock->isHidden() && dock->isActive()))
        nextBufferView();
}


BufferView *MainWin::allBuffersView() const
{
    // "All Buffers" is always the first dock created
    if (_bufferViews.count() > 0)
        return _bufferViews[0]->bufferView();
    return 0;
}


BufferView *MainWin::activeBufferView() const
{
    if (_activeBufferViewIndex < 0 || _activeBufferViewIndex >= _bufferViews.count())
        return 0;
    BufferViewDock *dock = _bufferViews.at(_activeBufferViewIndex);
    return dock->isActive() ? dock->bufferView() : 0;
}


void MainWin::changeActiveBufferView(int bufferViewId)
{
    if (bufferViewId < 0)
        return;

    if (_activeBufferViewIndex >= 0 && _activeBufferViewIndex < _bufferViews.count()) {
        _bufferViews[_activeBufferViewIndex]->setActive(false);
        _activeBufferViewIndex = -1;
    }

    for (int i = 0; i < _bufferViews.count(); i++) {
        BufferViewDock *dock = _bufferViews.at(i);
        if (dock->bufferViewId() == bufferViewId && !dock->isHidden()) {
            _activeBufferViewIndex = i;
            dock->setActive(true);
            return;
        }
    }

    nextBufferView(); // fallback
}


void MainWin::showPasswordChangeDlg()
{
    if((Client::coreFeatures() & Quassel::PasswordChange)) {
        PasswordChangeDlg dlg(this);
        dlg.exec();
    }
    else {
        QMessageBox box(QMessageBox::Warning, tr("Feature Not Supported"),
                        tr("<b>Your Quassel Core does not support this feature</b>"),
                        QMessageBox::Ok, this);
        box.setInformativeText(tr("You need a Quassel Core v0.12.0 or newer in order to be able to remotely change your password."));
        box.exec();
    }
}


void MainWin::changeActiveBufferView(bool backwards)
{
    if (_activeBufferViewIndex >= 0 && _activeBufferViewIndex < _bufferViews.count()) {
        _bufferViews[_activeBufferViewIndex]->setActive(false);
    }

    if (!_bufferViews.count())
        return;

    int c = _bufferViews.count();
    while (c--) { // yes, this will reactivate the current active one if all others fail
        if (backwards) {
            if (--_activeBufferViewIndex < 0)
                _activeBufferViewIndex = _bufferViews.count()-1;
        }
        else {
            if (++_activeBufferViewIndex >= _bufferViews.count())
                _activeBufferViewIndex = 0;
        }

        BufferViewDock *dock = _bufferViews.at(_activeBufferViewIndex);
        if (dock->isHidden())
            continue;

        dock->setActive(true);
        return;
    }

    _activeBufferViewIndex = -1;
}


void MainWin::nextBufferView()
{
    changeActiveBufferView(false);
}


void MainWin::previousBufferView()
{
    changeActiveBufferView(true);
}


void MainWin::nextBuffer()
{
    BufferView *view = activeBufferView();
    if (view)
        view->nextBuffer();
}


void MainWin::previousBuffer()
{
    BufferView *view = activeBufferView();
    if (view)
        view->previousBuffer();
}


void MainWin::hideCurrentBuffer()
{
    BufferView *view = activeBufferView();
    if (view)
        view->hideCurrentBuffer();
}


void MainWin::showNotificationsDlg()
{
    SettingsPageDlg dlg(new NotificationsSettingsPage(this), this);
    dlg.exec();
}


void MainWin::on_actionConfigureNetworks_triggered()
{
    SettingsPageDlg dlg(new NetworksSettingsPage(this), this);
    dlg.exec();
}


void MainWin::on_actionConfigureViews_triggered()
{
    SettingsPageDlg dlg(new BufferViewSettingsPage(this), this);
    dlg.exec();
}


void MainWin::on_actionLockLayout_toggled(bool lock)
{
    QList<VerticalDock *> docks = findChildren<VerticalDock *>();
    foreach(VerticalDock *dock, docks) {
        dock->showTitle(!lock);
    }
    if (Client::bufferViewManager()) {
        foreach(ClientBufferViewConfig *config, Client::bufferViewManager()->clientBufferViewConfigs()) {
            config->setLocked(lock);
        }
    }
    QtUiSettings().setValue("LockLayout", lock);
}


void MainWin::setupNickWidget()
{
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

    _nickListWidget->setVisible(false);
}


void MainWin::setupChatMonitor()
{
    VerticalDock *dock = new VerticalDock(tr("Chat Monitor"), this);
    dock->setObjectName("ChatMonitorDock");

    ChatMonitorFilter *filter = new ChatMonitorFilter(Client::messageModel(), this);
    _chatMonitorView = new ChatMonitorView(filter, this);
    _chatMonitorView->show();
    dock->setWidget(_chatMonitorView);
    dock->hide();

    addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);
    _viewMenu->addAction(dock->toggleViewAction());
    dock->toggleViewAction()->setText(tr("Show Chat Monitor"));
}


void MainWin::setupInputWidget()
{
    VerticalDock *dock = new VerticalDock(tr("Inputline"), this);
    dock->setObjectName("InputDock");

    _inputWidget = new InputWidget(dock);
    dock->setWidget(_inputWidget);

    addDockWidget(Qt::BottomDockWidgetArea, dock);

    _viewMenu->addAction(dock->toggleViewAction());
    dock->toggleViewAction()->setText(tr("Show Input Line"));

    _inputWidget->setModel(Client::bufferModel());
    _inputWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

    _inputWidget->inputLine()->installEventFilter(_bufferWidget);

    connect(_topicWidget, SIGNAL(switchedPlain()), _bufferWidget, SLOT(setFocus()));
}


void MainWin::setupTopicWidget()
{
    VerticalDock *dock = new VerticalDock(tr("Topic"), this);
    dock->setObjectName("TopicDock");
    _topicWidget = new TopicWidget(dock);

    dock->setWidget(_topicWidget);

    _topicWidget->setModel(Client::bufferModel());
    _topicWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());

    addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);

    _viewMenu->addAction(dock->toggleViewAction());
    dock->toggleViewAction()->setText(tr("Show Topic Line"));
}


void MainWin::setupTransferWidget()
{
    auto dock = new QDockWidget(tr("Transfers"), this);
    dock->setObjectName("TransferDock");
    dock->setAllowedAreas(Qt::TopDockWidgetArea|Qt::BottomDockWidgetArea);

    auto view = new QTableView(dock); // to be replaced by the real thing
    view->setModel(Client::transferModel());
    dock->setWidget(view);
    dock->hide(); // hidden by default
    addDockWidget(Qt::TopDockWidgetArea, dock, Qt::Vertical);

    auto action = dock->toggleViewAction();
    action->setText(tr("Show File Transfers"));
    action->setIcon(QIcon::fromTheme("download"));
    action->setShortcut(QKeySequence(Qt::Key_F6));
    QtUi::actionCollection("General")->addAction("ShowTransferWidget", action);
    _viewMenu->addAction(action);
}


void MainWin::setupViewMenuTail()
{
    _viewMenu->addSeparator();
    _viewMenu->addAction(_fullScreenAction);
}


void MainWin::setupTitleSetter()
{
    _titleSetter.setModel(Client::bufferModel());
    _titleSetter.setSelectionModel(Client::bufferModel()->standardSelectionModel());
}


void MainWin::setupStatusBar()
{
    // MessageProcessor progress
    statusBar()->addPermanentWidget(_msgProcessorStatusWidget);

    // Connection state
    _coreConnectionStatusWidget->update();
    statusBar()->addPermanentWidget(_coreConnectionStatusWidget);

    QAction *showStatusbar = QtUi::actionCollection("General")->action("ToggleStatusBar");

    QtUiSettings uiSettings;

    bool enabled = uiSettings.value("ShowStatusBar", QVariant(true)).toBool();
    showStatusbar->setChecked(enabled);
    enabled ? statusBar()->show() : statusBar()->hide();

    connect(showStatusbar, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
    connect(showStatusbar, SIGNAL(toggled(bool)), this, SLOT(saveStatusBarStatus(bool)));

    connect(Client::coreConnection(), SIGNAL(connectionMsg(QString)), statusBar(), SLOT(showMessage(QString)));
}


void MainWin::setupHotList()
{
    FlatProxyModel *flatProxy = new FlatProxyModel(this);
    flatProxy->setSourceModel(Client::bufferModel());
    _bufferHotList = new BufferHotListFilter(flatProxy);
}


void MainWin::saveMenuBarStatus(bool enabled)
{
    QtUiSettings uiSettings;
    uiSettings.setValue("ShowMenuBar", enabled);
}


void MainWin::saveStatusBarStatus(bool enabled)
{
    QtUiSettings uiSettings;
    uiSettings.setValue("ShowStatusBar", enabled);
}


void MainWin::setupSystray()
{
#ifdef HAVE_DBUS
    _systemTray = new StatusNotifierItem(this);
#elif !defined QT_NO_SYSTEMTRAYICON
    _systemTray = new LegacySystemTray(this);
#else
    _systemTray = new SystemTray(this); // dummy
#endif
    _systemTray->init();
}


void MainWin::setupToolBars()
{
    connect(_bufferWidget, SIGNAL(currentChanged(QModelIndex)),
        QtUi::toolBarActionProvider(), SLOT(currentBufferChanged(QModelIndex)));
    connect(_nickListWidget, SIGNAL(nickSelectionChanged(QModelIndexList)),
        QtUi::toolBarActionProvider(), SLOT(nickSelectionChanged(QModelIndexList)));

#ifdef Q_OS_MAC
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

    if (Quassel::runMode() != Quassel::Monolithic) {
        ActionCollection *coll = QtUi::actionCollection("General");
        _mainToolBar->addAction(coll->action("ConnectCore"));
        _mainToolBar->addAction(coll->action("DisconnectCore"));
    }

    QtUi::toolBarActionProvider()->addActions(_mainToolBar, ToolBarActionProvider::MainToolBar);
    _toolbarMenu->addAction(_mainToolBar->toggleViewAction());

#ifdef HAVE_KDE
    _nickToolBar = new KToolBar("NickToolBar", this, Qt::TopToolBarArea, false, true, true);
#else
    _nickToolBar = new QToolBar(this);
    _nickToolBar->setObjectName("NickToolBar");
#endif
    _nickToolBar->setWindowTitle(tr("Nick Toolbar"));
    _nickToolBar->setVisible(false); //default: not visible
    addToolBar(_nickToolBar);

    QtUi::toolBarActionProvider()->addActions(_nickToolBar, ToolBarActionProvider::NickToolBar);
    _toolbarMenu->addAction(_nickToolBar->toggleViewAction());

#ifdef Q_OS_MAC
    QtUiSettings uiSettings;

    bool visible = uiSettings.value("ShowMainToolBar", QVariant(true)).toBool();
    _mainToolBar->setVisible(visible);
    connect(_mainToolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(saveMainToolBarStatus(bool)));
#endif
}

void MainWin::saveMainToolBarStatus(bool enabled)
{
#ifdef Q_OS_MAC
    QtUiSettings uiSettings;
    uiSettings.setValue("ShowMainToolBar", enabled);
#else
    Q_UNUSED(enabled);
#endif
}


void MainWin::connectedToCore()
{
    Q_CHECK_PTR(Client::bufferViewManager());
    connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigAdded(int)), this, SLOT(addBufferView(int)));
    connect(Client::bufferViewManager(), SIGNAL(bufferViewConfigDeleted(int)), this, SLOT(removeBufferView(int)));
    connect(Client::bufferViewManager(), SIGNAL(initDone()), this, SLOT(loadLayout()));

    if (Client::transferManager()) {
        connect(Client::transferManager(), SIGNAL(transferAdded(QUuid)), SLOT(showNewTransferDlg(QUuid)));
    }

    setConnectedState();
}


void MainWin::setConnectedState()
{
    ActionCollection *coll = QtUi::actionCollection("General");

    coll->action("ConnectCore")->setEnabled(false);
    coll->action("DisconnectCore")->setEnabled(true);
    coll->action("ChangePassword")->setEnabled(true);
    coll->action("CoreInfo")->setEnabled(true);

    foreach(QAction *action, _fileMenu->actions()) {
        if (isRemoteCoreOnly(action))
            action->setVisible(!Client::internalCore());
    }

    disconnect(Client::backlogManager(), SIGNAL(updateProgress(int, int)), _msgProcessorStatusWidget, SLOT(setProgress(int, int)));
    disconnect(Client::backlogManager(), SIGNAL(messagesRequested(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
    disconnect(Client::backlogManager(), SIGNAL(messagesProcessed(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
    if (!Client::internalCore()) {
        connect(Client::backlogManager(), SIGNAL(updateProgress(int, int)), _msgProcessorStatusWidget, SLOT(setProgress(int, int)));
        connect(Client::backlogManager(), SIGNAL(messagesRequested(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
        connect(Client::backlogManager(), SIGNAL(messagesProcessed(const QString &)), this, SLOT(showStatusBarMessage(const QString &)));
    }

    // _viewMenu->setEnabled(true);
    if (!Client::internalCore())
        statusBar()->showMessage(tr("Connected to core."));
    else
        statusBar()->clearMessage();

    _coreConnectionStatusWidget->setVisible(!Client::internalCore());
    updateIcon();
    systemTray()->setState(SystemTray::Active);

    if (Client::networkIds().isEmpty()) {
        IrcConnectionWizard *wizard = new IrcConnectionWizard(this, Qt::Sheet);
        wizard->show();
    }
    else {
        // Monolithic always preselects last used buffer - Client only if the connection died
        if (Client::coreConnection()->wasReconnect() || Quassel::runMode() == Quassel::Monolithic) {
            QtUiSettings s;
            BufferId lastUsedBufferId(s.value("LastUsedBufferId").toInt());
            if (lastUsedBufferId.isValid())
                Client::bufferModel()->switchToBuffer(lastUsedBufferId);
        }
    }
}


void MainWin::loadLayout()
{
    QtUiSettings s;
    int accountId = Client::currentCoreAccount().accountId().toInt();
    QByteArray state = s.value(QString("MainWinState-%1").arg(accountId)).toByteArray();
    if (state.isEmpty()) {
        foreach(BufferViewDock *view, _bufferViews)
        view->show();
        _layoutLoaded = true;
        return;
    }
    _nickListWidget->setVisible(true);
    restoreState(state, accountId);
    int bufferViewId = s.value(QString("ActiveBufferView-%1").arg(accountId), -1).toInt();
    if (bufferViewId >= 0)
        changeActiveBufferView(bufferViewId);

    _layoutLoaded = true;
}


void MainWin::saveLayout()
{
    QtUiSettings s;
    int accountId = _bufferViews.count() ? Client::currentCoreAccount().accountId().toInt() : 0; // only save if we still have a layout!
    if (accountId > 0) {
        s.setValue(QString("MainWinState-%1").arg(accountId), saveState(accountId));
        BufferView *view = activeBufferView();
        s.setValue(QString("ActiveBufferView-%1").arg(accountId), view ? view->config()->bufferViewId() : -1);
    }
}


void MainWin::disconnectedFromCore()
{
    // save core specific layout and remove bufferviews;
    saveLayout();
    _layoutLoaded = false;

    QVariant actionData;
    BufferViewDock *dock;
    foreach(QAction *action, _bufferViewsMenu->actions()) {
        actionData = action->data();
        if (!actionData.isValid())
            continue;

        dock = qobject_cast<BufferViewDock *>(action->parent());
        if (dock && actionData.toInt() != -1) {
            removeAction(action);
            _bufferViews.removeAll(dock);
            dock->deleteLater();
        }
    }

    // store last active buffer
    QtUiSettings s;
    BufferId lastBufId = _bufferWidget->currentBuffer();
    if (lastBufId.isValid()) {
        s.setValue("LastUsedBufferId", lastBufId.toInt());
        // clear the current selection
        Client::bufferModel()->standardSelectionModel()->clearSelection();
    }
    restoreState(s.value("MainWinState").toByteArray());
    setDisconnectedState();
}


void MainWin::setDisconnectedState()
{
    ActionCollection *coll = QtUi::actionCollection("General");
    //ui.menuCore->setEnabled(false);
    coll->action("ConnectCore")->setEnabled(true);
    coll->action("DisconnectCore")->setEnabled(false);
    coll->action("CoreInfo")->setEnabled(false);
    coll->action("ChangePassword")->setEnabled(false);
    //_viewMenu->setEnabled(false);
    statusBar()->showMessage(tr("Not connected to core."));
    if (_msgProcessorStatusWidget)
        _msgProcessorStatusWidget->setProgress(0, 0);
    updateIcon();
    systemTray()->setState(SystemTray::Passive);
    _nickListWidget->setVisible(false);
}


void MainWin::userAuthenticationRequired(CoreAccount *account, bool *valid, const QString &errorMessage)
{
    Q_UNUSED(errorMessage)
    CoreConnectAuthDlg dlg(account, this);
    *valid = (dlg.exec() == QDialog::Accepted);
}


void MainWin::handleNoSslInClient(bool *accepted)
{
    QMessageBox box(QMessageBox::Warning, tr("Unencrypted Connection"), tr("<b>Your client does not support SSL encryption</b>"),
        QMessageBox::Ignore|QMessageBox::Cancel, this);
    box.setInformativeText(tr("Sensitive data, like passwords, will be transmitted unencrypted to your Quassel core."));
    box.setDefaultButton(QMessageBox::Ignore);
    *accepted = box.exec() == QMessageBox::Ignore;
}


void MainWin::handleNoSslInCore(bool *accepted)
{
    QMessageBox box(QMessageBox::Warning, tr("Unencrypted Connection"), tr("<b>Your core does not support SSL encryption</b>"),
        QMessageBox::Ignore|QMessageBox::Cancel, this);
    box.setInformativeText(tr("Sensitive data, like passwords, will be transmitted unencrypted to your Quassel core."));
    box.setDefaultButton(QMessageBox::Ignore);
    *accepted = box.exec() == QMessageBox::Ignore;
}


#ifdef HAVE_SSL

void MainWin::handleSslErrors(const QSslSocket *socket, bool *accepted, bool *permanently)
{
    QString errorString = "<ul>";
    foreach(const QSslError error, socket->sslErrors())
    errorString += QString("<li>%1</li>").arg(error.errorString());
    errorString += "</ul>";

    QMessageBox box(QMessageBox::Warning,
        tr("Untrusted Security Certificate"),
        tr("<b>The SSL certificate provided by the core at %1 is untrusted for the following reasons:</b>").arg(socket->peerName()),
        QMessageBox::Cancel, this);
    box.setInformativeText(errorString);
    box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setDefaultButton(box.addButton(tr("Show Certificate"), QMessageBox::HelpRole));

    QMessageBox::ButtonRole role;
    do {
        box.exec();
        role = box.buttonRole(box.clickedButton());
        if (role == QMessageBox::HelpRole) {
            SslInfoDlg dlg(socket, this);
            dlg.exec();
        }
    }
    while (role == QMessageBox::HelpRole);

    *accepted = role == QMessageBox::AcceptRole;
    if (*accepted) {
        QMessageBox box2(QMessageBox::Warning,
            tr("Untrusted Security Certificate"),
            tr("Would you like to accept this certificate forever without being prompted?"),
            0, this);
        box2.setDefaultButton(box2.addButton(tr("Current Session Only"), QMessageBox::NoRole));
        box2.addButton(tr("Forever"), QMessageBox::YesRole);
        box2.exec();
        *permanently =  box2.buttonRole(box2.clickedButton()) == QMessageBox::YesRole;
    }
}


#endif /* HAVE_SSL */

void MainWin::handleCoreConnectionError(const QString &error)
{
    QMessageBox::critical(this, tr("Core Connection Error"), error, QMessageBox::Ok);
}


void MainWin::showCoreConnectionDlg()
{
    CoreConnectDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        AccountId accId = dlg.selectedAccount();
        if (accId.isValid())
            Client::coreConnection()->connectToCore(accId);
    }
}


void MainWin::showCoreConfigWizard(const QVariantList &backends, const QVariantList &authenticators)
{
    CoreConfigWizard *wizard = new CoreConfigWizard(Client::coreConnection(), backends, authenticators, this);

    wizard->show();
}


void MainWin::showChannelList(NetworkId netId)
{
    ChannelListDlg *channelListDlg = new ChannelListDlg();

    if (!netId.isValid()) {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action)
            netId = action->data().value<NetworkId>();
    }

    channelListDlg->setAttribute(Qt::WA_DeleteOnClose);
    channelListDlg->setNetwork(netId);
    channelListDlg->show();
}


void MainWin::showIgnoreList(QString newRule)
{
    SettingsPageDlg dlg(new IgnoreListSettingsPage(this), this);
    // prepare config dialog for new rule
    if (!newRule.isEmpty())
        qobject_cast<IgnoreListSettingsPage *>(dlg.currentPage())->editIgnoreRule(newRule);
    dlg.exec();
}


void MainWin::showCoreInfoDlg()
{
    CoreInfoDlg(this).exec();
}


void MainWin::showAwayLog()
{
    if (_awayLog)
        return;
    AwayLogFilter *filter = new AwayLogFilter(Client::messageModel());
    _awayLog = new AwayLogView(filter, 0);
    filter->setParent(_awayLog);
    connect(_awayLog, SIGNAL(destroyed()), this, SLOT(awayLogDestroyed()));
    _awayLog->setAttribute(Qt::WA_DeleteOnClose);
    _awayLog->show();
}


void MainWin::awayLogDestroyed()
{
    _awayLog = 0;
}


void MainWin::showSettingsDlg()
{
    SettingsDlg *dlg = new SettingsDlg();

    //Category: Interface
    dlg->registerSettingsPage(new AppearanceSettingsPage(dlg));
    dlg->registerSettingsPage(new ChatViewSettingsPage(dlg));
    dlg->registerSettingsPage(new ChatViewColorSettingsPage(dlg));
    dlg->registerSettingsPage(new ChatMonitorSettingsPage(dlg));
    dlg->registerSettingsPage(new ItemViewSettingsPage(dlg));
    dlg->registerSettingsPage(new BufferViewSettingsPage(dlg));
    dlg->registerSettingsPage(new InputWidgetSettingsPage(dlg));
    dlg->registerSettingsPage(new TopicWidgetSettingsPage(dlg));
#ifdef HAVE_SONNET
    dlg->registerSettingsPage(new SonnetSettingsPage(dlg));
#endif
    dlg->registerSettingsPage(new HighlightSettingsPage(dlg));
    dlg->registerSettingsPage(new CoreHighlightSettingsPage(dlg));
    dlg->registerSettingsPage(new NotificationsSettingsPage(dlg));
    dlg->registerSettingsPage(new BacklogSettingsPage(dlg));

    //Category: IRC
    dlg->registerSettingsPage(new ConnectionSettingsPage(dlg));
    dlg->registerSettingsPage(new IdentitiesSettingsPage(dlg));
    dlg->registerSettingsPage(new NetworksSettingsPage(dlg));
    dlg->registerSettingsPage(new AliasesSettingsPage(dlg));
    dlg->registerSettingsPage(new IgnoreListSettingsPage(dlg));
    // dlg->registerSettingsPage(new DccSettingsPage(dlg)); not ready yet

    // Category: Remote Cores
    if (Quassel::runMode() != Quassel::Monolithic) {
        dlg->registerSettingsPage(new CoreAccountSettingsPage(dlg));
        dlg->registerSettingsPage(new CoreConnectionSettingsPage(dlg));
    }

    dlg->show();
}


void MainWin::showAboutDlg()
{
    AboutDlg(this).exec();
}


void MainWin::showShortcutsDlg()
{
#ifdef HAVE_KDE
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsDisallowed, this);
    foreach(KActionCollection *coll, QtUi::actionCollections())
    dlg.addCollection(coll, coll->property("Category").toString());
    dlg.configure(true);
#else
    SettingsPageDlg dlg(new ShortcutsSettingsPage(QtUi::actionCollections(), this), this);
    dlg.exec();
#endif
}


void MainWin::showNewTransferDlg(const QUuid &transferId)
{
    auto transfer = Client::transferManager()->transfer(transferId);
    if (transfer) {
        if (transfer->status() == Transfer::Status::New) {
            ReceiveFileDlg *dlg = new ReceiveFileDlg(transfer, this);
            dlg->show();
        }
    }
    else {
        qWarning() << "Unknown transfer ID" << transferId;
    }
}


void MainWin::onFullScreenToggled()
{
    // Relying on QWidget::isFullScreen is discouraged, see the KToggleFullScreenAction docs
    // Also, one should not use showFullScreen() or showNormal(), as those reset all other window flags

#ifdef HAVE_KDE
    static_cast<KToggleFullScreenAction*>(_fullScreenAction)->setFullScreen(this, _fullScreenAction->isChecked());
#else
    if (_fullScreenAction->isChecked())
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
#endif
}


/********************************************************************************************************/

bool MainWin::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::WindowActivate: {
        BufferId bufferId = Client::bufferModel()->currentBuffer();
        if (bufferId.isValid())
            Client::instance()->markBufferAsRead(bufferId);
        break;
    }
    case QEvent::WindowDeactivate:
        if (bufferWidget()->autoMarkerLineOnLostFocus())
            bufferWidget()->setMarkerLine();
        break;
    default:
        break;
    }
    return QMainWindow::event(event);
}


void MainWin::moveEvent(QMoveEvent *event)
{
    if (!(windowState() & Qt::WindowMaximized))
        _normalPos = event->pos();

    QMainWindow::moveEvent(event);
}


void MainWin::resizeEvent(QResizeEvent *event)
{
    if (!(windowState() & Qt::WindowMaximized))
        _normalSize = event->size();

    QMainWindow::resizeEvent(event);
}


void MainWin::closeEvent(QCloseEvent *event)
{
    QtUiSettings s;
    QtUiApplication *app = qobject_cast<QtUiApplication *> qApp;
    Q_ASSERT(app);
    // On OSX it can happen that the closeEvent occurs twice. (Especially if packaged with Frameworks)
    // This messes up MainWinState/MainWinHidden save/restore.
    // It's a bug in Qt: https://bugreports.qt.io/browse/QTBUG-43344
    if (!_aboutToQuit && !app->isAboutToQuit() && QtUi::haveSystemTray() && s.value("MinimizeOnClose").toBool()) {
        QtUi::hideMainWidget();
        event->ignore();
    }
    else if(!_aboutToQuit) {
        _aboutToQuit = true;
        event->accept();
        quit();
    }
    else {
        event->ignore();
    }
}


void MainWin::messagesInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);

    bool hasFocus = QApplication::activeWindow() != 0;

    for (int i = start; i <= end; i++) {
        QModelIndex idx = Client::messageModel()->index(i, ChatLineModel::ContentsColumn);
        if (!idx.isValid()) {
            qDebug() << "MainWin::messagesInserted(): Invalid model index!";
            continue;
        }
        Message::Flags flags = (Message::Flags)idx.data(ChatLineModel::FlagsRole).toInt();
        if (flags.testFlag(Message::Backlog) || flags.testFlag(Message::Self))
            continue;

        BufferId bufId = idx.data(ChatLineModel::BufferIdRole).value<BufferId>();
        BufferInfo::Type bufType = Client::networkModel()->bufferType(bufId);

        // check if bufferId belongs to the shown chatlists
        if (!(Client::bufferViewOverlay()->bufferIds().contains(bufId) ||
              Client::bufferViewOverlay()->tempRemovedBufferIds().contains(bufId)))
            continue;

        // check if it's the buffer currently displayed
        if (hasFocus && bufId == Client::bufferModel()->currentBuffer())
            continue;

        // only show notifications for higlights or queries
        if (bufType != BufferInfo::QueryBuffer && !(flags & Message::Highlight))
            continue;

        // and of course: don't notify for ignored messages
        if (Client::ignoreListManager() && Client::ignoreListManager()->match(idx.data(MessageModel::MessageRole).value<Message>(), Client::networkModel()->networkName(bufId)))
            continue;

        // seems like we have a legit notification candidate!
        QModelIndex senderIdx = Client::messageModel()->index(i, ChatLineModel::SenderColumn);
        QString sender = senderIdx.data(ChatLineModel::EditRole).toString();
        QString contents = idx.data(ChatLineModel::DisplayRole).toString();
        AbstractNotificationBackend::NotificationType type;

        if (bufType == BufferInfo::QueryBuffer && !hasFocus)
            type = AbstractNotificationBackend::PrivMsg;
        else if (bufType == BufferInfo::QueryBuffer && hasFocus)
            type = AbstractNotificationBackend::PrivMsgFocused;
        else if (flags & Message::Highlight && !hasFocus)
            type = AbstractNotificationBackend::Highlight;
        else
            type = AbstractNotificationBackend::HighlightFocused;

        QtUi::instance()->invokeNotification(bufId, type, sender, contents);
    }
}


void MainWin::currentBufferChanged(BufferId buffer)
{
    if (buffer.isValid())
        Client::instance()->markBufferAsRead(buffer);
}


void MainWin::clientNetworkCreated(NetworkId id)
{
    const Network *net = Client::network(id);
    QAction *act = new QAction(net->networkName(), this);
    act->setObjectName(QString("NetworkAction-%1").arg(id.toInt()));
    act->setData(QVariant::fromValue<NetworkId>(id));
    connect(net, SIGNAL(updatedRemotely()), this, SLOT(clientNetworkUpdated()));
    connect(act, SIGNAL(triggered()), this, SLOT(connectOrDisconnectFromNet()));

    QAction *beforeAction = 0;
    foreach(QAction *action, _networksMenu->actions()) {
        if (!action->data().isValid()) // ignore stock actions
            continue;
        if (net->networkName().localeAwareCompare(action->text()) < 0) {
            beforeAction = action;
            break;
        }
    }
    _networksMenu->insertAction(beforeAction, act);
}


void MainWin::clientNetworkUpdated()
{
    const Network *net = qobject_cast<const Network *>(sender());
    if (!net)
        return;

    QAction *action = findChild<QAction *>(QString("NetworkAction-%1").arg(net->networkId().toInt()));
    if (!action)
        return;

    action->setText(net->networkName());

    switch (net->connectionState()) {
    case Network::Initialized:
        action->setIcon(QIcon::fromTheme("network-connect"));
        // if we have no currently selected buffer, jump to the first connecting statusbuffer
        if (!bufferWidget()->currentBuffer().isValid()) {
            QModelIndex idx = Client::networkModel()->networkIndex(net->networkId());
            if (idx.isValid()) {
                BufferId statusBufferId = idx.data(NetworkModel::BufferIdRole).value<BufferId>();
                Client::bufferModel()->switchToBuffer(statusBufferId);
            }
        }
        break;
    case Network::Disconnected:
        action->setIcon(QIcon::fromTheme("network-disconnect"));
        break;
    default:
        action->setIcon(QIcon::fromTheme("network-wired"));
    }
}


void MainWin::clientNetworkRemoved(NetworkId id)
{
    QAction *action = findChild<QAction *>(QString("NetworkAction-%1").arg(id.toInt()));
    if (!action)
        return;

    action->deleteLater();
}


void MainWin::connectOrDisconnectFromNet()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act) return;
    const Network *net = Client::network(act->data().value<NetworkId>());
    if (!net) return;
    if (net->connectionState() == Network::Disconnected) net->requestConnect();
    else net->requestDisconnect();
}


void MainWin::on_jumpHotBuffer_triggered()
{
    if (!_bufferHotList->rowCount())
        return;

    Client::bufferModel()->switchToBuffer(_bufferHotList->hottestBuffer());
}

void MainWin::on_bufferSearch_triggered()
{
    if (_activeBufferViewIndex < 0 || _activeBufferViewIndex >= _bufferViews.count()) {
        qWarning() << "Tried to activate filter on invalid bufferview:" << _activeBufferViewIndex;
        return;
    }

    _bufferViews[_activeBufferViewIndex]->activateFilter();
}


void MainWin::onJumpKey()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action || !Client::bufferModel())
        return;
    int idx = action->property("Index").toInt();

    if (_jumpKeyMap.isEmpty())
        _jumpKeyMap = CoreAccountSettings().jumpKeyMap();

    if (!_jumpKeyMap.contains(idx))
        return;

    BufferId buffer = _jumpKeyMap.value(idx);
    if (buffer.isValid())
        Client::bufferModel()->switchToBuffer(buffer);
}


void MainWin::bindJumpKey()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action || !Client::bufferModel())
        return;
    int idx = action->property("Index").toInt();

    _jumpKeyMap[idx] = Client::bufferModel()->currentBuffer();
    CoreAccountSettings().setJumpKeyMap(_jumpKeyMap);
}


void MainWin::on_actionDebugNetworkModel_triggered()
{
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


void MainWin::on_actionDebugHotList_triggered()
{
    _bufferHotList->invalidate();
    _bufferHotList->sort(0, Qt::DescendingOrder);

    QTreeView *view = new QTreeView;
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->setModel(_bufferHotList);
    view->show();
}


void MainWin::on_actionDebugBufferViewOverlay_triggered()
{
    DebugBufferViewOverlay *overlay = new DebugBufferViewOverlay(0);
    overlay->setAttribute(Qt::WA_DeleteOnClose);
    overlay->show();
}


void MainWin::on_actionDebugMessageModel_triggered()
{
    QTableView *view = new QTableView(0);
    DebugMessageModelFilter *filter = new DebugMessageModelFilter(view);
    filter->setSourceModel(Client::messageModel());
    view->setModel(filter);
    view->setAttribute(Qt::WA_DeleteOnClose, true);
    view->verticalHeader()->hide();
    view->horizontalHeader()->setStretchLastSection(true);
    view->show();
}


void MainWin::on_actionDebugLog_triggered()
{
    DebugLogWidget *logWidget = new DebugLogWidget(0);
    logWidget->show();
}


void MainWin::showStatusBarMessage(const QString &message)
{
    statusBar()->showMessage(message, 10000);
}
