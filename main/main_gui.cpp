/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
 *   devel@quassel-irc.org                                                 *
 *                                                                          *
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

#include <iostream>

#include <QApplication>

#include "quassel.h"
#include "guiproxy.h"
#include "util.h"

#include "mainwin.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QApplication::setOrganizationDomain("quassel-irc.org");
  QApplication::setApplicationName("Quassel IRC");
  QApplication::setOrganizationName("The Quassel Team");

  Global::runMode = Global::GUIOnly;
  global = new Global();
  guiProxy = new GUIProxy();

  MainWin mainWin;
  int exitCode = app.exec();
  delete guiProxy;
  delete global;
}

GUIProxy::GUIProxy() {
  if(guiProxy) qFatal("Trying to instantiate more than one CoreProxy object!");

  blockSize = 0;

  connect(&socket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
  connect(&socket, SIGNAL(connected()), this, SIGNAL(coreConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SIGNAL(coreDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(serverError(QAbstractSocket::SocketError)));

  connect(global, SIGNAL(dataPutLocally(QString)), this, SLOT(updateCoreData(QString)));
  connect(this, SIGNAL(csUpdateGlobalData(QString, QVariant)), global, SLOT(updateData(QString, QVariant)));

}

void GUIProxy::connectToCore(QString host, quint16 port) {
  socket.connectToHost(host, port);
  //VarMap initmsg;
  //initmsg["GUIProtocol"] = GUI_PROTOCOL;
  //send(GS_CLIENT_INIT, QVariant(initmsg));
}

void GUIProxy::disconnectFromCore() {
  socket.close();
}

void GUIProxy::serverError(QAbstractSocket::SocketError) {
  emit coreConnectionError(socket.errorString());
}

void GUIProxy::serverHasData() {
  QVariant item;
  while(readDataFromDevice(&socket, blockSize, item)) {
    emit recvPartialItem(1,1);
    QList<QVariant> sigdata = item.toList();
    Q_ASSERT(sigdata.size() == 4);
    recv((CoreSignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]);
    blockSize = 0;
  }
  if(blockSize > 0) {
    emit recvPartialItem(socket.bytesAvailable(), blockSize);
  }
}

void GUIProxy::send(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  QList<QVariant> sigdata;
  sigdata.append(sig); sigdata.append(arg1); sigdata.append(arg2); sigdata.append(arg3);
  //qDebug() << "Sending signal: " << sigdata;
  writeDataToDevice(&socket, QVariant(sigdata));
}

void GUIProxy::updateCoreData(QString key) {
  QVariant data = global->getData(key);
  send(GS_UPDATE_GLOBAL_DATA, key, data);
}
