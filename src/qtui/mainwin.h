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

#ifndef _MAINWIN_H_
#define _MAINWIN_H_

#include "ui_mainwin.h"

#include "qtui.h"
#include "bufferviewfilter.h"

#include <QSystemTrayIcon>

class ServerListDlg;
class CoreConnectDlg;
class Buffer;
class SettingsDlg;
class QtUi;
class Message;
class NickListWidget;
class DebugConsole;

//!\brief The main window of Quassel's QtUi.
class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin(QtUi *gui, QWidget *parent = 0);
    virtual ~MainWin();

    void init();
    QDockWidget *addBufferView(const QString &, QAbstractItemModel *, const BufferViewFilter::Modes &, const QList<NetworkId> &);

    AbstractUiMsg *layoutMsg(const Message &);

  protected:
    void closeEvent(QCloseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);

  protected slots:
    void connectedToCore();
    void disconnectedFromCore();
    void systrayActivated( QSystemTrayIcon::ActivationReason );

  private slots:

    void showSettingsDlg();
    void showNetworkDlg();
    void showDebugConsole();

    void showCoreConnectionDlg(bool autoConnect = false);
    void coreConnectionDlgFinished(int result);

    void clientNetworkCreated(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();
    void connectOrDisconnectFromNet();

  signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();
    void requestBacklog(BufferInfo, QVariant, QVariant);

  private:
    Ui::MainWin ui;
    QtUi *gui;

    QMenu *systrayMenu;

    void setupMenus();
    void setupViews();
    void setupNickWidget();
    void setupChatMonitor();
    void setupInputWidget();
    void setupTopicWidget();
    void setupSystray();

    void setupSettingsDlg();

    void enableMenus();

    void bindKey(int key);
    void jumpKey(int key);

    QHash<int, BufferId> _keyboardJump;
    QSystemTrayIcon *systray;

    CoreConnectDlg *coreConnectDlg;
    SettingsDlg *settingsDlg;

    BufferId currentBuffer;
    QString currentProfile;

    QList<QDockWidget *> netViews;
    QDockWidget *nickDock;
    NickListWidget *nickListWidget;

    QAction *actionEditNetworks;
    QList<QAction *> networkActions;

    DebugConsole *debugConsole;
    friend class QtUi;
};

#endif
