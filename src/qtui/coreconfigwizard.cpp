/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QDebug>
#include <QAbstractButton>

#include "coreconfigwizard.h"
#include "iconloader.h"

CoreConfigWizard::CoreConfigWizard(const QList<QVariant> &backends, QWidget *parent) : QWizard(parent) {
  foreach(QVariant v, backends) _backends[v.toMap()["DisplayName"].toString()] = v;
  setPage(IntroPage, new CoreConfigWizardPages::IntroPage(this));
  setPage(AdminUserPage, new CoreConfigWizardPages::AdminUserPage(this));
  setPage(StorageSelectionPage, new CoreConfigWizardPages::StorageSelectionPage(_backends, this));
  syncPage = new CoreConfigWizardPages::SyncPage(this);
  connect(syncPage, SIGNAL(setupCore(const QString &)), this, SLOT(prepareCoreSetup(const QString &)));
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
}

QHash<QString, QVariant> CoreConfigWizard::backends() const {
  return _backends;
}

void CoreConfigWizard::prepareCoreSetup(const QString &backend) {
  // Prevent the user from changing any settings he already specified...
  foreach(int idx, visitedPages()) page(idx)->setEnabled(false);
  QVariantMap foo;
  foo["AdminUser"] = field("adminUser.user").toString();
  foo["AdminPasswd"] = field("adminUser.password").toString();
  foo["Backend"] = backend;
  emit setupCore(foo);
}

void CoreConfigWizard::coreSetupSuccess() {
  syncPage->setStatus(tr("Your core has been successfully configured. Logging you in..."));
  syncPage->setError(false);
  syncRelayPage->setMode(CoreConfigWizardPages::SyncRelayPage::Error);
  QVariantMap loginData;
  loginData["User"] = field("adminUser.user");
  loginData["Password"] = field("adminUser.password");
  loginData["RememberPasswd"] = field("adminUser.rememberPasswd");
  emit loginToCore(loginData);
}

void CoreConfigWizard::coreSetupFailed(const QString &error) {
  syncPage->setStatus(tr("Core configuration failed:<br><b>%1</b><br>Press <em>Next</em> to start over.").arg(error));
  syncPage->setError(true);
  syncRelayPage->setMode(CoreConfigWizardPages::SyncRelayPage::Error);
  //foreach(int idx, visitedPages()) page(idx)->setEnabled(true);
  //setStartId(SyncPage);
  //restart();

}

void CoreConfigWizard::startOver() {
  foreach(int idx, visitedPages()) page(idx)->setEnabled(true);
  setStartId(CoreConfigWizard::AdminUserPage);
  restart();
}

void CoreConfigWizard::loginSuccess() {
  syncPage->setStatus(tr("Your are now logged into your freshly configured Quassel Core!<br>"
                         "Please remember to configure your identities and networks now."));
  syncPage->setComplete(true);
  syncPage->setFinalPage(true);
}

void CoreConfigWizard::syncFinished() {
  // TODO: display identities and networks settings if appropriate!
  // accept();
}

namespace CoreConfigWizardPages {

/*** Intro Page ***/

IntroPage::IntroPage(QWidget *parent) : QWizardPage(parent) {
  ui.setupUi(this);
  setTitle(tr("Introduction"));
  //setSubTitle(tr("foobar"));
  //setPixmap(QWizard::WatermarkPixmap, QPixmap(":icons/quassel-icon.png"));

}

int IntroPage::nextId() const {
  return CoreConfigWizard::AdminUserPage;

}

/*** Admin User Page ***/

AdminUserPage::AdminUserPage(QWidget *parent) : QWizardPage(parent) {
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

int AdminUserPage::nextId() const {
  return CoreConfigWizard::StorageSelectionPage;

}

bool AdminUserPage::isComplete() const {
  bool ok = !ui.user->text().isEmpty() && !ui.password->text().isEmpty() && ui.password->text() == ui.password2->text();
  return ok;
}

/*** Storage Selection Page ***/

StorageSelectionPage::StorageSelectionPage(const QHash<QString, QVariant> &backends, QWidget *parent) : QWizardPage(parent) {
  ui.setupUi(this);
  _backends = backends;

  setTitle(tr("Select Storage Backend"));
  setSubTitle(tr("Please select a database backend for the Quassel Core storage to store the backlog and other data in."));
  setCommitPage(true);

  registerField("storage.backend", ui.backendList);

  foreach(QString key, _backends.keys()) {
    ui.backendList->addItem(_backends[key].toMap()["DisplayName"].toString(), key);
  }

  on_backendList_currentIndexChanged();
}

int StorageSelectionPage::nextId() const {
  return CoreConfigWizard::SyncPage;
}

QString StorageSelectionPage::selectedBackend() const {
  return ui.backendList->currentText();
}

void StorageSelectionPage::on_backendList_currentIndexChanged() {
  QString backend = ui.backendList->itemData(ui.backendList->currentIndex()).toString();
  ui.description->setText(_backends[backend].toMap()["Description"].toString());
}

/*** Sync Page ***/

SyncPage::SyncPage(QWidget *parent) : QWizardPage(parent) {
  ui.setupUi(this);
  setTitle(tr("Storing Your Settings"));
  setSubTitle(tr("Your settings are now stored in the core, and you will be logged in automatically."));
}

void SyncPage::initializePage() {
  complete = false;
  hasError = false;
  QString backend = qobject_cast<StorageSelectionPage *>(wizard()->page(CoreConfigWizard::StorageSelectionPage))->selectedBackend();
  Q_ASSERT(!backend.isEmpty());
  ui.user->setText(wizard()->field("adminUser.user").toString());
  ui.backend->setText(backend);
  emit setupCore(backend);
}

int SyncPage::nextId() const {
  if(!hasError) return -1;
  return CoreConfigWizard::SyncRelayPage;
}

bool SyncPage::isComplete() const {
  return complete;
}

void SyncPage::setStatus(const QString &status) {
  ui.status->setText(status);
}

void SyncPage::setError(bool e) {
  hasError = e;
}

void SyncPage::setComplete(bool c) {
  complete = c;
  completeChanged();
}

/*** Sync Relay Page ***/

SyncRelayPage::SyncRelayPage(QWidget *parent) : QWizardPage(parent) {
  mode = Success;
}

void SyncRelayPage::setMode(Mode m) {
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

int SyncRelayPage::nextId() const {
  emit startOver();
  return 0;
}

};  /* namespace CoreConfigWizardPages */
