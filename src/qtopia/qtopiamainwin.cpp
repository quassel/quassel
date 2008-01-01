/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
#include "coreconnectdlg.h"
#include "global.h"
#include "mainwidget.h"
#include "message.h"
#include "qtopiaui.h"
#include "signalproxy.h"

#include "ui_aboutdlg.h"

#include <Qtopia>
#include <QSoftMenuBar>

// This constructor is the first thing to be called for a Qtopia app, so we do the init stuff
// here (rather than in a main.cpp).
QtopiaMainWin::QtopiaMainWin(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
  qRegisterMetaType<QVariant>("QVariant");
  qRegisterMetaType<Message>("Message");
  qRegisterMetaType<BufferInfo>("BufferInfo");
  qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
  qRegisterMetaTypeStreamOperators<Message>("Message");
  qRegisterMetaTypeStreamOperators<BufferInfo>("BufferInfo");

  Global::runMode = Global::ClientOnly;

  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel IRC Team");

  QtopiaUi *gui = new QtopiaUi(this);
  Client::init(gui);

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  mainWidget = new MainWidget(this);
  setCentralWidget(mainWidget);

  NetworkModel *model = Client::networkModel();
  connect(model, SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));

  toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setWindowTitle(tr("Show Toolbar"));
  addToolBar(toolBar);

  bufferViewWidget = new BufferViewWidget(this);
  nickListWidget = new NickListWidget(this);

  setupActions();

  init();
  //gui->init();

}

// at this point, client is fully initialized
void QtopiaMainWin::init() {
  Client::signalProxy()->attachSignal(this, SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)));

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
    emit requestBacklog(id, 100, -1);
  }

#ifdef DEVELMODE
  // FIXME just for testing: select first available buffer
  if(Client::allBufferInfos().count() > 1) {
    Buffer *b = Client::buffer(Client::allBufferInfos()[1]);
    Client::networkModel()->selectBuffer(b);
  }
#endif
}

void QtopiaMainWin::disconnectedFromCore() {


}

AbstractUiMsg *QtopiaMainWin::layoutMsg(const Message &msg) {
  return new ChatLine(msg);
  //return 0;
}

void QtopiaMainWin::showBuffer(Buffer *b) {
  bufferViewWidget->hide();
  mainWidget->setBuffer(b);
  nickListWidget->setBuffer(b);
  showNicksAction->setEnabled(b && b->bufferType() == Buffer::ChannelType);

}

void QtopiaMainWin::showBufferView() {
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

