/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include <QDateTime>
#include <QHostAddress>
#include <QSslCipher>
#include <QSslSocket>

#include "sslinfodlg.h"
#include "util.h"

// ========================================
//  SslInfoDlg
// ========================================

SslInfoDlg::SslInfoDlg(const QSslSocket *socket, QWidget *parent)
    : QDialog(parent),
    _socket(socket)
{
    ui.setupUi(this);

    QSslCipher cipher = socket->sessionCipher();

    ui.hostname->setText(socket->peerName());
    ui.address->setText(socket->peerAddress().toString());
    ui.encryption->setText(cipher.name());
    ui.protocol->setText(cipher.protocolString());

    connect(ui.certificateChain, SIGNAL(currentIndexChanged(int)), SLOT(setCurrentCert(int)));
    foreach(const QSslCertificate &cert, socket->peerCertificateChain()) {
        ui.certificateChain->addItem(subjectInfo(cert, QSslCertificate::CommonName));
    }
}


void SslInfoDlg::setCurrentCert(int index)
{
    QSslCertificate cert = socket()->peerCertificateChain().at(index);
    ui.subjectCommonName->setText(subjectInfo(cert, QSslCertificate::CommonName));
    ui.subjectOrganization->setText(subjectInfo(cert, QSslCertificate::Organization));
    ui.subjectOrganizationalUnit->setText(subjectInfo(cert, QSslCertificate::OrganizationalUnitName));
    ui.subjectCountry->setText(subjectInfo(cert, QSslCertificate::CountryName));
    ui.subjectState->setText(subjectInfo(cert, QSslCertificate::StateOrProvinceName));
    ui.subjectCity->setText(subjectInfo(cert, QSslCertificate::LocalityName));

    ui.issuerCommonName->setText(issuerInfo(cert, QSslCertificate::CommonName));
    ui.issuerOrganization->setText(issuerInfo(cert, QSslCertificate::Organization));
    ui.issuerOrganizationalUnit->setText(issuerInfo(cert, QSslCertificate::OrganizationalUnitName));
    ui.issuerCountry->setText(issuerInfo(cert, QSslCertificate::CountryName));
    ui.issuerState->setText(issuerInfo(cert, QSslCertificate::StateOrProvinceName));
    ui.issuerCity->setText(issuerInfo(cert, QSslCertificate::LocalityName));

    if (socket()->sslErrors().isEmpty())
        ui.trusted->setText(tr("Yes"));
    else {
        QString errorString = tr("No, for the following reasons:<ul>");
        foreach(const QSslError &error, socket()->sslErrors())
        errorString += "<li>" + error.errorString() + "</li>";
        errorString += "</ul>";
        ui.trusted->setText(errorString);
    }

    ui.validity->setText(tr("%1 to %2").arg(cert.effectiveDate().date().toString(Qt::ISODate), cert.expiryDate().date().toString(Qt::ISODate)));
    ui.md5Digest->setText(prettyDigest(cert.digest(QCryptographicHash::Md5)));
    ui.sha1Digest->setText(prettyDigest(cert.digest(QCryptographicHash::Sha1)));
#if QT_VERSION < 0x050000
    // Qt 4 doesn't include SHA-2 hashes, so hide the row
    ui.sha256Label->hide();
    ui.sha256Digest->hide();
#else
    ui.sha256Digest->setText(prettyDigest(cert.digest(QCryptographicHash::Sha256)));
#endif
}

// in Qt5, subjectInfo returns a QStringList(); turn this into a comma-separated string instead
QString SslInfoDlg::subjectInfo(const QSslCertificate &cert, QSslCertificate::SubjectInfo subjectInfo) const
{
#if QT_VERSION < 0x050000
    return cert.subjectInfo(subjectInfo);
#else
    return cert.subjectInfo(subjectInfo).join(", ");
#endif
}


// same here
QString SslInfoDlg::issuerInfo(const QSslCertificate &cert, QSslCertificate::SubjectInfo subjectInfo) const
{
#if QT_VERSION < 0x050000
    return cert.issuerInfo(subjectInfo);
#else
    return cert.issuerInfo(subjectInfo).join(", ");
#endif
}
