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
        ui.certificateChain->addItem(cert.subjectInfo(QSslCertificate::CommonName));
    }
}


void SslInfoDlg::setCurrentCert(int index)
{
    QSslCertificate cert = socket()->peerCertificateChain().at(index);
    ui.subjectCommonName->setText(cert.subjectInfo(QSslCertificate::CommonName));
    ui.subjectOrganization->setText(cert.subjectInfo(QSslCertificate::Organization));
    ui.subjectOrganizationalUnit->setText(cert.subjectInfo(QSslCertificate::OrganizationalUnitName));
    ui.subjectCountry->setText(cert.subjectInfo(QSslCertificate::CountryName));
    ui.subjectState->setText(cert.subjectInfo(QSslCertificate::StateOrProvinceName));
    ui.subjectCity->setText(cert.subjectInfo(QSslCertificate::LocalityName));

    ui.issuerCommonName->setText(cert.issuerInfo(QSslCertificate::CommonName));
    ui.issuerOrganization->setText(cert.issuerInfo(QSslCertificate::Organization));
    ui.issuerOrganizationalUnit->setText(cert.issuerInfo(QSslCertificate::OrganizationalUnitName));
    ui.issuerCountry->setText(cert.issuerInfo(QSslCertificate::CountryName));
    ui.issuerState->setText(cert.issuerInfo(QSslCertificate::StateOrProvinceName));
    ui.issuerCity->setText(cert.issuerInfo(QSslCertificate::LocalityName));

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
}
