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

#include <QDebug>

#include "coreproxy.h"
#include "global.h"
#include "util.h"

CoreProxy::CoreProxy() {
  if(coreProxy) qFatal("Trying to instantiate more than one CoreProxy object!");

  connect(global, SIGNAL(dataPutLocally(QString)), this, SLOT(updateGlobalData(QString)));
  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  if(!server.listen(QHostAddress::Any, 4242)) {
    qFatal(QString(QString("Could not open GUI client port %1: %2").arg(4242).arg(server.errorString())).toAscii());
  }
  qDebug() << "Listening for GUI clients on port" << server.serverPort() << ".";
}

void CoreProxy::incomingConnection() {
  QTcpSocket *socket = server.nextPendingConnection();
  connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
  connect(socket, SIGNAL(readyRead()), this, SLOT(clientHasData()));
  clients.append(socket);
  blockSizes.insert(socket, (quint32)0);
  qDebug() << "Client connected from " << socket->peerAddress().toString();
}

void CoreProxy::clientHasData() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  Q_ASSERT(socket && blockSizes.contains(socket));
  quint32 bsize = blockSizes.value(socket);
  QVariant item;
  while(readDataFromDevice(socket, bsize, item)) {
    QList<QVariant> sigdata = item.toList();
    Q_ASSERT(sigdata.size() == 4);
    switch((GUISignal)sigdata[0].toInt()) {
      case GS_CLIENT_INIT:  processClientInit(socket, sigdata[1]); break;
      case GS_UPDATE_GLOBAL_DATA: processClientUpdate(socket, sigdata[1].toString(), sigdata[2]); break;
      //case GS_CLIENT_READY: processClientReady(sigdata[1], sigdata[2], sigdata[3]); break;
      default: recv((GUISignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]); break;
    }
    blockSizes[socket] = bsize = 0;
  }
  blockSizes[socket] = bsize;
}

void CoreProxy::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  blockSizes.remove(socket);
  clients.removeAll(socket);
  qDebug() << "Client disconnected.";
}

void CoreProxy::processClientInit(QTcpSocket *socket, const QVariant &v) {
  VarMap msg = v.toMap();
  if(msg["GUIProtocol"].toUInt() != GUI_PROTOCOL) {
    qDebug() << "Client version mismatch. Disconnecting.";
    socket->close();
    return;
  }
  VarMap reply;
  VarMap coreData;
  QStringList dataKeys = global->getKeys();
  QString key;
  foreach(key, dataKeys) {
    coreData[key] = global->getData(key);
  }
  reply["CoreData"] = coreData;
  //QVariant payload = QByteArray(1000000, 'a');
  //reply["payload"] = payload;
  QList<QVariant> sigdata;
  sigdata.append(CS_CORE_STATE); sigdata.append(QVariant(reply)); sigdata.append(QVariant()); sigdata.append(QVariant());
  writeDataToDevice(socket, QVariant(sigdata));
}

void CoreProxy::processClientUpdate(QTcpSocket *socket, QString key, QVariant data) {
  global->updateData(key, data);
  QList<QVariant> sigdata;
  sigdata.append(CS_UPDATE_GLOBAL_DATA); sigdata.append(key); sigdata.append(data); sigdata.append(QVariant());
  QTcpSocket *s;
  foreach(s, clients) {
    if(s != socket) writeDataToDevice(s, QVariant(sigdata));
  }
}

void CoreProxy::updateGlobalData(QString key) {
  QVariant data = global->getData(key);
  emit csUpdateGlobalData(key, data);
}

void CoreProxy::send(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  sendToGUI(sig, arg1, arg2, arg3);
  QList<QVariant> sigdata;
  sigdata.append(sig); sigdata.append(arg1); sigdata.append(arg2); sigdata.append(arg3);
  //qDebug() << "Sending signal: " << sigdata;
  QTcpSocket *socket;
  foreach(socket, clients) {
    writeDataToDevice(socket, QVariant(sigdata));
  }
}

void CoreProxy::recv(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  //qDebug() << "[CORE] Received signal" << sig << ":" << arg1<<arg2<<arg3;
  switch(sig) {
    case GS_UPDATE_GLOBAL_DATA: emit gsPutGlobalData(arg1.toString(), arg2); break;
    case GS_USER_INPUT: emit gsUserInput(arg1.toString()); break;
    case GS_REQUEST_CONNECT: emit gsRequestConnect(arg1.toStringList()); break;
    default: qWarning() << "Unknown signal in CoreProxy::recv: " << sig;
  }
}

CoreProxy *coreProxy;
