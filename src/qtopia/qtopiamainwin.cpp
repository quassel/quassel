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

#include "qtopiamainwin.h"

#include "networkmodel.h"
#include "bufferviewwidget.h"
#include "nicklistwidget.h"
#include "chatline.h"
#include "clientbacklogmanager.h"
#include "coreconnectdlg.h"
#include "global.h"
#include "mainwidget.h"
#include "message.h"
#include "network.h"
#include "qtopiaui.h"
#include "signalproxy.h"

#include "ui_aboutdlg.h"

#include <Qtopia>
#include <QSoftMenuBar>

// This constructor is the first thing to be called for a Qtopia app, so we do the init stuff
// here (rather than in a main.cpp).
QtopiaMainWin::QtopiaMainWin(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
  Global::registerMetaTypes();

#include "../../version.inc"

  Global::runMode = Global::ClientOnly;
  Global::defaultPort = 4242;
  Global::DEBUG = true;

  Network::setDefaultCodecForServer("ISO-8859-1");
  Network::setDefaultCodecForEncoding("UTF-8");
  Network::setDefaultCodecForDecoding("ISO-8859-15");

  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel Project");

  QtopiaUi *gui = new QtopiaUi(this);
  Client::init(gui);

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":icons/quassel-icon.png"));
  setWindowIconText("Quassel IRC");

  mainWidget = new MainWidget(this);
  mainWidget->setModel(Client::bufferModel());
  mainWidget->setSelectionModel(Client::bufferModel()->standardSelectionModel());
  setCentralWidget(mainWidget);

  toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setWindowTitle(tr("Show Toolbar"));
  addToolBar(toolBar);

  //bufferViewWidget = new BufferViewWidget(this);
  bufferViewWidget = 0;  // delayed creation to avoid QPainter warnings
  nickListWidget = new NickListWidget(this);

  connect(mainWidget, SIGNAL(currentChanged(BufferId)), this, SLOT(showBuffer(BufferId)));

  setupActions();

  init();
  //gui->init();

}

// at this point, client is fully initialized
void QtopiaMainWin::init() {
  showMaximized();
  CoreConnectDlg *dlg = new CoreConnectDlg(this);
  //setCentralWidget(dlg);
  dlg->showMaximized();
  dlg->exec();
}

QtopiaMainWin::~QtopiaMainWin() {


}

void QtopiaMainWin::closeEvent(QCloseEvent *event) {
#ifndef DEVELMODE
  QMessageBox *box = new QMessageBox(QMessageBox::Question, tr("Quit Quassel IRC?"), tr("Do you really want to quit Quassel IRC?"),
                                     QMessageBox::Cancel, this);
  QAbstractButton *quit = box->addButton(tr("Quit"), QMessageBox::AcceptRole);
  box->exec();
  if(box->clickedButton() == quit) event->accept();
  else event->ignore();
  box->deleteLater();
#else
  event->accept();
#endif
}

void QtopiaMainWin::setupActions() {
  showBuffersAction = toolBar->addAction(QIcon(":icon/options-hide"), tr("Show Buffers"), this, SLOT(showBufferView()));  // FIXME provide real icon
  showNicksAction = toolBar->addAction(QIcon(":icon/list"), tr("Show Nicks"), this, SLOT(showNickList()));
  showNicksAction->setEnabled(false);

  QMenu *menu = new QMenu(this);
  menu->addAction(showBuffersAction);
  menu->addAction(showNicksAction);
  menu->addSeparator();
  menu->addAction(toolBar->toggleViewAction());
  menu->addSeparator();
  menu->addAction(tr("About..."), this, SLOT(showAboutDlg()));

  QSoftMenuBar::addMenuTo(this, menu);
}

void QtopiaMainWin::connectedToCore() {
  foreach(BufferInfo id, Client::allBufferInfos()) {
    Client::backlogManager()->requestBacklog(id.bufferId(), 500, -1);
  }
}

void QtopiaMainWin::disconnectedFromCore() {


}

AbstractUiMsg *QtopiaMainWin::layoutMsg(const Message &msg) {
  return new ChatLine(msg);
  //return 0;
}

void QtopiaMainWin::showBuffer(BufferId id) {
  nickListWidget->setBuffer(id);
  Buffer *b = Client::buffer(id);
  //showNicksAction->setEnabled(b && b->bufferInfo().type() == BufferInfo::ChannelBuffer);  FIXME enable again when we have a nicklist!

}

void QtopiaMainWin::showBufferView() {
  if(!bufferViewWidget) {
    bufferViewWidget = new BufferViewWidget(this);
    connect(mainWidget, SIGNAL(currentChanged(BufferId)), bufferViewWidget, SLOT(accept()));
  }
  bufferViewWidget->showMaximized();
}

void QtopiaMainWin::showNickList() {
  nickListWidget->showMaximized();
}

void QtopiaMainWin::showAboutDlg() {
  QDialog *dlg = new QDialog(this);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  Ui::AboutDlg ui;
  ui.setupUi(dlg);
  dlg->showMaximized();
}

