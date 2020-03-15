/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#pragma once

#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QTcpServer>

#include "metricsserver.h"

class SslServer : public QTcpServer
{
    Q_OBJECT

public:
    SslServer(QObject* parent = nullptr);

    const QSslCertificate& certificate() const { return _cert; }
    const QSslKey& key() const { return _key; }
    bool isCertValid() const { return _isCertValid; }

    /**
     * Reloads SSL certificates used for connections
     *
     * If this command fails, it will try to maintain the most recent working certificate.  Error
     * conditions are automatically written to the log.
     *
     * @return True if certificates reloaded successfully, otherwise false.
     */
    bool reloadCerts();

    void setMetricsServer(MetricsServer* metricsServer);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

    bool setCertificate(const QString& path, const QString& keyPath);

private:
    /**
     * Loads SSL certificates used for connections
     *
     * If this command fails, it will try to maintain the most recent working certificate.  Will log
     * specific failure points, but does not offer verbose guidance.
     *
     * @return True if certificates loaded successfully, otherwise false.
     */
    bool loadCerts();
    QSslKey loadKey(QFile* keyFile);

    MetricsServer* _metricsServer{nullptr};

    QSslCertificate _cert;
    QSslKey _key;
    QList<QSslCertificate> _ca;
    bool _isCertValid{false};

    // Used when reloading certificates later
    QString _sslCertPath;  /// Path to the certificate file
    QString _sslKeyPath;   /// Path to the private key file (may be in same file as above)

    QDateTime _certificateExpires;
};
