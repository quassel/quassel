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

#include <QtGui>

#include "mainwin.h"
#include "channelwidget.h"
#include "serverlist.h"

#include "core.h"
#include "server.h"

MainWin::MainWin() : QMainWindow() {

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":/default/tux.png"));
  setWindowIconText("Quassel IRC");
  //workspace = new QWorkspace(this);
  //setCentralWidget(workspace);
  ChannelWidget *cw = new ChannelWidget(this);
  //workspace->addWindow(cw);
  setCentralWidget(cw);
  serverListDlg = new ServerListDlg(this);
  serverListDlg->setVisible(serverListDlg->showOnStartup());
  //showServerList();

  setupMenus();
  statusBar()->showMessage(tr("Ready"));

}

void MainWin::setupMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  serverListAct = fileMenu->addAction(QIcon(":/default/server.png"), tr("&Server List..."), this, SLOT(showServerList()), tr("F7"));
  fileMenu->addSeparator();
  quitAct = fileMenu->addAction(QIcon(":/default/exit.png"), tr("&Quit"), qApp, SLOT(quit()), tr("CTRL+Q"));

  editMenu = menuBar()->addMenu(tr("&Edit"));
  editMenu->setEnabled(0);

  ircMenu = menuBar()->addMenu(tr("&IRC"));
  ircMenu->setEnabled(0);

  serverMenu = menuBar()->addMenu(tr("Ser&ver"));
  serverMenu->setEnabled(0);

  windowMenu = menuBar()->addMenu(tr("&Window"));
  windowMenu->setEnabled(0);

  settingsMenu = menuBar()->addMenu(tr("&Settings"));
  identitiesAct = settingsMenu->addAction(QIcon(":/default/identity.png"), tr("&Identities..."), serverListDlg, SLOT(editIdentities()));
  settingsMenu->addSeparator();
  configAct = settingsMenu->addAction(QIcon(":/default/configure.png"), tr("&Configure Quassel..."));
  configAct->setEnabled(0);

  helpMenu = menuBar()->addMenu(tr("&Help"));
  aboutAct = helpMenu->addAction(tr("&About"));
  aboutAct->setEnabled(0);
  aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));


  //toolBar = new QToolBar("Test", this);
  //toolBar->addAction(identitiesAct);
  //addToolBar(Qt::TopToolBarArea, toolBar);
}

void MainWin::showServerList() {
//  if(!serverListDlg) {
//    serverListDlg = new ServerListDlg(this);
//  }
  serverListDlg->show();
}

