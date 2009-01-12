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

#ifndef SSLSERVER_H
#define SSLSERVER_H

#ifdef HAVE_SSL

#include <QSslCertificate>
#include <QSslKey>
#include <QTcpServer>
#include <QLinkedList>

class SslServer : public QTcpServer {
  Q_OBJECT

public:
  SslServer(QObject *parent = 0);

  virtual inline bool hasPendingConnections() const { return !_pendingConnections.isEmpty(); }
  virtual QTcpSocket *nextPendingConnection();

  virtual inline const QSslCertificate &certificate() const { return _cert; }
  virtual inline const QSslKey &key() const { return _key; }
  virtual inline bool certIsValid() const { return _certIsValid; }

protected:
  virtual void incomingConnection(int socketDescriptor);

private:
  QLinkedList<QTcpSocket *> _pendingConnections;
  QSslCertificate _cert;
  QSslKey _key;
  bool _certIsValid;
};

#endif //HAVE_SSL

#endif //SSLSERVER_H
