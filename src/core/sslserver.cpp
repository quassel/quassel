/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
    // Keep track if the SSL warning has been mentioned at least once before
    static bool sslWarningShown = false;

    if(Quassel::isOptionSet("ssl-cert")) {
        _sslCertPath = Quassel::optionValue("ssl-cert");
    } else {
        _sslCertPath = Quassel::configDirPath() + "quasselCert.pem";
    }

    if(Quassel::isOptionSet("ssl-key")) {
        _sslKeyPath = Quassel::optionValue("ssl-key");
    } else {
        _sslKeyPath = _sslCertPath;
    }

    // Initialize the certificates for first-time usage
    if (!loadCerts()) {
        if (!sslWarningShown) {
            quWarning()
            << "SslServer: Unable to set certificate file\n"
            << "          Quassel Core will still work, but cannot provide SSL for client connections.\n"
            << "          Please see https://quassel-irc.org/faq/cert to learn how to enable SSL support.";
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


bool SslServer::loadCerts()
{
    // Load the certificates specified in the path.  If needed, other prep work can be done here.
    return setCertificate(_sslCertPath, _sslKeyPath);
}


bool SslServer::reloadCerts()
{
    if (loadCerts()) {
        return true;
    } else {
        // Reloading certificates currently only occur in response to a request.  Always print an
        // error if something goes wrong, in order to simplify checking if it's working.
        if (isCertValid()) {
            quWarning()
            << "SslServer: Unable to reload certificate file, reverting\n"
            << "          Quassel Core will use the previous key to provide SSL for client connections.\n"
            << "          Please see https://quassel-irc.org/faq/cert to learn how to enable SSL support.";
        } else {
            quWarning()
            << "SslServer: Unable to reload certificate file\n"
            << "          Quassel Core will still work, but cannot provide SSL for client connections.\n"
            << "          Please see https://quassel-irc.org/faq/cert to learn how to enable SSL support.";
        }
        return false;
    }
}


bool SslServer::setCertificate(const QString &path, const QString &keyPath)
{
    // Don't reset _isCertValid here, in case an older but valid certificate is still loaded.
    // Use temporary variables in order to avoid overwriting the existing certificates until
    // everything is confirmed good.
    QSslCertificate untestedCert;
    QList<QSslCertificate> untestedCA;
    QSslKey untestedKey;

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

    untestedCert = certList[0];
    certList.removeFirst(); // remove server cert

    // store CA and intermediates certs
    untestedCA = certList;

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

        untestedKey = QSslKey(&keyFile, QSsl::Rsa);
        keyFile.close();
    } else {
        untestedKey = QSslKey(&certFile, QSsl::Rsa);
    }

    certFile.close();

    if (untestedCert.isNull()) {
        quWarning() << "SslServer:" << qPrintable(path) << "contains no certificate data";
        return false;
    }

    // We allow the core to offer SSL anyway, so no "return false" here. Client will warn about the cert being invalid.
    const QDateTime now = QDateTime::currentDateTime();
    if (now < untestedCert.effectiveDate())
        quWarning() << "SslServer: Certificate won't be valid before" << untestedCert.effectiveDate().toString();

    else if (now > untestedCert.expiryDate())
        quWarning() << "SslServer: Certificate expired on" << untestedCert.expiryDate().toString();

    else { // Qt4's isValid() checks for time range and blacklist; avoid a double warning, hence the else block
#if QT_VERSION < 0x050000
        if (!untestedCert.isValid())
#else
        if (untestedCert.isBlacklisted())
#endif
            quWarning() << "SslServer: Certificate blacklisted";
    }
    if (untestedKey.isNull()) {
        quWarning() << "SslServer:" << qPrintable(keyPath) << "contains no key data";
        return false;
    }

    _isCertValid = true;

    // All keys are valid, update the externally visible copy used for new connections.
    _cert = untestedCert;
    _ca = untestedCA;
    _key = untestedKey;

    return _isCertValid;
}


#endif // HAVE_SSL
