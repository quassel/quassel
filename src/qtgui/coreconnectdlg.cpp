/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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
#include "coreconnectdlg.h"
#include "clientproxy.h"
#include "global.h"
#include "client.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  ui.progressBar->hide();
  coreState = 0;
  QSettings s;
  connect(ui.hostName, SIGNAL(textChanged(QString)), this, SLOT(hostEditChanged(QString)));
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(hostSelected()));

  ui.hostName->setText(s.value("GUI/CoreHost", "localhost").toString());
  ui.hostName->setSelection(0, ui.hostName->text().length());
  ui.hostPort->setValue(s.value("GUI/CorePort", 4242).toInt());
  ui.autoConnect->setChecked(s.value("GUI/CoreAutoConnect", true).toBool());
  if(s.value("GUI/CoreAutoConnect").toBool()) {
    hostSelected();
  }
}

void CoreConnectDlg::setStartState() {
  ui.hostName->show(); ui.hostPort->show(); ui.hostLabel->show(); ui.portLabel->show();
  ui.statusText->setText(tr("Connect to Quassel Core running on:"));
  ui.buttonBox->button(QDialogButtonBox::Ok)->show();
  ui.hostName->setEnabled(true); ui.hostPort->setEnabled(true);
  ui.hostName->setSelection(0, ui.hostName->text().length());
}

void CoreConnectDlg::hostEditChanged(QString txt) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(txt.length());
}

void CoreConnectDlg::hostSelected() {
  ui.hostName->hide(); ui.hostPort->hide(); ui.hostLabel->hide(); ui.portLabel->hide();
  ui.statusText->setText(tr("Connecting to %1:%2" ).arg(ui.hostName->text()).arg(ui.hostPort->value()));
  ui.buttonBox->button(QDialogButtonBox::Ok)->hide();
  connect(ClientProxy::instance(), SIGNAL(coreConnected()), this, SLOT(coreConnected()));
  connect(ClientProxy::instance(), SIGNAL(coreConnectionError(QString)), this, SLOT(coreConnectionError(QString)));
  Client::instance()->connectToCore(ui.hostName->text(), ui.hostPort->value());

}

void CoreConnectDlg::coreConnected() {
  ui.hostLabel->hide(); ui.hostName->hide(); ui.portLabel->hide(); ui.hostPort->hide();
  ui.statusText->setText(tr("Synchronizing..."));
  QSettings s;
  s.setValue("GUI/CoreHost", ui.hostName->text());
  s.setValue("GUI/CorePort", ui.hostPort->value());
  s.setValue("GUI/CoreAutoConnect", ui.autoConnect->isChecked());
  connect(ClientProxy::instance(), SIGNAL(recvPartialItem(quint32, quint32)), this, SLOT(updateProgressBar(quint32, quint32)));
  connect(ClientProxy::instance(), SIGNAL(csCoreState(QVariant)), this, SLOT(recvCoreState(QVariant)));
  ui.progressBar->show();
  VarMap initmsg;
  initmsg["GUIProtocol"] = GUI_PROTOCOL;
  // FIXME guiProxy->send(GS_CLIENT_INIT, QVariant(initmsg));
}

void CoreConnectDlg::coreConnectionError(QString err) {
  QMessageBox::warning(this, tr("Connection Error"), tr("<b>Could not connect to Quassel Core!</b><br>\n") + err, QMessageBox::Retry);
  disconnect(ClientProxy::instance(), 0, this, 0);
  ui.autoConnect->setChecked(false);
  setStartState();
}

void CoreConnectDlg::updateProgressBar(quint32 recv, quint32 avail) {
  ui.progressBar->setMaximum(avail);
  ui.progressBar->setValue(recv);
}

void CoreConnectDlg::recvCoreState(QVariant state) {
  ui.progressBar->hide();
  coreState = state;
  accept();
}

QVariant CoreConnectDlg::getCoreState() {
  return coreState;
}
