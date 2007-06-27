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
#include "ui_mainwin.h"

#include "quasselui.h"
//#include "global.h"
#include "message.h"
#include "chatwidget.h"
#include "bufferviewfilter.h"
#include "bufferview.h"

class ServerListDlg;
class CoreConnectDlg;
class Buffer;
class SettingsDlg;

//!\brief The main window and central object of Quassel GUI.
/** In addition to displaying the main window including standard stuff like a menubar,
 * dockwidgets and of course the chat window, this class also stores all data it
 * receives from the core, and it maintains a list of all known nicks.
 */
class MainWin : public QMainWindow, public AbstractUi {
  Q_OBJECT

  public:
    MainWin();
    virtual ~MainWin();

    void init();
    void addBufferView(const QString &, QAbstractItemModel *, const BufferViewFilter::Modes &, const QStringList &);

    AbstractUiMsg *layoutMsg(const Message &);

  protected:
    void closeEvent(QCloseEvent *event);

    //void importOldBacklog();

  private slots:

    void showServerList();
    void showSettingsDlg();

    void showBuffer(BufferId);
    void showBuffer(Buffer *);

    void importBacklog();

  signals:
    void importOldBacklog();

  private:
    Ui::MainWin ui;

    void setupMenus();
    void setupViews();
    void setupSettingsDlg();

    QSystemTrayIcon *systray;

    ServerListDlg *serverListDlg;
    CoreConnectDlg *coreConnectDlg;
    SettingsDlg *settingsDlg;

    uint currentBuffer;

    QList<QDockWidget *> netViews;

};

#endif
