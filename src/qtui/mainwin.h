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

#ifndef MAINWIN_H_
#define MAINWIN_H_

#include "ui_mainwin.h"

#include "qtui.h"
#include "titlesetter.h"
#include "sessionsettings.h"

#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>

class ActionCollection;
class Buffer;
class BufferViewConfig;
class MsgProcessorStatusWidget;
class Message;
class NickListWidget;

#ifdef HAVE_DBUS
#  include "desktopnotifications.h"
#endif

//!\brief The main window of Quassel's QtUi.
class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin(QWidget *parent = 0);
    virtual ~MainWin();

    void init();

    void addBufferView(BufferViewConfig *config = 0);

    void displayTrayIconMessage(const QString &title, const QString &message);

#ifdef HAVE_DBUS
    void sendDesktopNotification(const QString &title, const QString &message);
#endif

    virtual bool event(QEvent *event);
  public slots:
    void setTrayIconActivity(bool active = false);
    void saveStateToSession(const QString &sessionId);
    void saveStateToSessionSettings(SessionSettings &s);

  protected:
    void closeEvent(QCloseEvent *event);
    virtual void changeEvent(QEvent *event);

  protected slots:
    void connectedToCore();
    void setConnectedState();
    void updateLagIndicator(int lag);
    void securedConnection();
    void disconnectedFromCore();
    void setDisconnectedState();
    void systrayActivated( QSystemTrayIcon::ActivationReason );

  private slots:
    void addBufferView(int bufferViewConfigId);
    void removeBufferView(int bufferViewConfigId);
    void messagesInserted(const QModelIndex &parent, int start, int end);
    void showChannelList(NetworkId netId = NetworkId());
    void showCoreInfoDlg();
    void showSettingsDlg();
    void on_actionEditNetworks_triggered();
    void on_actionManageViews_triggered();
    void on_actionLockDockPositions_toggled(bool lock);
    void showAboutDlg();
    void on_actionDebugNetworkModel_triggered(bool);

    void showCoreConnectionDlg(bool autoConnect = false);

    void clientNetworkCreated(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();
    void connectOrDisconnectFromNet();

    void makeTrayIconBlink();
    void saveStatusBarStatus(bool enabled);

    void loadLayout();
    void saveLayout();

#ifdef HAVE_DBUS
    void desktopNotificationClosed(uint id, uint reason);
    void desktopNotificationInvoked(uint id, const QString & action);
#endif

  signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();

  private:
    Ui::MainWin ui;

    QMenu *systrayMenu;
    QLabel *coreLagLabel;
    QLabel *sslLabel;
    MsgProcessorStatusWidget *msgProcessorStatusWidget;

    TitleSetter _titleSetter;

    void setupActions();
    void setupMenus();
    void setupViews();
    void setupNickWidget();
    void setupChatMonitor();
    void setupInputWidget();
    void setupTopicWidget();
    void setupStatusBar();
    void setupSystray();

    void toggleVisibility();
    void enableMenus();

    QSystemTrayIcon *systray;
    QPixmap activeTrayIcon;
    QPixmap onlineTrayIcon;
    QPixmap offlineTrayIcon;
    bool trayIconActive;
    QTimer *timer;

    BufferId currentBuffer;
    QString currentProfile;

    QList<QDockWidget *> _netViews;
    NickListWidget *nickListWidget;

    ActionCollection *_actionCollection;

#ifdef HAVE_DBUS
    org::freedesktop::Notifications *desktopNotifications;
    quint32 notificationId;
#endif

    friend class QtUi;
};

#endif
