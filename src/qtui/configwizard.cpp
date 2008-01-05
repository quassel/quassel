/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QtGui>

#include "configwizard.h"

ConfigWizard::ConfigWizard(const QStringList &storageProviders, QWidget *parent) : QWizard(parent) {
  setPage(Page_Intro, new IntroPage());
  setPage(Page_AdminUser, new AdminUserPage());
  setPage(Page_StorageSelection, new StorageSelectionPage(storageProviders));
  setPage(Page_StorageDetails, new StorageDetailsPage());
  setPage(Page_Conclusion, new ConclusionPage(storageProviders));
  
  setStartId(Page_Intro);

#ifndef Q_WS_MAC
  setWizardStyle(ModernStyle);
#endif
 
  setOption(HaveHelpButton, false);
  setOption(NoBackButtonOnStartPage, true);
  setOption(HaveNextButtonOnLastPage, false);
  setOption(HaveFinishButtonOnEarlyPages, false);
  setOption(NoCancelButton, true);
  
  setWindowTitle(tr("Core Configuration Wizard"));
}


IntroPage::IntroPage(QWidget *parent) : QWizardPage(parent) {
  setTitle(tr("Introduction"));
  
  label = new QLabel(tr("This wizard will guide you through the setup process for your shiny new Quassel IRC Client."));
  label->setWordWrap(true);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(label);
  setLayout(layout);
}

int IntroPage::nextId() const {
  return ConfigWizard::Page_AdminUser;
}


AdminUserPage::AdminUserPage(QWidget *parent) : QWizardPage(parent) {
  setTitle(tr("Setup Admin User"));
  setSubTitle(tr("Please enter credentials for the admin user."));
  
  nameLabel = new QLabel(tr("Name:"));
  nameEdit = new QLineEdit();
  nameLabel->setBuddy(nameEdit);
  
  passwordLabel = new QLabel(tr("Password:"));
  passwordEdit = new QLineEdit();
  passwordEdit->setEchoMode(QLineEdit::Password);
  passwordLabel->setBuddy(passwordLabel);
  
  registerField("adminuser.name*", nameEdit);
  registerField("adminuser.password*", passwordEdit);
  
  QGridLayout *layout = new QGridLayout();
  layout->addWidget(nameLabel, 0, 0);
  layout->addWidget(nameEdit, 0, 1);
  layout->addWidget(passwordLabel, 1, 0);
  layout->addWidget(passwordEdit, 1, 1);
  setLayout(layout);
}

int AdminUserPage::nextId() const {
  return ConfigWizard::Page_StorageSelection;
}


StorageSelectionPage::StorageSelectionPage(const QStringList &storageProviders, QWidget *parent) : QWizardPage(parent) {
  setTitle(tr("Select Storage Provider"));
  setSubTitle(tr("Please select the storage provider you want to use."));
  
  storageSelection = new QComboBox();
  storageSelection->addItems(storageProviders);
  
  registerField("storage.provider", storageSelection);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(storageSelection);
  setLayout(layout);
}

int StorageSelectionPage::nextId() const {
  QString selection = storageSelection->currentText();
  if (!selection.compare("Sqlite", Qt::CaseInsensitive)) {
    return ConfigWizard::Page_Conclusion;
  } else {
    return ConfigWizard::Page_StorageDetails;
  }
}


StorageDetailsPage::StorageDetailsPage(QWidget *parent) : QWizardPage(parent) {
  setTitle(tr("Setup Storage Provider"));
  setSubTitle(tr("Please enter credentials for the selected storage provider."));
  
  hostLabel = new QLabel(tr("Host:"));
  hostEdit = new QLineEdit();
  hostLabel->setBuddy(hostEdit);

  portLabel = new QLabel(tr("Port:"));
  portEdit = new QLineEdit();
  QIntValidator *portValidator = new QIntValidator(0, 65535, this);
  portEdit->setValidator(portValidator);
  portLabel->setBuddy(portEdit);
  
  databaseLabel = new QLabel(tr("Database:"));
  databaseEdit = new QLineEdit();
  databaseLabel->setBuddy(databaseEdit);
  
  userLabel = new QLabel(tr("User:"));
  userEdit = new QLineEdit();
  userLabel->setBuddy(userEdit);
  
  passwordLabel = new QLabel(tr("Password:"));
  passwordEdit = new QLineEdit();
  passwordEdit->setEchoMode(QLineEdit::Password);
  passwordLabel->setBuddy(passwordLabel);
  
  registerField("storage.host*", hostEdit);
  registerField("storage.port*", portEdit);
  registerField("storage.database*", databaseEdit);
  registerField("storage.user*", userEdit);
  registerField("storage.password*", passwordEdit);
  
  QGridLayout *layout = new QGridLayout();
  layout->addWidget(hostLabel, 0, 0);
  layout->addWidget(hostEdit, 0, 1);
  layout->addWidget(portLabel, 1, 0);
  layout->addWidget(portEdit, 1, 1);
  layout->addWidget(databaseLabel, 2, 0);
  layout->addWidget(databaseEdit, 2, 1);
  layout->addWidget(userLabel, 3, 0);
  layout->addWidget(userEdit, 3, 1);
  layout->addWidget(passwordLabel, 4, 0);
  layout->addWidget(passwordEdit, 4, 1);
  setLayout(layout);
}

int StorageDetailsPage::nextId() const {
  return ConfigWizard::Page_Conclusion;
}


ConclusionPage::ConclusionPage(const QStringList &storageProviders, QWidget *parent) : QWizardPage(parent) {
  setTitle(tr("Conclusion"));
  setSubTitle(tr("You chose the following configuration:"));
  
  this->storageProviders = storageProviders;
  
  adminuser = new QLabel();
  storage = new QLabel();
  storage->setWordWrap(true);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(adminuser);
  layout->addWidget(storage);
  setLayout(layout);
}

int ConclusionPage::nextId() const {
  return -1;
}

void ConclusionPage::initializePage() {
  QString adminuserText = "Admin User: " + field("adminuser.name").toString();
  adminuser->setText(adminuserText);
  
  QString storageText = "Selected Storage Provider: ";
  QString sp = storageProviders.value(field("storage.provider").toInt());
  if (!sp.compare("Sqlite", Qt::CaseInsensitive)) {
    storageText.append(sp);
  } else {
    storageText += sp + "\nHost: " + field("storage.host").toString() + "\nPort: " + field("storage.port").toString() + "\nDatabase: " + field("storage.database").toString() + "\nUser: " + field("storage.user").toString();
  }
  storage->setText(storageText);
}

