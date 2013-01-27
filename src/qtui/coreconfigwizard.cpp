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

#include <QDebug>
#include <QAbstractButton>
#include <QFormLayout>
#include <QSpinBox>

#include "coreconfigwizard.h"
#include "coreconnection.h"
#include "iconloader.h"

CoreConfigWizard::CoreConfigWizard(CoreConnection *connection, const QList<QVariant> &backends, QWidget *parent)
    : QWizard(parent),
    _connection(connection)
{
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose);

    foreach(const QVariant &v, backends)
    _backends[v.toMap()["DisplayName"].toString()] = v;

    setPage(IntroPage, new CoreConfigWizardPages::IntroPage(this));
    setPage(AdminUserPage, new CoreConfigWizardPages::AdminUserPage(this));
    setPage(StorageSelectionPage, new CoreConfigWizardPages::StorageSelectionPage(_backends, this));
    syncPage = new CoreConfigWizardPages::SyncPage(this);
    connect(syncPage, SIGNAL(setupCore(const QString &, const QVariantMap &)), SLOT(prepareCoreSetup(const QString &, const QVariantMap &)));
    setPage(SyncPage, syncPage);
    syncRelayPage = new CoreConfigWizardPages::SyncRelayPage(this);
    connect(syncRelayPage, SIGNAL(startOver()), this, SLOT(startOver()));
    setPage(SyncRelayPage, syncRelayPage);
    //setPage(Page_StorageDetails, new StorageDetailsPage());
    //setPage(Page_Conclusion, new ConclusionPage(storageProviders));

    setStartId(IntroPage);
    //setStartId(StorageSelectionPage);

#ifndef Q_WS_MAC
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
    setPixmap(QWizard::LogoPixmap, DesktopIcon("quassel"));

    connect(connection, SIGNAL(coreSetupSuccess()), SLOT(coreSetupSuccess()));
    connect(connection, SIGNAL(coreSetupFailed(QString)), SLOT(coreSetupFailed(QString)));
    //connect(connection, SIGNAL(loginSuccess()), SLOT(loginSuccess()));
    connect(connection, SIGNAL(synchronized()), SLOT(syncFinished()));
    connect(this, SIGNAL(rejected()), connection, SLOT(disconnectFromCore()));
}


QHash<QString, QVariant> CoreConfigWizard::backends() const
{
    return _backends;
}


void CoreConfigWizard::prepareCoreSetup(const QString &backend, const QVariantMap &properties)
{
    // Prevent the user from changing any settings he already specified...
    foreach(int idx, visitedPages())
    page(idx)->setEnabled(false);

    QVariantMap foo;
    foo["AdminUser"] = field("adminUser.user").toString();
    foo["AdminPasswd"] = field("adminUser.password").toString();
    foo["Backend"] = backend;
    foo["ConnectionProperties"] = properties;
    coreConnection()->doCoreSetup(foo);
}


void CoreConfigWizard::coreSetupSuccess()
{
    syncPage->setStatus(tr("Your core has been successfully configured. Logging you in..."));
    syncPage->setError(false);
    syncRelayPage->setMode(CoreConfigWizardPages::SyncRelayPage::Error);
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

    //ui.user->setText("foo");
    //ui.password->setText("foo");
    //ui.password2->setText("foo");
}


int AdminUserPage::nextId() const
{
    return CoreConfigWizard::StorageSelectionPage;
}


bool AdminUserPage::isComplete() const
{
    bool ok = !ui.user->text().isEmpty() && !ui.password->text().isEmpty() && ui.password->text() == ui.password2->text();
    return ok;
}


/*** Storage Selection Page ***/

StorageSelectionPage::StorageSelectionPage(const QHash<QString, QVariant> &backends, QWidget *parent)
    : QWizardPage(parent),
    _connectionBox(0),
    _backends(backends)
{
    ui.setupUi(this);

    setTitle(tr("Select Storage Backend"));
    setSubTitle(tr("Please select a database backend for the Quassel Core storage to store the backlog and other data in."));
    setCommitPage(true);

    registerField("storage.backend", ui.backendList);

    foreach(QString key, _backends.keys()) {
        ui.backendList->addItem(_backends[key].toMap()["DisplayName"].toString(), key);
    }

    on_backendList_currentIndexChanged();
}


int StorageSelectionPage::nextId() const
{
    return CoreConfigWizard::SyncPage;
}


QString StorageSelectionPage::selectedBackend() const
{
    return ui.backendList->currentText();
}


QVariantMap StorageSelectionPage::connectionProperties() const
{
    QString backend = ui.backendList->itemData(ui.backendList->currentIndex()).toString();

    QVariantMap properties;
    QStringList setupKeys = _backends[backend].toMap()["SetupKeys"].toStringList();
    if (!setupKeys.isEmpty()) {
        QVariantMap defaults = _backends[backend].toMap()["SetupDefaults"].toMap();
        foreach(QString key, setupKeys) {
            QWidget *widget = _connectionBox->findChild<QWidget *>(key);
            QVariant def;
            if (defaults.contains(key)) {
                def = defaults[key];
            }
            switch (def.type()) {
            case QVariant::Int:
            {
                QSpinBox *spinbox = qobject_cast<QSpinBox *>(widget);
                Q_ASSERT(spinbox);
                def = QVariant(spinbox->value());
            }
            break;
            default:
            {
                QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget);
                Q_ASSERT(lineEdit);
                def = QVariant(lineEdit->text());
            }
            }
            properties[key] = def;
        }
    }
    qDebug() << properties;

//   QVariantMap properties = _backends[backend].toMap()["ConnectionProperties"].toMap();
//   if(!properties.isEmpty() && _connectionBox) {
//     QVariantMap::iterator propertyIter = properties.begin();
//     while(propertyIter != properties.constEnd()) {
//       QWidget *widget = _connectionBox->findChild<QWidget *>(propertyIter.key());
//       switch(propertyIter.value().type()) {
//       case QVariant::Int:
//      {
//        QSpinBox *spinbox = qobject_cast<QSpinBox *>(widget);
//        Q_ASSERT(spinbox);
//        propertyIter.value() = QVariant(spinbox->value());
//      }
//      break;
//       default:
//      {
//        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget);
//        Q_ASSERT(lineEdit);
//        propertyIter.value() = QVariant(lineEdit->text());
//      }
//       }
//       propertyIter++;
//     }
//   }
    return properties;
}


void StorageSelectionPage::on_backendList_currentIndexChanged()
{
    QString backend = ui.backendList->itemData(ui.backendList->currentIndex()).toString();
    ui.description->setText(_backends[backend].toMap()["Description"].toString());

    if (_connectionBox) {
        layout()->removeWidget(_connectionBox);
        _connectionBox->deleteLater();
        _connectionBox = 0;
    }

    QStringList setupKeys = _backends[backend].toMap()["SetupKeys"].toStringList();
    if (!setupKeys.isEmpty()) {
        QVariantMap defaults = _backends[backend].toMap()["SetupDefaults"].toMap();
        QGroupBox *propertyBox = new QGroupBox(this);
        propertyBox->setTitle(tr("Connection Properties"));
        QFormLayout *formlayout = new QFormLayout;

        foreach(QString key, setupKeys) {
            QWidget *widget = 0;
            QVariant def;
            if (defaults.contains(key)) {
                def = defaults[key];
            }
            switch (def.type()) {
            case QVariant::Int:
            {
                QSpinBox *spinbox = new QSpinBox(propertyBox);
                spinbox->setMaximum(64000);
                spinbox->setValue(def.toInt());
                widget = spinbox;
            }
            break;
            default:
            {
                QLineEdit *lineEdit = new QLineEdit(def.toString(), propertyBox);
                if (key.toLower().contains("password")) {
                    lineEdit->setEchoMode(QLineEdit::Password);
                }
                widget = lineEdit;
            }
            }
            widget->setObjectName(key);
            formlayout->addRow(key + ":", widget);
        }
        propertyBox->setLayout(formlayout);
        static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->indexOf(ui.descriptionBox) + 1, propertyBox);
        _connectionBox = propertyBox;
    }
}


/*** Sync Page ***/

SyncPage::SyncPage(QWidget *parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setTitle(tr("Storing Your Settings"));
    setSubTitle(tr("Your settings are now stored in the core, and you will be logged in automatically."));
}


void SyncPage::initializePage()
{
    complete = false;
    hasError = false;

    StorageSelectionPage *storagePage = qobject_cast<StorageSelectionPage *>(wizard()->page(CoreConfigWizard::StorageSelectionPage));
    QString backend = storagePage->selectedBackend();
    QVariantMap properties = storagePage->connectionProperties();
    Q_ASSERT(!backend.isEmpty());
    ui.user->setText(wizard()->field("adminUser.user").toString());
    ui.backend->setText(backend);
    emit setupCore(backend, properties);
}


int SyncPage::nextId() const
{
    if (!hasError) return -1;
    return CoreConfigWizard::SyncRelayPage;
}


bool SyncPage::isComplete() const
{
    return complete;
}


void SyncPage::setStatus(const QString &status)
{
    ui.status->setText(status);
}


void SyncPage::setError(bool e)
{
    hasError = e;
}


void SyncPage::setComplete(bool c)
{
    complete = c;
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


/*
void SyncRelayPage::initializePage() {
  return;
  if(mode == Success) {
    wizard()->accept();
  } else {
    emit startOver();
  }
}
*/

int SyncRelayPage::nextId() const
{
    emit startOver();
    return 0;
}
};  /* namespace CoreConfigWizardPages */
