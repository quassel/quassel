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
#include <QtCore>

#include "global.h"

#include "mainwin.h"
#include "channelwidget.h"
#include "serverlist.h"
#include "coreconnectdlg.h"

MainWin::MainWin() : QMainWindow() {

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  QSettings s;
  s.beginGroup("Geometry");
  resize(s.value("MainWinSize", QSize(500, 400)).toSize());
  move(s.value("MainWinPos", QPoint(50, 50)).toPoint());
  s.endGroup();

  //workspace = new QWorkspace(this);
  //setCentralWidget(workspace);
  //ChannelWidget *cw = new ChannelWidget(this);
  //workspace->addWindow(cw);
  //setCentralWidget(cw);
  statusBar()->showMessage(tr("Waiting for core..."));
  setEnabled(false);
  show();
  syncToCore();
  setEnabled(true);
  serverListDlg = new ServerListDlg(this);
  serverListDlg->setVisible(serverListDlg->showOnStartup());
  setupMenus();
  //identitiesAct = settingsMenu->addAction(QIcon(":/default/identity.png"), tr("&Identities..."), serverListDlg, SLOT(editIdentities()));
  //showServerList();
  ChannelWidget *cw = new ChannelWidget(this);
  setCentralWidget(cw);
  //setEnabled(true);
  statusBar()->showMessage(tr("Ready."));
}

void MainWin::syncToCore() {
  if(global->getData("CoreReady").toBool()) return;
  // ok, apparently we are running as standalone GUI
  coreConnectDlg = new CoreConnectDlg(this);
  if(coreConnectDlg->exec() != QDialog::Accepted) {
    //qApp->quit();
    exit(1);
  }
  VarMap state = coreConnectDlg->getCoreState().toMap()["CoreData"].toMap();
  delete coreConnectDlg;
  QString key;
  foreach(key, state.keys()) {
    global->updateData(key, state[key]);
  }
  if(!global->getData("CoreReady").toBool()) {
    QMessageBox::critical(this, tr("Fatal Error"), tr("<b>Could not synchronize with Quassel Core!</b><br>Quassel GUI will be aborted."), QMessageBox::Abort);
    //qApp->quit();
    exit(1);
  }
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

void MainWin::closeEvent(QCloseEvent *event)
{
  //if (userReallyWantsToQuit()) {
    QSettings s;
    s.beginGroup("Geometry");
    s.setValue("MainWinSize", size());
    s.setValue("MainWinPos", pos());
    s.endGroup();
    event->accept();
  //} else {
    //event->ignore();
  //}
}
