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

#ifndef IRCCONNECTIONWIZARD_H
#define IRCCONNECTIONWIZARD_H

#include <QWizard>

#include "types.h"

class IrcConnectionWizard : public QWizard
{
    Q_OBJECT

public:
    IrcConnectionWizard(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    static QWizardPage *createIntroductionPage(QWidget *parent = 0);

private slots:
    void finishClicked();
    void identityReady(IdentityId id);
    void networkReady(NetworkId id);

private:
    QWizardPage *_introductionPage;
    QWizardPage *_identityPage;
    QWizardPage *_networkPage;
};


// ==============================
//  Wizard Pages
// ==============================

// Identity Page
#include "clientidentity.h"

class IdentityEditWidget;

class IdentityPage : public QWizardPage
{
    Q_OBJECT

public:
    IdentityPage(QWidget *parent = 0);

    CertIdentity *identity();

private:
    IdentityEditWidget *_identityEditWidget;
    CertIdentity *_identity;
};


// Network Page
#include "network.h"

class SimpleNetworkEditor;

class NetworkPage : public QWizardPage
{
    Q_OBJECT

public:
    NetworkPage(QWidget *parent = 0);

    NetworkInfo networkInfo();
    QStringList channelList();

private:
    SimpleNetworkEditor *_networkEditor;
    NetworkInfo _networkInfo;
    QStringList _channelList;
};


#endif //IRCCONNECTIONWIZARD_H
