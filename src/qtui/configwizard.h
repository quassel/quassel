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

#ifndef CONFIGWIZARD_H_
#define CONFIGWIZARD_H_

#include <QWizard>

class QLabel;
class QLineEdit;
class QComboBox;

class ConfigWizard : public QWizard {
  Q_OBJECT
  
  public:
    enum {
      Page_Intro,
      Page_AdminUser,
      Page_StorageSelection,
      Page_StorageDetails,
      Page_Conclusion
    };
    
    ConfigWizard(const QStringList &storageProviders, QWidget *parent = NULL);
};

class IntroPage : public QWizardPage {
  Q_OBJECT
  
  public:
    IntroPage(QWidget *parent = NULL);
    
    int nextId() const;
    
  private:
    QLabel *label;
};

class AdminUserPage : public QWizardPage {
  Q_OBJECT
  
  public:
    AdminUserPage(QWidget *parent = NULL);
    
    int nextId() const;
    
  private:
    QLabel *nameLabel;
    QLineEdit *nameEdit;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
};

class StorageSelectionPage : public QWizardPage {
  Q_OBJECT
  
  public:
    StorageSelectionPage(const QStringList &storageProviders, QWidget *parent = NULL);
    
    int nextId() const;
    
  private:
    QComboBox *storageSelection;
};

class StorageDetailsPage : public QWizardPage {
  Q_OBJECT
  
  public:
    StorageDetailsPage(QWidget *parent = NULL);
    
    int nextId() const;
    
  private:
    QLabel *hostLabel;
    QLineEdit *hostEdit;
    QLabel *portLabel;
    QLineEdit *portEdit;
    QLabel *databaseLabel;
    QLineEdit *databaseEdit;
    QLabel *userLabel;
    QLineEdit *userEdit;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
};

class ConclusionPage : public QWizardPage {
  Q_OBJECT
  
  public:
    ConclusionPage(const QStringList &storageProviders, QWidget *parent = NULL);
    
    void initializePage();
    int nextId() const;
    
  private:
    QLabel *adminuser;
    QLabel *storage;
    QStringList storageProviders;
};

#endif /*CONFIGWIZARD_H_*/
