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

//#include "global.h"
#include "message.h"
#include "chatwidget.h"
#include "bufferviewfilter.h"
#include "bufferview.h"

class ServerListDlg;
class CoreConnectDlg;
class Buffer;
class SettingsDlg;
class MainWin;

class QtGui : public AbstractUi {
  Q_OBJECT

  public:
    QtGui();
    ~QtGui();
    void init();
    AbstractUiMsg *layoutMsg(const Message &);

  protected slots:
    void connectedToCore();
    void disconnectedFromCore();

  private:
    MainWin *mainWin;
};


//!\brief The main window of Quassel's QtGui.
class MainWin : public QMainWindow {
  Q_OBJECT

  public:
    MainWin(QtGui *gui, QWidget *parent = 0);
    virtual ~MainWin();

    void init();
    void addBufferView(const QString &, QAbstractItemModel *, const BufferViewFilter::Modes &, const QStringList &);

    AbstractUiMsg *layoutMsg(const Message &);

  protected:
    void closeEvent(QCloseEvent *event);

  protected slots:
    void connectedToCore();
    void disconnectedFromCore();

  private slots:

    void showServerList();
    void showSettingsDlg();

    void showBuffer(BufferId);
    void showBuffer(Buffer *);

    void importBacklog();

  signals:
    void connectToCore(const VarMap &connInfo);
    void disconnectFromCore();
    void requestBacklog(BufferId, QVariant, QVariant);
    void importOldBacklog();

  private:
    Ui::MainWin ui;
    QtGui *gui;

    void setupMenus();
    void setupViews();
    void setupSettingsDlg();

    void enableMenus();

    QSystemTrayIcon *systray;

    ServerListDlg *serverListDlg;
    CoreConnectDlg *coreConnectDlg;
    SettingsDlg *settingsDlg;

    uint currentBuffer;

    QList<QDockWidget *> netViews;

    friend class QtGui;
};

#endif
