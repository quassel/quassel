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

#ifndef IDENTITYEDITWIDGET_H
#define IDENTITYEDITWIDGET_H

#include "ui_identityeditwidget.h"
#include "ui_nickeditdlg.h"

#ifdef HAVE_SSL
#include <QSslCertificate>
#include <QSslKey>
#endif

#include "clientidentity.h"

class IdentityEditWidget : public QWidget
{
    Q_OBJECT

public:
    IdentityEditWidget(QWidget *parent = 0);

    enum SslState {
        NoSsl,
        UnsecureSsl,
        AllowSsl
    };

    void displayIdentity(CertIdentity *id, CertIdentity *saveId = 0);
    void saveToIdentity(CertIdentity *id);

public slots:
    void setSslState(SslState state);
    void showAdvanced(bool advanced);

protected:
#ifdef HAVE_SSL
    virtual bool eventFilter(QObject *watched, QEvent *event);
#endif

signals:
    void requestEditSsl();
    void widgetHasChanged();

private slots:
    void on_addNick_clicked();
    void on_deleteNick_clicked();
    void on_renameNick_clicked();
    void on_nickUp_clicked();
    void on_nickDown_clicked();

#ifdef HAVE_SSL
    void on_clearOrLoadKeyButton_clicked();
    void on_clearOrLoadCertButton_clicked();
#endif
    void setWidgetStates();

#ifdef HAVE_SSL
    void sslDragEnterEvent(QDragEnterEvent *event);
    void sslDropEvent(QDropEvent *event, bool isCert);
#endif

private:
    Ui::IdentityEditWidget ui;
    bool _editSsl;

#ifdef HAVE_SSL
    QSslKey keyByFilename(const QString &filename);
    void showKeyState(const QSslKey &key);
    QSslCertificate certByFilename(const QString &filename);
    void showCertState(const QSslCertificate &cert);
#endif

    bool testHasChanged();
};


class NickEditDlg : public QDialog
{
    Q_OBJECT

public:
    NickEditDlg(const QString &oldnick, const QStringList &existing = QStringList(), QWidget *parent = 0);

    QString nick() const;

private slots:
    void on_nickEdit_textChanged(const QString &);

private:
    Ui::NickEditDlg ui;

    QString oldNick;
    QStringList existing;
};


#endif //IDENTITYEDITWIDGET_H
