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

#ifndef MAINWIN_H_
#define MAINWIN_H_

#ifdef HAVE_KDE
#  include <KMainWindow>
#else
#  include <QMainWindow>
#endif

#include <QSystemTrayIcon>

#include "qtui.h"
#include "sessionsettings.h"
#include "titlesetter.h"

class ActionCollection;
class BufferView;
class BufferViewConfig;
class ClientBufferViewConfig;
class BufferViewDock;
class BufferWidget;
class InputWidget;
class MsgProcessorStatusWidget;
class NickListWidget;
class SystemTrayIcon;

class QMenu;
class QLabel;
class QToolBar;

class KHelpMenu;

//!\brief The main window of Quassel's QtUi.
class MainWin
#ifdef HAVE_KDE
: public KMainWindow {
#else
: public QMainWindow {
#endif
  Q_OBJECT

  public:
    MainWin(QWidget *parent = 0);
    virtual ~MainWin();

    void init();

    void addBufferView(ClientBufferViewConfig *config);
    BufferView *allBuffersView() const;

    inline QSystemTrayIcon *systemTrayIcon() const;

    virtual bool event(QEvent *event);

    static void flagRemoteCoreOnly(QObject *object) { object->setProperty("REMOTE_CORE_ONLY", true); }
    static bool isRemoteCoreOnly(QObject *object) { return object->property("REMOTE_CORE_ONLY").toBool(); }

  public slots:
    void saveStateToSession(const QString &sessionId);
    void saveStateToSessionSettings(SessionSettings &s);
    void showStatusBarMessage(const QString &message);

  protected:
    void closeEvent(QCloseEvent *event);
    virtual void changeEvent(QEvent *event);

  protected slots:
    void connectedToCore();
    void setConnectedState();
    void updateLagIndicator(int lag = -1);
    void disconnectedFromCore();
    void setDisconnectedState();
    void systrayActivated(QSystemTrayIcon::ActivationReason);

  private slots:
    void addBufferView(int bufferViewConfigId);
    void awayLogDestroyed();
    void removeBufferView(int bufferViewConfigId);
    void messagesInserted(const QModelIndex &parent, int start, int end);
    void showAboutDlg();
    void showChannelList(NetworkId netId = NetworkId());
    void startInternalCore();
    void showCoreConnectionDlg(bool autoConnect = false);
    void showCoreInfoDlg();
    void showAwayLog();
    void showSettingsDlg();
    void showNotificationsDlg();
#ifdef HAVE_KDE
    void showShortcutsDlg();
#endif
    void on_actionConfigureNetworks_triggered();
    void on_actionConfigureViews_triggered();
    void on_actionLockLayout_toggled(bool lock);
    void on_actionDebugNetworkModel_triggered();
    void on_actionDebugMessageModel_triggered();
    void on_actionDebugLog_triggered();

    void clientNetworkCreated(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();
    void connectOrDisconnectFromNet();

    void saveStatusBarStatus(bool enabled);

    void loadLayout();
    void saveLayout();

  signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();

  private:
#ifdef HAVE_KDE
    KHelpMenu *_kHelpMenu;
#endif

    QMenu *systrayMenu;
    QLabel *coreLagLabel;
    QLabel *sslLabel;
    MsgProcessorStatusWidget *msgProcessorStatusWidget;

    TitleSetter _titleSetter;

    void setupActions();
    void setupBufferWidget();
    void setupMenus();
    void setupNickWidget();
    void setupChatMonitor();
    void setupInputWidget();
    void setupTopicWidget();
    void setupStatusBar();
    void setupSystray();
    void setupTitleSetter();
    void setupToolBars();

    void updateIcon();
    void hideToTray();
    void toggleMinimizedToTray();
    void enableMenus();

    QSystemTrayIcon *_trayIcon;

    QList<BufferViewDock *> _bufferViews;
    BufferWidget *_bufferWidget;
    NickListWidget *_nickListWidget;
    InputWidget *_inputWidget;

    QMenu *_fileMenu, *_networksMenu, *_viewMenu, *_bufferViewsMenu, *_settingsMenu, *_helpMenu, *_helpDebugMenu;
    QMenu *_toolbarMenu;
    QToolBar *_mainToolBar, *_chatViewToolBar, *_nickToolBar;

    QWidget *_awayLog;
    friend class QtUi;
};

QSystemTrayIcon *MainWin::systemTrayIcon() const {
  return _trayIcon;
}

#endif
