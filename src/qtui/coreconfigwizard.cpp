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

#include <QDebug>
#include <QAbstractButton>
#include <QFormLayout>
#include <QIcon>
#include <QSpinBox>

#include "coreconfigwizard.h"
#include "coreconnection.h"

#include "client.h"

namespace {

template<typename FieldInfo>
void createFieldWidgets(QGroupBox *fieldBox, const std::vector<FieldInfo> &fieldInfos)
{
    // Create a config UI based on the field types sent from the backend
    // We make some assumptions here (like integer range and password field names) that may not
    // hold true for future authenticator types - but the only way around it for now would be to
    // provide specialized config widgets for those (which may be a good idea anyway, e.g. if we
    // think about client-side translations...)

    QFormLayout *formLayout = new QFormLayout;
    for (auto &&fieldInfo : fieldInfos) {
        QWidget *widget {nullptr};
        switch (std::get<2>(fieldInfo).type()) {
            case QVariant::Int:
                widget = new QSpinBox(fieldBox);
                // Here we assume that int fields are always in 16 bit range, like ports
                static_cast<QSpinBox *>(widget)->setMinimum(0);
                static_cast<QSpinBox *>(widget)->setMaximum(65535);
                static_cast<QSpinBox *>(widget)->setValue(std::get<2>(fieldInfo).toInt());
                break;
            case QVariant::String:
                widget = new QLineEdit(std::get<2>(fieldInfo).toString(), fieldBox);
                // Here we assume that fields named something with "password" are actual password inputs
                if (std::get<0>(fieldInfo).toLower().contains("password"))
                    static_cast<QLineEdit *>(widget)->setEchoMode(QLineEdit::Password);
                break;
            default:
                qWarning() << "Unsupported type for backend property" << std::get<0>(fieldInfo);
        }
        if (widget) {
            widget->setObjectName(std::get<0>(fieldInfo));
            formLayout->addRow(std::get<1>(fieldInfo) + ":", widget);
        }
    }
    fieldBox->setLayout(formLayout);
}


template<typename FieldInfo>
QVariantMap propertiesFromFieldWidgets(QGroupBox *fieldBox, const std::vector<FieldInfo> &fieldInfos)
{
    QVariantMap properties;
    if (!fieldBox)
        return properties;

    for (auto &&fieldInfo : fieldInfos) {
        QString key = std::get<0>(fieldInfo);
        QVariant value;
        switch (std::get<2>(fieldInfo).type()) {
            case QVariant::Int: {
                QSpinBox *spinBox = fieldBox->findChild<QSpinBox *>(key);
                if (spinBox)
                    value = spinBox->value();
                else
                    qWarning() << "Could not find child widget for field" << key;
                break;
            }
            case QVariant::String: {
                QLineEdit *lineEdit = fieldBox->findChild<QLineEdit *>(key);
                if (lineEdit)
                    value = lineEdit->text();
                else
                    qWarning() << "Could not find child widget for field" << key;
                break;
            }
            default:
                qWarning() << "Unsupported type for backend property" << key;
        }
        properties[key] = std::move(value);
    }
    return properties;
}

} // anon


CoreConfigWizard::CoreConfigWizard(CoreConnection *connection, const QVariantList &backendInfos, const QVariantList &authInfos, QWidget *parent)
    : QWizard(parent),
    _connection{connection}
{
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose);

    setPage(IntroPage, new CoreConfigWizardPages::IntroPage(this));
    setPage(AdminUserPage, new CoreConfigWizardPages::AdminUserPage(this));
    setPage(AuthenticationSelectionPage, new CoreConfigWizardPages::AuthenticationSelectionPage(authInfos, this));
    setPage(StorageSelectionPage, new CoreConfigWizardPages::StorageSelectionPage(backendInfos, this));
    syncPage = new CoreConfigWizardPages::SyncPage(this);
    connect(syncPage, SIGNAL(setupCore(const QString &, const QVariantMap &, const QString &, const QVariantMap &)),
            SLOT(prepareCoreSetup(const QString &, const QVariantMap &, const QString &, const QVariantMap &)));
    setPage(SyncPage, syncPage);
    syncRelayPage = new CoreConfigWizardPages::SyncRelayPage(this);
    connect(syncRelayPage, SIGNAL(startOver()), this, SLOT(startOver()));
    setPage(SyncRelayPage, syncRelayPage);
    //setPage(Page_StorageDetails, new StorageDetailsPage());
    //setPage(Page_Conclusion, new ConclusionPage(storageProviders));

    setStartId(IntroPage);
    //setStartId(StorageSelectionPage);

#ifndef Q_OS_MAC
    setWizardStyle(ModernStyle);
#endif

    setOption(HaveHelpButton, false);
    setOption(NoBackButtonOnStartPage, true);
    setOption(HaveNextButtonOnLastPage, false);
    setOption(HaveFinishButtonOnEarlyPages, false);
    setOption(NoCancelButton, true);
    setOption(IndependentPages, true);
    //setOption(ExtendedWatermarkPixmap, true);

    setModal(true);

    setWindowTitle(tr("Core Configuration Wizard"));
    setPixmap(QWizard::LogoPixmap, QIcon::fromTheme("quassel", QIcon(":/icons/quassel.png")).pixmap(48));

    connect(connection, SIGNAL(coreSetupSuccess()), SLOT(coreSetupSuccess()));
    connect(connection, SIGNAL(coreSetupFailed(QString)), SLOT(coreSetupFailed(QString)));
    //connect(connection, SIGNAL(loginSuccess()), SLOT(loginSuccess()));
    connect(connection, SIGNAL(synchronized()), SLOT(syncFinished()));
    connect(this, SIGNAL(rejected()), connection, SLOT(disconnectFromCore()));
}


void CoreConfigWizard::prepareCoreSetup(const QString &backend, const QVariantMap &properties, const QString &authenticator, const QVariantMap &authProperties)
{
    // Prevent the user from changing any settings he already specified...
    foreach(int idx, visitedPages())
    page(idx)->setEnabled(false);

    // FIXME? We need to be able to set up older cores that don't have auth backend support.
    // So if the core doesn't support that feature, don't pass those parameters.
    if (!(Client::coreFeatures() & Quassel::Authenticators)) {
        coreConnection()->setupCore(Protocol::SetupData(field("adminUser.user").toString(), field("adminUser.password").toString(), backend, properties));
    }
    else {
        coreConnection()->setupCore(Protocol::SetupData(field("adminUser.user").toString(), field("adminUser.password").toString(), backend, properties, authenticator, authProperties));
    }
}


void CoreConfigWizard::coreSetupSuccess()
{
    syncPage->setStatus(tr("Your core has been successfully configured. Logging you in..."));
    syncPage->setError(false);
    syncRelayPage->setMode(CoreConfigWizardPages::SyncRelayPage::Success);
    coreConnection()->loginToCore(field("adminUser.user").toString(), field("adminUser.password").toString(), field("adminUser.rememberPasswd").toBool());
}


void CoreConfigWizard::coreSetupFailed(const QString &error)
{
    syncPage->setStatus(tr("Core configuration failed:<br><b>%1</b><br>Press <em>Next</em> to start over.").arg(error));
    syncPage->setError(true);
    syncRelayPage->setMode(CoreConfigWizardPages::SyncRelayPage::Error);
    //foreach(int idx, visitedPages()) page(idx)->setEnabled(true);
    //setStartId(SyncPage);
    //restart();
}


void CoreConfigWizard::startOver()
{
    foreach(int idx, visitedPages()) page(idx)->setEnabled(true);
    setStartId(CoreConfigWizard::AdminUserPage);
    restart();
}


void CoreConfigWizard::loginSuccess()
{
    syncPage->setStatus(tr("Your are now logged into your freshly configured Quassel Core!<br>"
                           "Please remember to configure your identities and networks now."));
    syncPage->setComplete(true);
    syncPage->setFinalPage(true);
}


void CoreConfigWizard::syncFinished()
{
    accept();
}


namespace CoreConfigWizardPages {
/*** Intro Page ***/

IntroPage::IntroPage(QWidget *parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setTitle(tr("Introduction"));
    //setSubTitle(tr("foobar"));
    //setPixmap(QWizard::WatermarkPixmap, QPixmap(":icons/quassel-icon.png"));
}


int IntroPage::nextId() const
{
    return CoreConfigWizard::AdminUserPage;
}


/*** Admin User Page ***/

AdminUserPage::AdminUserPage(QWidget *parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setTitle(tr("Create Admin User"));
    setSubTitle(tr("First, we will create a user on the core. This first user will have administrator privileges."));

    registerField("adminUser.user*", ui.user);
    registerField("adminUser.password*", ui.password);
    registerField("adminUser.password2*", ui.password2);
    registerField("adminUser.rememberPasswd", ui.rememberPasswd);
}


int AdminUserPage::nextId() const
{
    // If the core doesn't support auth backends, skip that page!
    if (!(Client::coreFeatures() & Quassel::Authenticators)) {
        return CoreConfigWizard::StorageSelectionPage;
    }
    else {
        return CoreConfigWizard::AuthenticationSelectionPage;
    }
}


bool AdminUserPage::isComplete() const
{
    bool ok = !ui.user->text().isEmpty() && !ui.password->text().isEmpty() && ui.password->text() == ui.password2->text();
    return ok;
}

/*** Authentication Selection Page ***/

AuthenticationSelectionPage::AuthenticationSelectionPage(const QVariantList &authInfos, QWidget *parent)
    : QWizardPage(parent)
{
    ui.setupUi(this);

    setTitle(tr("Select Authentication Backend"));
    setSubTitle(tr("Please select a backend for Quassel Core to use for authenticating users."));

    registerField("authentication.backend", ui.backendList);

    for (auto &&authInfo : authInfos) {
        auto props = authInfo.toMap();
        // Extract field infos to avoid having to reparse the list
        std::vector<FieldInfo> fields;
        const auto &list = props["SetupData"].toList();
        for (int i = 0; i + 2 < list.size(); i += 3) {
            fields.emplace_back(std::make_tuple(list[i].toString(), list[i+1].toString(), list[i+2]));
        }
        props.remove("SetupData");

        _authProperties.emplace_back(props);
        _authFields.emplace_back(std::move(fields));

        // Create entry in authenticator selector
        ui.backendList->addItem(props["DisplayName"].toString(), props["BackendId"].toString());
    }

    ui.backendList->setCurrentIndex(0);
}


int AuthenticationSelectionPage::nextId() const
{
    return CoreConfigWizard::StorageSelectionPage;
}


QString AuthenticationSelectionPage::displayName() const
{
    return ui.backendList->currentText();
}


QString AuthenticationSelectionPage::authenticator() const
{
#if QT_VERSION >= 0x050200
    return ui.backendList->currentData().toString();
#else
    return ui.backendList->itemData(ui.backendList->currentIndex()).toString();
#endif
}


QVariantMap AuthenticationSelectionPage::authProperties() const
{
    return propertiesFromFieldWidgets(_fieldBox, _authFields[ui.backendList->currentIndex()]);
}


void AuthenticationSelectionPage::on_backendList_currentIndexChanged(int index)
{
    ui.description->setText(_authProperties[index]["Description"].toString());

    if (_fieldBox) {
        layout()->removeWidget(_fieldBox);
        _fieldBox->deleteLater();
        _fieldBox = nullptr;
    }
    if (!_authFields[index].empty()) {
        _fieldBox = new QGroupBox(this);
        _fieldBox->setTitle(tr("Authentication Settings"));
        createFieldWidgets(_fieldBox, _authFields[index]);
        static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->indexOf(ui.descriptionBox) + 1, _fieldBox);
    }
}

/*** Storage Selection Page ***/

StorageSelectionPage::StorageSelectionPage(const QVariantList &backendInfos, QWidget *parent)
    : QWizardPage(parent)
{
    ui.setupUi(this);

    setTitle(tr("Select Storage Backend"));
    setSubTitle(tr("Please select a storage backend for Quassel Core."));
    setCommitPage(true);

    registerField("storage.backend", ui.backendList);

    int defaultIndex {0};  // Legacy cores send backend infos in arbitrary order

    for (auto &&backendInfo : backendInfos) {
        auto props = backendInfo.toMap();
        // Extract field infos to avoid having to reparse the list
        std::vector<FieldInfo> fields;

        // Legacy cores (prior to 0.13) didn't send SetupData for storage backends; deal with this
        if (!props.contains("SetupData")) {
            const auto &defaultValues = props["SetupDefaults"].toMap();
            for (auto &&key : props["SetupKeys"].toStringList()) {
                fields.emplace_back(std::make_tuple(key, key, defaultValues.value(key, QString{})));
            }
            if (props.value("IsDefault", false).toBool()) {
                defaultIndex = ui.backendList->count();
            }
        }
        else {
            const auto &list = props["SetupData"].toList();
            for (int i = 0; i + 2 < list.size(); i += 3) {
                fields.emplace_back(std::make_tuple(list[i].toString(), list[i+1].toString(), list[i+2]));
            }
            props.remove("SetupData");
        }
        props.remove("SetupKeys");
        props.remove("SetupDefaults");
        // Legacy cores (prior to 0.13) don't send the BackendId property
        if (!props.contains("BackendId"))
            props["BackendId"] = props["DisplayName"];
        _backendProperties.emplace_back(props);
        _backendFields.emplace_back(std::move(fields));

        // Create entry in backend selector
        ui.backendList->addItem(props["DisplayName"].toString(), props["BackendId"].toString());
    }

    ui.backendList->setCurrentIndex(defaultIndex);
}


int StorageSelectionPage::nextId() const
{
    return CoreConfigWizard::SyncPage;
}


QString StorageSelectionPage::displayName() const
{
    return ui.backendList->currentText();
}


QString StorageSelectionPage::backend() const
{
#if QT_VERSION >= 0x050200
    return ui.backendList->currentData().toString();
#else
    return ui.backendList->itemData(ui.backendList->currentIndex()).toString();
#endif
}


QVariantMap StorageSelectionPage::backendProperties() const
{
    return propertiesFromFieldWidgets(_fieldBox, _backendFields[ui.backendList->currentIndex()]);
}


void StorageSelectionPage::on_backendList_currentIndexChanged(int index)
{
    ui.description->setText(_backendProperties[index]["Description"].toString());

    if (_fieldBox) {
        layout()->removeWidget(_fieldBox);
        _fieldBox->deleteLater();
        _fieldBox = nullptr;
    }
    if (!_backendFields[index].empty()) {
        _fieldBox = new QGroupBox(this);
        _fieldBox->setTitle(tr("Storage Settings"));
        createFieldWidgets(_fieldBox, _backendFields[index]);
        static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->indexOf(ui.descriptionBox) + 1, _fieldBox);
    }
}


/*** Sync Page ***/

SyncPage::SyncPage(QWidget *parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setTitle(tr("Storing Your Settings"));
    setSubTitle(tr("Your settings are now being stored in the core, and you will be logged in automatically."));
}


void SyncPage::initializePage()
{
    _complete = false;
    _hasError = false;
    emit completeChanged();

    // Fill in sync info about the storage layer.
    StorageSelectionPage *storagePage = qobject_cast<StorageSelectionPage *>(wizard()->page(CoreConfigWizard::StorageSelectionPage));
    QString backend = storagePage->backend();
    QVariantMap backendProperties = storagePage->backendProperties();
    ui.backend->setText(storagePage->displayName());

    // Fill in sync info about the authentication layer.
    AuthenticationSelectionPage *authPage = qobject_cast<AuthenticationSelectionPage *>(wizard()->page(CoreConfigWizard::AuthenticationSelectionPage));
    QString authenticator = authPage->authenticator();
    QVariantMap authProperties = authPage->authProperties();
    ui.authenticator->setText(authPage->displayName());

    ui.user->setText(wizard()->field("adminUser.user").toString());

    emit setupCore(backend, backendProperties, authenticator, authProperties);
}


int SyncPage::nextId() const
{
    if (!_hasError)
        return -1;
    return CoreConfigWizard::SyncRelayPage;
}


bool SyncPage::isComplete() const
{
    return _complete || _hasError;
}


void SyncPage::setStatus(const QString &status)
{
    ui.status->setText(status);
}


void SyncPage::setError(bool e)
{
    _hasError = e;
    setFinalPage(!e);
    emit completeChanged();
}


void SyncPage::setComplete(bool c)
{
    _complete = c;
    completeChanged();
}


/*** Sync Relay Page ***/

SyncRelayPage::SyncRelayPage(QWidget *parent) : QWizardPage(parent)
{
    mode = Success;
}


void SyncRelayPage::setMode(Mode m)
{
    mode = m;
}

int SyncRelayPage::nextId() const
{
    emit startOver();
    return 0;
}
};  /* namespace CoreConfigWizardPages */
