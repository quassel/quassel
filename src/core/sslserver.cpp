/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include <QDateTime>
#include <QFile>

#include "logger.h"
#include "quassel.h"

#ifdef HAVE_SSL

SslServer::SslServer(QObject *parent)
    : QTcpServer(parent),
    _isCertValid(false)
{
    static bool sslWarningShown = false;

    QString ssl_cert;
    QString ssl_key;

    if(Quassel::isOptionSet("ssl-cert")) {
        ssl_cert = Quassel::optionValue("ssl-cert");
    } else {
        ssl_cert = Quassel::configDirPath() + "quasselCert.pem";
    }

    if(Quassel::isOptionSet("ssl-key")) {
        ssl_key = Quassel::optionValue("ssl-key");
    } else {
        ssl_key = ssl_cert;
    }

    if (!setCertificate(ssl_cert, ssl_key)) {
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

#if QT_VERSION >= 0x050000
void SslServer::incomingConnection(qintptr socketDescriptor)
#else
void SslServer::incomingConnection(int socketDescriptor)
#endif
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


bool SslServer::setCertificate(const QString &path, const QString &keyPath)
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

    // load key from keyPath if it differs from path, otherwise load key from path
    if(path != keyPath) {
        QFile keyFile(keyPath);
        if(!keyFile.exists()) {
            quWarning() << "SslServer: Key file" << qPrintable(keyPath) << "does not exist";
            return false;
        }

        if (!keyFile.open(QIODevice::ReadOnly)) {
            quWarning()
            << "SslServer: Failed to open key file" << qPrintable(keyPath)
            << "error:" << keyFile.error();
            return false;
        }

        _key = QSslKey(&keyFile, QSsl::Rsa);
        keyFile.close();
    } else {
        _key = QSslKey(&certFile, QSsl::Rsa);
    }

    certFile.close();

    if (_cert.isNull()) {
        quWarning() << "SslServer:" << qPrintable(path) << "contains no certificate data";
        return false;
    }

    // We allow the core to offer SSL anyway, so no "return false" here. Client will warn about the cert being invalid.
    const QDateTime now = QDateTime::currentDateTime();
    if (now < _cert.effectiveDate())
        quWarning() << "SslServer: Certificate won't be valid before" << _cert.effectiveDate().toString();

    else if (now > _cert.expiryDate())
        quWarning() << "SslServer: Certificate expired on" << _cert.expiryDate().toString();

    else { // Qt4's isValid() checks for time range and blacklist; avoid a double warning, hence the else block
#if QT_VERSION < 0x050000
        if (!_cert.isValid())
#else
        if (_cert.isBlacklisted())
#endif
            quWarning() << "SslServer: Certificate blacklisted";
    }
    if (_key.isNull()) {
        quWarning() << "SslServer:" << qPrintable(keyPath) << "contains no key data";
        return false;
    }

    _isCertValid = true;

    return _isCertValid;
}


#endif // HAVE_SSL
