/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#ifndef SSLINFODLG_H
#define SSLINFODLG_H

#include <QDialog>
#include <QSslCertificate>

#include "ui_sslinfodlg.h"

class QSslSocket;

// ========================================
//  SslInfoDialog
// ========================================

class SslInfoDlg : public QDialog
{
    Q_OBJECT

public:
    SslInfoDlg(const QSslSocket* socket, QWidget* parent = nullptr);
    inline const QSslSocket* socket() const { return _socket; }

private slots:
    void setCurrentCert(int index);

private:
    // simplify handling the API changes between Qt4 and Qt5 (QString -> QStringList)
    QString subjectInfo(const QSslCertificate& cert, QSslCertificate::SubjectInfo subjectInfo) const;
    QString issuerInfo(const QSslCertificate& cert, QSslCertificate::SubjectInfo subjectInfo) const;

private:
    Ui::SslInfoDlg ui;
    const QSslSocket* _socket;
};

#endif
