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

#include "ircconnectionwizard.h"

#include "client.h"
#include "identityeditwidget.h"
#include "simplenetworkeditor.h"

#include <QVBoxLayout>

IrcConnectionWizard::IrcConnectionWizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags),
    _introductionPage(0),
    _identityPage(0),
    _networkPage(0)
{
    _introductionPage = createIntroductionPage(this);
    _identityPage = new IdentityPage(this);
    _networkPage = new NetworkPage(this);

    addPage(_introductionPage);
    addPage(_identityPage);
    addPage(_networkPage);

    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    setOptions(options() | (QWizard::WizardOptions)(QWizard::NoDefaultButton | QWizard::CancelButtonOnLeft));
    setOption(QWizard::NoCancelButton, false);

    connect(button(QWizard::FinishButton), SIGNAL(clicked()), this, SLOT(finishClicked()));
    setButtonText(QWizard::FinishButton, tr("Save && Connect"));
}


QWizardPage *IrcConnectionWizard::createIntroductionPage(QWidget *parent)
{
    QWizardPage *page = new QWizardPage(parent);
    page->setTitle(QObject::tr("Welcome to Quassel IRC"));

    QLabel *label = new QLabel(QObject::tr("This wizard will help you to set up your default identity and your IRC network connection.<br>"
                                           "This only covers basic settings. You can cancel this wizard any time and use the settings dialog for more detailed changes."), page);
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);
    return page;
}


void IrcConnectionWizard::finishClicked()
{
    CertIdentity *identity = static_cast<IdentityPage *>(_identityPage)->identity();
    if (identity->id().isValid()) {
        Client::updateIdentity(identity->id(), identity->toVariantMap());
        identityReady(identity->id());
    }
    else {
        connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(identityReady(IdentityId)));
        Client::createIdentity(*identity);
    }
}


void IrcConnectionWizard::identityReady(IdentityId id)
{
    disconnect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(identityReady(IdentityId)));
    NetworkPage *networkPage = static_cast<NetworkPage *>(_networkPage);
    NetworkInfo networkInfo = networkPage->networkInfo();
    QStringList channels = networkPage->channelList();
    networkInfo.identity = id;
    connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(networkReady(NetworkId)));
    Client::createNetwork(networkInfo, channels);
}


void IrcConnectionWizard::networkReady(NetworkId id)
{
    disconnect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(networkReady(NetworkId)));
    const Network *net = Client::network(id);
    Q_ASSERT(net);
    net->requestConnect();
    deleteLater();
}


// ==============================
//  Wizard Pages
// ==============================

// Identity Page
IdentityPage::IdentityPage(QWidget *parent)
    : QWizardPage(parent),
    _identityEditWidget(new IdentityEditWidget(this)),
    _identity(0)
{
    setTitle(tr("Setup Identity"));

    if (Client::identityIds().isEmpty()) {
        _identity = new CertIdentity(-1, this);
        _identity->setToDefaults();
        _identity->setIdentityName(tr("Default Identity"));
    }
    else {
        _identity = new CertIdentity(*Client::identity(Client::identityIds().first()), this);
    }

    _identityEditWidget->displayIdentity(_identity);
    _identityEditWidget->showAdvanced(false);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(_identityEditWidget);
    setLayout(layout);
}


CertIdentity *IdentityPage::identity()
{
    _identityEditWidget->saveToIdentity(_identity);
    return _identity;
}


// Network Page
NetworkPage::NetworkPage(QWidget *parent)
    : QWizardPage(parent),
    _networkEditor(new SimpleNetworkEditor(this))
{
    QStringList defaultNets = Network::presetNetworks(true);
    if (!defaultNets.isEmpty()) {
        NetworkInfo info = Network::networkInfoFromPreset(defaultNets[0]);
        if (!info.networkName.isEmpty()) {
            _networkInfo = info;
            _channelList = Network::presetDefaultChannels(defaultNets[0]);
        }
    }

    _networkEditor->displayNetworkInfo(_networkInfo);
    _networkEditor->setDefaultChannels(_channelList);

    setTitle(tr("Setup Network Connection"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(_networkEditor);
    setLayout(layout);
}


NetworkInfo NetworkPage::networkInfo()
{
    _networkEditor->saveToNetworkInfo(_networkInfo);
    return _networkInfo;
}


QStringList NetworkPage::channelList()
{
    _channelList = _networkEditor->defaultChannels();
    return _channelList;
}
