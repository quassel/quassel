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

#pragma once

#include <tuple>
#include <vector>

#include <QWizard>
#include <QVariantMap>

#include "ui_coreconfigwizardintropage.h"
#include "ui_coreconfigwizardadminuserpage.h"
#include "ui_coreconfigwizardauthenticationselectionpage.h"
#include "ui_coreconfigwizardstorageselectionpage.h"
#include "ui_coreconfigwizardsyncpage.h"

class CoreConnection;

namespace CoreConfigWizardPages {
class SyncPage;
class SyncRelayPage;
};

class CoreConfigWizard : public QWizard
{
    Q_OBJECT

public:
    enum {
        IntroPage,
        AdminUserPage,
        AuthenticationSelectionPage,
        StorageSelectionPage,
        SyncPage,
        SyncRelayPage,
        StorageDetailsPage,
        ConclusionPage
    };

    CoreConfigWizard(CoreConnection *connection, const QVariantList &backendInfos, const QVariantList &authInfos, QWidget *parent = 0);

    inline CoreConnection *coreConnection() const { return _connection; }

signals:
    void setupCore(const QVariant &setupData);
    void loginToCore(const QString &user, const QString &password, bool rememberPassword);

public slots:
    void loginSuccess();
    void syncFinished();

private slots:
    void prepareCoreSetup(const QString &backend, const QVariantMap &properties, const QString &authenticator, const QVariantMap &authProperties);
    void coreSetupSuccess();
    void coreSetupFailed(const QString &);
    void startOver();

private:
    CoreConfigWizardPages::SyncPage *syncPage;
    CoreConfigWizardPages::SyncRelayPage *syncRelayPage;

    CoreConnection *_connection;
};


namespace CoreConfigWizardPages {
class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);
    int nextId() const;
private:
    Ui::CoreConfigWizardIntroPage ui;
};


class AdminUserPage : public QWizardPage
{
    Q_OBJECT

public:
    AdminUserPage(QWidget *parent = 0);
    int nextId() const;
    bool isComplete() const;
private:
    Ui::CoreConfigWizardAdminUserPage ui;
};

// Authentication selection before storage selection.
class AuthenticationSelectionPage : public QWizardPage
{
    Q_OBJECT
    using FieldInfo = std::tuple<QString, QString, QVariant>;

public:
    AuthenticationSelectionPage(const QVariantList &authInfos, QWidget *parent = 0);
    int nextId() const;
    QString displayName() const;
    QString authenticator() const;
    QVariantMap authProperties() const;

private slots:
    void on_backendList_currentIndexChanged(int index);

private:
    Ui::CoreConfigWizardAuthenticationSelectionPage ui;
    QGroupBox *_fieldBox {nullptr};
    std::vector<QVariantMap> _authProperties;
    std::vector<std::vector<FieldInfo>> _authFields;
};

class StorageSelectionPage : public QWizardPage
{
    Q_OBJECT
    using FieldInfo = std::tuple<QString, QString, QVariant>;

public:
    StorageSelectionPage(const QVariantList &backendInfos, QWidget *parent = 0);
    int nextId() const;
    QString displayName() const;
    QString backend() const;
    QVariantMap backendProperties() const;

private slots:
    void on_backendList_currentIndexChanged(int index);

private:
    Ui::CoreConfigWizardStorageSelectionPage ui;
    QGroupBox *_fieldBox {nullptr};
    std::vector<QVariantMap> _backendProperties;
    std::vector<std::vector<FieldInfo>> _backendFields;
};

class SyncPage : public QWizardPage
{
    Q_OBJECT

public:
    SyncPage(QWidget *parent = 0);
    void initializePage();
    int nextId() const;
    bool isComplete() const;

public slots:
    void setStatus(const QString &status);
    void setError(bool);
    void setComplete(bool);

signals:
    void setupCore(const QString &backend, const QVariantMap &, const QString &authenticator, const QVariantMap &);

private:
    Ui::CoreConfigWizardSyncPage ui;
    bool _complete {false};
    bool _hasError {false};
};


class SyncRelayPage : public QWizardPage
{
    Q_OBJECT

public:
    SyncRelayPage(QWidget *parent = 0);
    int nextId() const;
    enum Mode { Success, Error };

public slots:
    void setMode(Mode);

signals:
    void startOver() const;

private:
    Mode mode;
};
}
