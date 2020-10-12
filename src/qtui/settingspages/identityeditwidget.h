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

#include <QSslCertificate>
#include <QSslKey>

#include "clientidentity.h"

#include "ui_identityeditwidget.h"
#include "ui_nickeditdlg.h"

class IdentityEditWidget : public QWidget
{
    Q_OBJECT

public:
    IdentityEditWidget(QWidget* parent = nullptr);

    enum SslState
    {
        NoSsl,
        UnsecureSsl,
        AllowSsl
    };

    void displayIdentity(CertIdentity* id, CertIdentity* saveId = nullptr);
    void saveToIdentity(CertIdentity* id);

public slots:
    void setSslState(SslState state);
    void showAdvanced(bool advanced);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void requestEditSsl();
    void widgetHasChanged();

private slots:
    void on_addNick_clicked();
    void on_deleteNick_clicked();
    void on_renameNick_clicked();
    void on_nickUp_clicked();
    void on_nickDown_clicked();

    void on_clearOrLoadKeyButton_clicked();
    void on_clearOrLoadCertButton_clicked();
    void setWidgetStates();

    void sslDragEnterEvent(QDragEnterEvent* event);
    void sslDropEvent(QDropEvent* event, bool isCert);

private:
    Ui::IdentityEditWidget ui;
    bool _editSsl;

    QSslKey keyByFilename(const QString& filename);
    void showKeyState(const QSslKey& key);
    QSslCertificate certByFilename(const QString& filename);
    void showCertState(const QSslCertificate& cert);

    bool testHasChanged();
};

class NickEditDlg : public QDialog
{
    Q_OBJECT

public:
    NickEditDlg(const QString& oldnick, QStringList existing = QStringList(), QWidget* parent = nullptr);

    QString nick() const;

private slots:
    void on_nickEdit_textChanged(const QString&);

private:
    Ui::NickEditDlg ui;

    QString oldNick;
    QStringList existing;
};
