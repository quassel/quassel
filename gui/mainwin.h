/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

class ServerListDlg;
class CoreConnectDlg;
class NetworkView;
class Buffer;
class SettingsDlg;

//!\brief The main window and central object of Quassel GUI.
/** In addition to displaying the main window including standard stuff like a menubar,
 * dockwidgets and of course the chat window, this class also stores all data it
 * receives from the core, and it maintains a list of all known nicks.
 */
class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin();

  protected:
    void closeEvent(QCloseEvent *event);

  signals:
    void sendInput(QString network, QString buffer, QString message);
    void bufferSelected(QString net, QString buffer);

  private slots:
    void userInput(QString, QString, QString);
    void networkConnected(QString);
    void networkDisconnected(QString);
    void recvNetworkState(QString, QVariant);
    void recvMessage(QString network, Message message);
    void recvStatusMsg(QString network, QString message);
    void setTopic(QString, QString, QString);
    void setNicks(QString, QString, QStringList);
    void addNick(QString net, QString nick, VarMap props);
    void removeNick(QString net, QString nick);
    void renameNick(QString net, QString oldnick, QString newnick);
    void updateNick(QString net, QString nick, VarMap props);
    void setOwnNick(QString net, QString nick);

    void showServerList();
    void showSettingsDlg();

    void showBuffer(QString net, QString buf);

  private:
    Ui::MainWin ui;

    void setupMenus();
    void syncToCore();  // implemented in main_mono.cpp or main_gui.cpp
    Buffer * getBuffer(QString net, QString buf);
    void buffersUpdated();

    QWorkspace *workspace;
    QWidget *widget;

    ServerListDlg *serverListDlg;
    CoreConnectDlg *coreConnectDlg;
    SettingsDlg *settingsDlg;

    QString currentNetwork, currentBuffer;
    QHash<QString, QHash<QString, Buffer*> > buffers;
    QHash<QString, QHash<QString, VarMap> > nicks;
    QHash<QString, bool> connected;
    QHash<QString, QString> ownNick;
    QHash<QString, QList<Message> > coreBackLog;

    NetworkView *netView;
};

#endif
