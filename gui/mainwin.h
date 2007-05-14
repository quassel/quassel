/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include <QtGui>
#include "gui/ui_mainwin.h"

#include "global.h"
#include "message.h"
#include "chatwidget.h"

class ServerListDlg;
class CoreConnectDlg;
class NetworkView;
class Buffer;
class BufferWidget;
class SettingsDlg;

extern LayoutThread *layoutThread;

//!\brief The main window and central object of Quassel GUI.
/** In addition to displaying the main window including standard stuff like a menubar,
 * dockwidgets and of course the chat window, this class also stores all data it
 * receives from the core, and it maintains a list of all known nicks.
 */
class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin();
    ~MainWin();

    void init();
    void registerNetView(NetworkView *);

  protected:
    void closeEvent(QCloseEvent *event);

  signals:
    void sendInput(BufferId, QString message);
    void bufferSelected(Buffer *);
    void bufferUpdated(Buffer *);
    void bufferDestroyed(Buffer *);
    void backlogReceived(Buffer *, QList<Message>);
    void requestBacklog(BufferId, QVariant, QVariant);

    void importOldBacklog();

  private slots:
    void userInput(BufferId, QString);
    void networkConnected(QString);
    void networkDisconnected(QString);
    void recvNetworkState(QString, QVariant);
    void recvMessage(Message message);
    void recvStatusMsg(QString network, QString message);
    void setTopic(QString net, QString buf, QString);
    void addNick(QString net, QString nick, VarMap props);
    void removeNick(QString net, QString nick);
    void renameNick(QString net, QString oldnick, QString newnick);
    void updateNick(QString net, QString nick, VarMap props);
    void setOwnNick(QString net, QString nick);
    void recvBacklogData(BufferId, QList<QVariant>, bool);

    void showServerList();
    void showSettingsDlg();

    void showBuffer(BufferId);
    void showBuffer(Buffer *);

    void importBacklog();
    void layoutMsg();

  private:
    Ui::MainWin ui;

    void setupMenus();
    void setupViews();
    void setupSettingsDlg();
    void syncToCore();  // implemented in main_mono.cpp or main_gui.cpp
    //Buffer * getBuffer(QString net, QString buf);
    Buffer *getBuffer(BufferId);
    BufferId getStatusBufferId(QString net);
    BufferId getBufferId(QString net, QString buf);
    //void buffersUpdated();

    QSystemTrayIcon *systray;
    //QWorkspace *workspace;
    //QWidget *widget;
    //BufferWidget *bufferWidget;

    ServerListDlg *serverListDlg;
    CoreConnectDlg *coreConnectDlg;
    SettingsDlg *settingsDlg;

    //QString currentNetwork, currentBuffer;
    //QHash<QString, QHash<QString, Buffer*> > buffers;
    uint currentBuffer;
    QHash<BufferId, Buffer *> buffers;
    QHash<uint, BufferId> bufferIds;
    QHash<QString, QHash<QString, VarMap> > nicks;
    QHash<QString, bool> connected;
    QHash<QString, QString> ownNick;
    //QHash<QString, QList<Message> > coreBackLog;
    QList<BufferId> coreBuffers;

    QList<NetworkView *> netViews;

    QTimer *layoutTimer;
    QList<Message> layoutQueue;
};

#endif
