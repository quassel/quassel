/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "sslserver.h"

#ifdef HAVE_SSL
#  include <QSslSocket>
#endif

#include <QFile>
#include <QDebug>

#include "logger.h"
#include "util.h"

#ifdef HAVE_SSL

SslServer::SslServer(QObject *parent)
  : QTcpServer(parent)
{
  setCertificate(quasselDir().absolutePath() + "/quasselCert.pem");
}

QTcpSocket *SslServer::nextPendingConnection() {
  if(_pendingConnections.isEmpty())
    return 0;
  else
    return _pendingConnections.takeFirst();
}

void SslServer::incomingConnection(int socketDescriptor) {
  QSslSocket *serverSocket = new QSslSocket(this);
  if(serverSocket->setSocketDescriptor(socketDescriptor)) {
    if(certIsValid()) {
      serverSocket->setLocalCertificate(_cert);
      serverSocket->setPrivateKey(_key);
    }
    _pendingConnections << serverSocket;
    emit newConnection();
  } else {
    delete serverSocket;
  }
}

bool SslServer::setCertificate(const QString &path) {
  _certIsValid = false;

  if (path.isNull()) {
    return false;
  }

  QFile certFile(path);
  if (! certFile.exists()) {
    qWarning() << "SslServer: Certificate file" << qPrintable(path) << "does not exist";
    return false;
  }

  certFile.open(QIODevice::ReadOnly);
  _cert = QSslCertificate(&certFile);

  certFile.reset();
  _key = QSslKey(&certFile, QSsl::Rsa);
  certFile.close();

  if(_cert.isNull() || !_cert.isValid() || _key.isNull()) {
    qWarning() << "SslServer: SSL Certificate is either missing or has a wrong format!\n"
               << "          Quassel Core will still work, but cannot provide SSL for client connections.\n"
               << "          Please see http://quassel-irc.org/faq/cert to learn how to enable SSL support.";
  } else {
    _certIsValid = true;
  }

  return _certIsValid;
}

#endif // HAVE_SSL
