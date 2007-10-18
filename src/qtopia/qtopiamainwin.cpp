/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#include "qtopiamainwin.h"

#include "buffertreemodel.h"
#include "chatline.h"
#include "coreconnectdlg.h"
#include "global.h"
#include "mainwidget.h"
#include "message.h"
#include "qtopiagui.h"
#include "signalproxy.h"

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
  QCoreApplication::setOrganizationName("Quassel IRC Development Team");

  //Style::init();
  QtopiaGui *gui = new QtopiaGui(this);
  Client::init(gui);

  setWindowTitle("Quassel IRC");
  setWindowIcon(QIcon(":/qirc-icon.png"));
  setWindowIconText("Quassel IRC");

  mainWidget = new MainWidget(this);
  setCentralWidget(mainWidget);

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16, 16));
  toolBar->addAction(QIcon(":icon/trash"), "Trash");
  addToolBar(toolBar);

  init();
  //gui->init();

}

// at this point, client is fully initialized
void QtopiaMainWin::init() {
  Client::signalProxy()->attachSignal(this, SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)));
  connect(Client::bufferModel(), SIGNAL(bufferSelected(Buffer *)), this, SLOT(showBuffer(Buffer *)));

  CoreConnectDlg *dlg = new CoreConnectDlg(this);
  //setCentralWidget(dlg);
  dlg->showMaximized();
  dlg->exec();
}

QtopiaMainWin::~QtopiaMainWin() {


}

void QtopiaMainWin::connectedToCore() {
  foreach(BufferInfo id, Client::allBufferInfos()) {
    emit requestBacklog(id, 100, -1);
  }
  // FIXME just for testing: select first available buffer
  if(Client::allBufferInfos().count()) {
    Buffer *b = Client::buffer(Client::allBufferInfos()[0]);
    Client::bufferModel()->selectBuffer(b);
  }
}

void QtopiaMainWin::disconnectedFromCore() {


}

AbstractUiMsg *QtopiaMainWin::layoutMsg(const Message &msg) {
  return new ChatLine(msg);
  //return 0;
}

void QtopiaMainWin::showBuffer(Buffer *b) {
  mainWidget->setBuffer(b);

}
