/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "sslserver.h"

#ifdef HAVE_SSL
#  include <QSslSocket>
#endif

#include <QFile>

#include "logger.h"
#include "quassel.h"

#ifdef HAVE_SSL

SslServer::SslServer(QObject *parent)
    : QTcpServer(parent),
    _isCertValid(false)
{
    static bool sslWarningShown = false;
    if (!setCertificate(Quassel::configDirPath() + "quasselCert.pem")) {
        if (!sslWarningShown) {
            quWarning()
            << "SslServer: Unable to set certificate file\n"
            << "          Quassel Core will still work, but cannot provide SSL for client connections.\n"
            << "          Please see http://quassel-irc.org/faq/cert to learn how to enable SSL support.";
            sslWarningShown = true;
        }
    }
}


QTcpSocket *SslServer::nextPendingConnection()
{
    if (_pendingConnections.isEmpty())
        return 0;
    else
        return _pendingConnections.takeFirst();
}


void SslServer::incomingConnection(int socketDescriptor)
{
    QSslSocket *serverSocket = new QSslSocket(this);
    if (serverSocket->setSocketDescriptor(socketDescriptor)) {
        if (isCertValid()) {
            serverSocket->setLocalCertificate(_cert);
            serverSocket->setPrivateKey(_key);
            serverSocket->addCaCertificates(_ca);
        }
        _pendingConnections << serverSocket;
        emit newConnection();
    }
    else {
        delete serverSocket;
    }
}


bool SslServer::setCertificate(const QString &path)
{
    _isCertValid = false;

    if (path.isEmpty())
        return false;

    QFile certFile(path);
    if (!certFile.exists()) {
        quWarning() << "SslServer: Certificate file" << qPrintable(path) << "does not exist";
        return false;
    }

    if (!certFile.open(QIODevice::ReadOnly)) {
        quWarning()
        << "SslServer: Failed to open certificate file" << qPrintable(path)
        << "error:" << certFile.error();
        return false;
    }

    QList<QSslCertificate> certList = QSslCertificate::fromDevice(&certFile);

    if (certList.isEmpty()) {
        quWarning() << "SslServer: Certificate file doesn't contain a certificate";
        return false;
    }

    _cert = certList[0];
    certList.removeFirst(); // remove server cert

    // store CA and intermediates certs
    _ca = certList;

    if (!certFile.reset()) {
        quWarning() << "SslServer: IO error reading certificate file";
        return false;
    }

    _key = QSslKey(&certFile, QSsl::Rsa);
    certFile.close();

    if (_cert.isNull()) {
        quWarning() << "SslServer:" << qPrintable(path) << "contains no certificate data";
        return false;
    }
    if (!_cert.isValid()) {
        quWarning() << "SslServer: Invalid certificate (most likely expired)";
    }
    if (_key.isNull()) {
        quWarning() << "SslServer:" << qPrintable(path) << "contains no key data";
        return false;
    }

    _isCertValid = true;

    return _isCertValid;
}


#endif // HAVE_SSL
