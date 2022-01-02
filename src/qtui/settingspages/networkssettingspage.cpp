/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "networkssettingspage.h"

#include <utility>

#include <QHeaderView>
#include <QMessageBox>
#include <QTextCodec>

#include "client.h"
#include "icon.h"
#include "identity.h"
#include "network.h"
#include "presetnetworks.h"
#include "settingspagedlg.h"
#include "util.h"
#include "widgethelpers.h"

// IRCv3 capabilities
#include "irccap.h"

#include "settingspages/identitiessettingspage.h"

NetworksSettingsPage::NetworksSettingsPage(QWidget* parent)
    : SettingsPage(tr("IRC"), tr("Networks"), parent)
{
    ui.setupUi(this);

    // hide SASL options for older cores
    if (!Client::isCoreFeatureEnabled(Quassel::Feature::SaslAuthentication))
        ui.sasl->hide();
    if (!Client::isCoreFeatureEnabled(Quassel::Feature::SaslExternal))
        ui.saslExtInfo->hide();

    // set up icons
    ui.renameNetwork->setIcon(icon::get("edit-rename"));
    ui.addNetwork->setIcon(icon::get("list-add"));
    ui.deleteNetwork->setIcon(icon::get("edit-delete"));
    ui.addServer->setIcon(icon::get("list-add"));
    ui.deleteServer->setIcon(icon::get("edit-delete"));
    ui.editServer->setIcon(icon::get("configure"));
    ui.upServer->setIcon(icon::get("go-up"));
    ui.downServer->setIcon(icon::get("go-down"));
    ui.editIdentities->setIcon(icon::get("configure"));

    connectedIcon = icon::get("network-connect");
    connectingIcon = icon::get("network-wired");  // FIXME network-connecting
    disconnectedIcon = icon::get("network-disconnect");

    // Status icons
    infoIcon = icon::get({"emblem-information", "dialog-information"});
    successIcon = icon::get({"emblem-success", "dialog-information"});
    unavailableIcon = icon::get({"emblem-unavailable", "dialog-warning"});
    questionIcon = icon::get({"emblem-question", "dialog-question", "dialog-information"});

    foreach (int mib, QTextCodec::availableMibs()) {
        QByteArray codec = QTextCodec::codecForMib(mib)->name();
        ui.sendEncoding->addItem(codec);
        ui.recvEncoding->addItem(codec);
        ui.serverEncoding->addItem(codec);
    }
    ui.sendEncoding->model()->sort(0);
    ui.recvEncoding->model()->sort(0);
    ui.serverEncoding->model()->sort(0);
    currentId = 0;
    setEnabled(Client::isConnected());  // need a core connection!
    setWidgetStates();

    connectToWidgetsChangedSignals({ui.identityList,         ui.performEdit,          ui.sasl,
                                    ui.saslAccount,          ui.saslPassword,         ui.autoIdentify,
                                    ui.autoIdentifyService,  ui.autoIdentifyPassword, ui.useCustomEncodings,
                                    ui.sendEncoding,         ui.recvEncoding,         ui.serverEncoding,
                                    ui.autoReconnect,        ui.reconnectInterval,    ui.reconnectRetries,
                                    ui.unlimitedRetries,     ui.rejoinOnReconnect,    ui.useCustomMessageRate,
                                    ui.messageRateBurstSize, ui.messageRateDelay,     ui.unlimitedMessageRate,
                                    ui.enableCapServerTime},
                                   this,
                                   &NetworksSettingsPage::widgetHasChanged);

    connect(Client::instance(), &Client::coreConnectionStateChanged, this, &NetworksSettingsPage::coreConnectionStateChanged);
    connect(Client::instance(), &Client::networkCreated, this, &NetworksSettingsPage::clientNetworkAdded);
    connect(Client::instance(), &Client::networkRemoved, this, &NetworksSettingsPage::clientNetworkRemoved);
    connect(Client::instance(), &Client::identityCreated, this, &NetworksSettingsPage::clientIdentityAdded);
    connect(Client::instance(), &Client::identityRemoved, this, &NetworksSettingsPage::clientIdentityRemoved);

    foreach (IdentityId id, Client::identityIds()) {
        clientIdentityAdded(id);
    }
}

void NetworksSettingsPage::save()
{
    setEnabled(false);
    if (currentId != 0)
        saveToNetworkInfo(networkInfos[currentId]);

    QList<NetworkInfo> toCreate, toUpdate;
    QList<NetworkId> toRemove;
    QHash<NetworkId, NetworkInfo>::iterator i = networkInfos.begin();
    while (i != networkInfos.end()) {
        NetworkId id = (*i).networkId;
        if (id < 0) {
            toCreate.append(*i);
            // if(id == currentId) currentId = 0;
            // QList<QListWidgetItem *> items = ui.networkList->findItems((*i).networkName, Qt::MatchExactly);
            // if(items.count()) {
            //  Q_ASSERT(items[0]->data(Qt::UserRole).value<NetworkId>() == id);
            //  delete items[0];
            //}
            // i = networkInfos.erase(i);
            ++i;
        }
        else {
            if ((*i) != Client::network((*i).networkId)->networkInfo()) {
                toUpdate.append(*i);
            }
            ++i;
        }
    }
    foreach (NetworkId id, Client::networkIds()) {
        if (!networkInfos.contains(id))
            toRemove.append(id);
    }
    SaveNetworksDlg dlg(toCreate, toUpdate, toRemove, this);
    int ret = dlg.exec();
    if (ret == QDialog::Rejected) {
        // canceled -> reload everything to be safe
        load();
    }
    setChangedState(false);
    setEnabled(true);
}

void NetworksSettingsPage::load()
{
    reset();

    // Handle UI dependent on core feature flags here
    if (Client::isCoreFeatureEnabled(Quassel::Feature::CustomRateLimits)) {
        // Custom rate limiting supported, allow toggling
        ui.useCustomMessageRate->setEnabled(true);
        // Reset tooltip to default.
        ui.useCustomMessageRate->setToolTip(QString("%1").arg(tr("<p>Override default message rate limiting.</p>"
                                                                 "<p><b>Setting limits too low may get you disconnected"
                                                                 " from the server!</b></p>")));
        // If changed, update the message below!
    }
    else {
        // Custom rate limiting not supported, disallow toggling
        ui.useCustomMessageRate->setEnabled(false);
        // Split up the message to allow re-using translations:
        // [Original tool-tip]
        // [Bold 'does not support feature' message]
        // [Specific version needed and feature details]
        ui.useCustomMessageRate->setToolTip(QString("%1<br/><b>%2</b><br/>%3")
                                                .arg(tr("<p>Override default message rate limiting.</p>"
                                                        "<p><b>Setting limits too low may get you disconnected"
                                                        " from the server!</b></p>"),
                                                     tr("Your Quassel core does not support this feature"),
                                                     tr("You need a Quassel core v0.13.0 or newer in order to "
                                                        "modify message rate limits.")));
    }

    if (!Client::isConnected() || Client::isCoreFeatureEnabled(Quassel::Feature::SkipIrcCaps)) {
        // Either disconnected or IRCv3 capability skippping supported, enable configuration and
        // hide warning.  Don't show the warning needlessly when disconnected.
        ui.enableCapsConfigWidget->setEnabled(true);
        ui.enableCapsStatusLabel->setText(tr("These features require support from the network"));
        ui.enableCapsStatusIcon->setPixmap(infoIcon.pixmap(16));
    }
    else {
        // Core does not IRCv3 capability skipping, show warning and disable configuration
        ui.enableCapsConfigWidget->setEnabled(false);
        ui.enableCapsStatusLabel->setText(tr("Your Quassel core is too old to configure IRCv3 features"));
        ui.enableCapsStatusIcon->setPixmap(unavailableIcon.pixmap(16));
    }

    // Hide the SASL EXTERNAL notice until a network's shown.  Stops it from showing while loading
    // backlog from the core.
    sslUpdated();

    // Reset network capability status in case no valid networks get selected (a rare situation)
    resetNetworkCapStates();

    foreach (NetworkId netid, Client::networkIds()) {
        clientNetworkAdded(netid);
    }
    ui.networkList->setCurrentRow(0);

    setChangedState(false);
}

void NetworksSettingsPage::reset()
{
    currentId = 0;
    ui.networkList->clear();
    networkInfos.clear();
}

bool NetworksSettingsPage::aboutToSave()
{
    if (currentId != 0)
        saveToNetworkInfo(networkInfos[currentId]);
    QList<int> errors;
    foreach (NetworkInfo info, networkInfos.values()) {
        if (!info.serverList.count())
            errors.append(1);
    }
    if (!errors.count())
        return true;
    QString error(tr("<b>The following problems need to be corrected before your changes can be applied:</b><ul>"));
    if (errors.contains(1))
        error += tr("<li>All networks need at least one server defined</li>");
    error += tr("</ul>");
    QMessageBox::warning(this, tr("Invalid Network Settings"), error);
    return false;
}

void NetworksSettingsPage::widgetHasChanged()
{
    if (_ignoreWidgetChanges)
        return;
    bool changed = testHasChanged();
    if (changed != hasChanged())
        setChangedState(changed);
}

bool NetworksSettingsPage::testHasChanged()
{
    if (currentId != 0) {
        saveToNetworkInfo(networkInfos[currentId]);
    }
    if (Client::networkIds().count() != networkInfos.count())
        return true;
    foreach (NetworkId id, networkInfos.keys()) {
        if (id < 0)
            return true;
        if (Client::network(id)->networkInfo() != networkInfos[id])
            return true;
    }
    return false;
}

void NetworksSettingsPage::setWidgetStates()
{
    // network list
    if (ui.networkList->selectedItems().count()) {
        ui.detailsBox->setEnabled(true);
        ui.renameNetwork->setEnabled(true);
        ui.deleteNetwork->setEnabled(true);

        /* button disabled for now
        NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        const Network *net = id > 0 ? Client::network(id) : 0;
        ui.connectNow->setEnabled(net);
        //    && (Client::network(id)->connectionState() == Network::Initialized
        //    || Client::network(id)->connectionState() == Network::Disconnected));
        if(net) {
          if(net->connectionState() == Network::Disconnected) {
            ui.connectNow->setIcon(connectedIcon);
            ui.connectNow->setText(tr("Connect"));
          } else {
            ui.connectNow->setIcon(disconnectedIcon);
            ui.connectNow->setText(tr("Disconnect"));
          }
        } else {
          ui.connectNow->setIcon(QIcon());
          ui.connectNow->setText(tr("Apply first!"));
        } */
    }
    else {
        ui.renameNetwork->setEnabled(false);
        ui.deleteNetwork->setEnabled(false);
        // ui.connectNow->setEnabled(false);
        ui.detailsBox->setEnabled(false);
    }
    // network details
    if (ui.serverList->selectedItems().count()) {
        ui.editServer->setEnabled(true);
        ui.deleteServer->setEnabled(true);
        ui.upServer->setEnabled(ui.serverList->currentRow() > 0);
        ui.downServer->setEnabled(ui.serverList->currentRow() < ui.serverList->count() - 1);
    }
    else {
        ui.editServer->setEnabled(false);
        ui.deleteServer->setEnabled(false);
        ui.upServer->setEnabled(false);
        ui.downServer->setEnabled(false);
    }
}

void NetworksSettingsPage::setItemState(NetworkId id, QListWidgetItem* item)
{
    if (!item && !(item = networkItem(id)))
        return;
    const Network* net = Client::network(id);
    if (!net || net->isInitialized())
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
    else
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    if (net && net->connectionState() == Network::Initialized) {
        item->setIcon(connectedIcon);
    }
    else if (net && net->connectionState() != Network::Disconnected) {
        item->setIcon(connectingIcon);
    }
    else {
        item->setIcon(disconnectedIcon);
    }
    if (net) {
        bool select = false;
        // check if we already have another net of this name in the list, and replace it
        QList<QListWidgetItem*> items = ui.networkList->findItems(net->networkName(), Qt::MatchExactly);
        if (items.count()) {
            foreach (QListWidgetItem* i, items) {
                NetworkId oldid = i->data(Qt::UserRole).value<NetworkId>();
                if (oldid > 0)
                    continue;  // only locally created nets should be replaced
                if (oldid == currentId) {
                    select = true;
                    currentId = 0;
                    ui.networkList->clearSelection();
                }
                int row = ui.networkList->row(i);
                if (row >= 0) {
                    QListWidgetItem* olditem = ui.networkList->takeItem(row);
                    Q_ASSERT(olditem);
                    delete olditem;
                }
                networkInfos.remove(oldid);
                break;
            }
        }
        item->setText(net->networkName());
        if (select)
            item->setSelected(true);
    }
}

void NetworksSettingsPage::resetNetworkCapStates()
{
    // Set the status to a blank (invalid) network ID, resetting all UI
    setNetworkCapStates(NetworkId());
}

void NetworksSettingsPage::setNetworkCapStates(NetworkId id)
{
    const Network* net = Client::network(id);
    if (net && Client::isCoreFeatureEnabled(Quassel::Feature::CapNegotiation)) {
        // Capability negotiation is supported, network exists.
        // Check if the network is connected.  Don't use net->isConnected() as that won't be true
        // during capability negotiation when capabilities are added and removed.
        if (net->connectionState() != Network::Disconnected) {
            // Network exists and is connected, check available capabilities...
            // [SASL]
            // Quassel switches between SASL PLAIN and SASL EXTERNAL based on the existence of an
            // SSL certificate on the identity - check EXTERNAL if CertID exists, PLAIN if not
            bool usingSASLExternal = displayedNetworkHasCertId();
            if ((usingSASLExternal && net->saslMaybeSupports(IrcCap::SaslMech::EXTERNAL))
                    || (!usingSASLExternal && net->saslMaybeSupports(IrcCap::SaslMech::PLAIN))) {
                setCapSASLStatus(CapSupportStatus::MaybeSupported, usingSASLExternal);
            }
            else {
                setCapSASLStatus(CapSupportStatus::MaybeUnsupported, usingSASLExternal);
            }

            // Add additional capability-dependent interface updates here
        }
        else {
            // Network is disconnected
            // [SASL]
            setCapSASLStatus(CapSupportStatus::Disconnected);

            // Add additional capability-dependent interface updates here
        }
    }
    else {
        // Capability negotiation is not supported and/or network doesn't exist.
        // Don't assume anything and reset all capability-dependent interface elements to neutral.
        // [SASL]
        setCapSASLStatus(CapSupportStatus::Unknown);

        // Add additional capability-dependent interface updates here
    }
}

void NetworksSettingsPage::coreConnectionStateChanged(bool state)
{
    this->setEnabled(state);
    if (state) {
        load();
    }
    else {
        // reset
        // currentId = 0;
    }
}

void NetworksSettingsPage::clientIdentityAdded(IdentityId id)
{
    const Identity* identity = Client::identity(id);
    connect(identity, &SyncableObject::updatedRemotely, this, &NetworksSettingsPage::clientIdentityUpdated);

    QString name = identity->identityName();
    for (int j = 0; j < ui.identityList->count(); j++) {
        if ((j > 0 || ui.identityList->itemData(0).toInt() != 1) && name.localeAwareCompare(ui.identityList->itemText(j)) < 0) {
            ui.identityList->insertItem(j, name, id.toInt());
            widgetHasChanged();
            return;
        }
    }
    // append
    ui.identityList->insertItem(ui.identityList->count(), name, id.toInt());
    widgetHasChanged();
}

void NetworksSettingsPage::clientIdentityUpdated()
{
    const auto* identity = qobject_cast<const Identity*>(sender());
    if (!identity) {
        qWarning() << "NetworksSettingsPage: Invalid identity to update!";
        return;
    }
    int row = ui.identityList->findData(identity->id().toInt());
    if (row < 0) {
        qWarning() << "NetworksSettingsPage: Invalid identity to update!";
        return;
    }
    if (ui.identityList->itemText(row) != identity->identityName()) {
        ui.identityList->setItemText(row, identity->identityName());
    }
}

void NetworksSettingsPage::clientIdentityRemoved(IdentityId id)
{
    IdentityId defaultId = defaultIdentity();
    if (currentId != 0)
        saveToNetworkInfo(networkInfos[currentId]);
    foreach (NetworkInfo info, networkInfos.values()) {
        if (info.identity == id) {
            if (info.networkId == currentId)
                ui.identityList->setCurrentIndex(0);
            info.identity = defaultId;
            networkInfos[info.networkId] = info;
            if (info.networkId > 0)
                Client::updateNetwork(info);
        }
    }
    ui.identityList->removeItem(ui.identityList->findData(id.toInt()));
    widgetHasChanged();
}

QListWidgetItem* NetworksSettingsPage::networkItem(NetworkId id) const
{
    for (int i = 0; i < ui.networkList->count(); i++) {
        QListWidgetItem* item = ui.networkList->item(i);
        if (item->data(Qt::UserRole).value<NetworkId>() == id)
            return item;
    }
    return nullptr;
}

void NetworksSettingsPage::clientNetworkAdded(NetworkId id)
{
    insertNetwork(id);
    // connect(Client::network(id), &Network::updatedRemotely, this, &NetworksSettingsPage::clientNetworkUpdated);
    connect(Client::network(id), &Network::configChanged, this, &NetworksSettingsPage::clientNetworkUpdated);

    connect(Client::network(id), &Network::connectionStateSet, this, &NetworksSettingsPage::networkConnectionStateChanged);
    connect(Client::network(id), &Network::connectionError, this, &NetworksSettingsPage::networkConnectionError);

    // Handle capability changes in case a server dis/connects with the settings window open.
    connect(Client::network(id), &Network::capAdded, this, &NetworksSettingsPage::clientNetworkCapsUpdated);
    connect(Client::network(id), &Network::capRemoved, this, &NetworksSettingsPage::clientNetworkCapsUpdated);
}

void NetworksSettingsPage::clientNetworkUpdated()
{
    const auto* net = qobject_cast<const Network*>(sender());
    if (!net) {
        qWarning() << "Update request for unknown network received!";
        return;
    }
    networkInfos[net->networkId()] = net->networkInfo();
    setItemState(net->networkId());
    if (net->networkId() == currentId)
        displayNetwork(net->networkId());
    setWidgetStates();
    widgetHasChanged();
}

void NetworksSettingsPage::clientNetworkRemoved(NetworkId id)
{
    if (!networkInfos.contains(id))
        return;
    if (id == currentId)
        displayNetwork(0);
    NetworkInfo info = networkInfos.take(id);
    QList<QListWidgetItem*> items = ui.networkList->findItems(info.networkName, Qt::MatchExactly);
    foreach (QListWidgetItem* item, items) {
        if (item->data(Qt::UserRole).value<NetworkId>() == id)
            delete ui.networkList->takeItem(ui.networkList->row(item));
    }
    setWidgetStates();
    widgetHasChanged();
}

void NetworksSettingsPage::networkConnectionStateChanged(Network::ConnectionState state)
{
    Q_UNUSED(state);
    const auto* net = qobject_cast<const Network*>(sender());
    if (!net)
        return;
    /*
    if(net->networkId() == currentId) {
      ui.connectNow->setEnabled(state == Network::Initialized || state == Network::Disconnected);
    }
    */
    setItemState(net->networkId());
    if (net->networkId() == currentId) {
        // Network is currently shown.  Update the capability-dependent UI in case capabilities have
        // changed.
        setNetworkCapStates(currentId);
    }
    setWidgetStates();
}

void NetworksSettingsPage::networkConnectionError(const QString&) {}

QListWidgetItem* NetworksSettingsPage::insertNetwork(NetworkId id)
{
    NetworkInfo info = Client::network(id)->networkInfo();
    networkInfos[id] = info;
    return insertNetwork(info);
}

QListWidgetItem* NetworksSettingsPage::insertNetwork(const NetworkInfo& info)
{
    QListWidgetItem* item = nullptr;
    QList<QListWidgetItem*> items = ui.networkList->findItems(info.networkName, Qt::MatchExactly);
    if (!items.count())
        item = new QListWidgetItem(disconnectedIcon, info.networkName, ui.networkList);
    else {
        // we overwrite an existing net if it a) has the same name and b) has a negative ID meaning we created it locally before
        // -> then we can be sure that this is the core-side replacement for the net we created
        foreach (QListWidgetItem* i, items) {
            NetworkId id = i->data(Qt::UserRole).value<NetworkId>();
            if (id < 0) {
                item = i;
                break;
            }
        }
        if (!item)
            item = new QListWidgetItem(disconnectedIcon, info.networkName, ui.networkList);
    }
    item->setData(Qt::UserRole, QVariant::fromValue(info.networkId));
    setItemState(info.networkId, item);
    widgetHasChanged();
    return item;
}

// Called when selecting 'Configure' from the buffer list
void NetworksSettingsPage::bufferList_Open(NetworkId netId)
{
    QListWidgetItem* item = networkItem(netId);
    ui.networkList->setCurrentItem(item, QItemSelectionModel::SelectCurrent);
}

void NetworksSettingsPage::displayNetwork(NetworkId id)
{
    _ignoreWidgetChanges = true;
    if (id != 0) {
        NetworkInfo info = networkInfos[id];

        // this is only needed when the core supports SASL EXTERNAL
        if (Client::isCoreFeatureEnabled(Quassel::Feature::SaslExternal)) {
            if (_cid) {
                // Clean up existing CertIdentity
                disconnect(_cid, &CertIdentity::sslSettingsUpdated, this, &NetworksSettingsPage::sslUpdated);
                delete _cid;
                _cid = nullptr;
            }
            auto *identity = Client::identity(info.identity);
            if (identity) {
                // Connect new CertIdentity
                _cid = new CertIdentity(*identity, this);
                _cid->enableEditSsl(true);
                connect(_cid, &CertIdentity::sslSettingsUpdated, this, &NetworksSettingsPage::sslUpdated);
            }
            else {
                qWarning() << "NetworksSettingsPage::displayNetwork can't find Identity for IdentityId:" << info.identity;
            }
        }

        ui.identityList->setCurrentIndex(ui.identityList->findData(info.identity.toInt()));
        ui.serverList->clear();
        foreach (Network::Server server, info.serverList) {
            QListWidgetItem* item = new QListWidgetItem(QString("%1:%2").arg(server.host).arg(server.port));
            if (server.useSsl)
                item->setIcon(icon::get("document-encrypt"));
            ui.serverList->addItem(item);
        }
        // setItemState(id);
        // ui.randomServer->setChecked(info.useRandomServer);
        // Update the capability-dependent UI in case capabilities have changed.
        setNetworkCapStates(id);
        ui.performEdit->setPlainText(info.perform.join("\n"));
        ui.autoIdentify->setChecked(info.useAutoIdentify);
        ui.autoIdentifyService->setText(info.autoIdentifyService);
        ui.autoIdentifyPassword->setText(info.autoIdentifyPassword);
        ui.sasl->setChecked(info.useSasl);
        ui.saslAccount->setText(info.saslAccount);
        ui.saslPassword->setText(info.saslPassword);
        if (info.codecForEncoding.isEmpty()) {
            ui.sendEncoding->setCurrentIndex(ui.sendEncoding->findText(Network::defaultCodecForEncoding()));
            ui.recvEncoding->setCurrentIndex(ui.recvEncoding->findText(Network::defaultCodecForDecoding()));
            ui.serverEncoding->setCurrentIndex(ui.serverEncoding->findText(Network::defaultCodecForServer()));
            ui.useCustomEncodings->setChecked(false);
        }
        else {
            ui.sendEncoding->setCurrentIndex(ui.sendEncoding->findText(info.codecForEncoding));
            ui.recvEncoding->setCurrentIndex(ui.recvEncoding->findText(info.codecForDecoding));
            ui.serverEncoding->setCurrentIndex(ui.serverEncoding->findText(info.codecForServer));
            ui.useCustomEncodings->setChecked(true);
        }
        ui.autoReconnect->setChecked(info.useAutoReconnect);
        ui.reconnectInterval->setValue(info.autoReconnectInterval);
        ui.reconnectRetries->setValue(info.autoReconnectRetries);
        ui.unlimitedRetries->setChecked(info.unlimitedReconnectRetries);
        ui.rejoinOnReconnect->setChecked(info.rejoinChannels);
        // Custom rate limiting
        ui.unlimitedMessageRate->setChecked(info.unlimitedMessageRate);
        // Set 'ui.useCustomMessageRate' after 'ui.unlimitedMessageRate' so if the latter is
        // disabled, 'ui.messageRateDelayFrame' will remain disabled.
        ui.useCustomMessageRate->setChecked(info.useCustomMessageRate);
        ui.messageRateBurstSize->setValue(info.messageRateBurstSize);
        // Convert milliseconds (integer) into seconds (double)
        ui.messageRateDelay->setValue(info.messageRateDelay / 1000.0f);
        // Skipped IRCv3 capabilities
        ui.enableCapServerTime->setChecked(!info.skipCaps.contains(IrcCap::SERVER_TIME));
    }
    else {
        // just clear widgets
        if (_cid) {
            disconnect(_cid, &CertIdentity::sslSettingsUpdated, this, &NetworksSettingsPage::sslUpdated);
            delete _cid;
        }
        ui.identityList->setCurrentIndex(-1);
        ui.serverList->clear();
        ui.performEdit->clear();
        ui.autoIdentifyService->clear();
        ui.autoIdentifyPassword->clear();
        ui.saslAccount->clear();
        ui.saslPassword->clear();
        setWidgetStates();
    }
    _ignoreWidgetChanges = false;
    currentId = id;
}

void NetworksSettingsPage::saveToNetworkInfo(NetworkInfo& info)
{
    info.identity = ui.identityList->itemData(ui.identityList->currentIndex()).toInt();
    // info.useRandomServer = ui.randomServer->isChecked();
    info.perform = ui.performEdit->toPlainText().split("\n");
    info.useAutoIdentify = ui.autoIdentify->isChecked();
    info.autoIdentifyService = ui.autoIdentifyService->text();
    info.autoIdentifyPassword = ui.autoIdentifyPassword->text();
    info.useSasl = ui.sasl->isChecked();
    info.saslAccount = ui.saslAccount->text();
    info.saslPassword = ui.saslPassword->text();
    if (!ui.useCustomEncodings->isChecked()) {
        info.codecForEncoding.clear();
        info.codecForDecoding.clear();
        info.codecForServer.clear();
    }
    else {
        info.codecForEncoding = ui.sendEncoding->currentText().toLatin1();
        info.codecForDecoding = ui.recvEncoding->currentText().toLatin1();
        info.codecForServer = ui.serverEncoding->currentText().toLatin1();
    }
    info.useAutoReconnect = ui.autoReconnect->isChecked();
    info.autoReconnectInterval = ui.reconnectInterval->value();
    info.autoReconnectRetries = ui.reconnectRetries->value();
    info.unlimitedReconnectRetries = ui.unlimitedRetries->isChecked();
    info.rejoinChannels = ui.rejoinOnReconnect->isChecked();
    // Custom rate limiting
    info.useCustomMessageRate = ui.useCustomMessageRate->isChecked();
    info.messageRateBurstSize = ui.messageRateBurstSize->value();
    // Convert seconds (double) into milliseconds (integer)
    info.messageRateDelay = static_cast<quint32>((ui.messageRateDelay->value() * 1000));
    info.unlimitedMessageRate = ui.unlimitedMessageRate->isChecked();
    // Skipped IRCv3 capabilities
    if (ui.enableCapServerTime->isChecked()) {
        // Capability enabled, remove it from the skip list
        info.skipCaps.removeAll(IrcCap::SERVER_TIME);
    } else if (!info.skipCaps.contains(IrcCap::SERVER_TIME)) {
        // Capability disabled and not in the skip list, add it
        info.skipCaps.append(IrcCap::SERVER_TIME);
    }
}

void NetworksSettingsPage::clientNetworkCapsUpdated()
{
    // Grab the updated network
    const auto* net = qobject_cast<const Network*>(sender());
    if (!net) {
        qWarning() << "Update request for unknown network received!";
        return;
    }
    if (net->networkId() == currentId) {
        // Network is currently shown.  Update the capability-dependent UI in case capabilities have
        // changed.
        setNetworkCapStates(currentId);
    }
}

void NetworksSettingsPage::setCapSASLStatus(const CapSupportStatus saslStatus, bool usingSASLExternal)
{
    if (_capSaslStatusSelected != saslStatus || _capSaslStatusUsingExternal != usingSASLExternal) {
        // Update the cached copy of SASL status used with the Details dialog
        _capSaslStatusSelected = saslStatus;
        _capSaslStatusUsingExternal = usingSASLExternal;

        // Update the user interface
        switch (saslStatus) {
        case CapSupportStatus::Unknown:
            // There's no capability negotiation or network doesn't exist.  Don't assume
            // anything.
            ui.saslStatusLabel->setText(QString("<i>%1</i>").arg(tr("Could not check if supported by network")));
            ui.saslStatusIcon->setPixmap(questionIcon.pixmap(16));
            break;
        case CapSupportStatus::Disconnected:
            // Disconnected from network, no way to check.
            ui.saslStatusLabel->setText(QString("<i>%1</i>").arg(tr("Cannot check if supported when disconnected")));
            ui.saslStatusIcon->setPixmap(questionIcon.pixmap(16));
            break;
        case CapSupportStatus::MaybeUnsupported:
            // The network doesn't advertise support for SASL PLAIN/EXTERNAL.  Here be dragons.
            ui.saslStatusLabel->setText(QString("<i>%1</i>").arg(tr("Not currently supported by network")));
            ui.saslStatusIcon->setPixmap(unavailableIcon.pixmap(16));
            break;
        case CapSupportStatus::MaybeSupported:
            // The network advertises support for SASL PLAIN/EXTERNAL.  Encourage using it!
            // Unfortunately we don't know for sure if it's desired or functional.
            if (usingSASLExternal) {
                // SASL EXTERNAL is used
                // With SASL v3.1, it's not possible to reliably tell if SASL EXTERNAL is supported,
                // or just SASL PLAIN.  Use less assertive phrasing.
                ui.saslStatusLabel->setText(QString("<i>%1</i>").arg(tr("May be supported by network")));
            }
            else {
                // SASL PLAIN is used
                ui.saslStatusLabel->setText(QString("<i>%1</i>").arg(tr("Supported by network")));
            }
            ui.saslStatusIcon->setPixmap(successIcon.pixmap(16));
            break;
        }
    }
}

void NetworksSettingsPage::sslUpdated()
{
    if (displayedNetworkHasCertId()) {
        ui.saslPlainContents->setDisabled(true);
        ui.saslExtInfo->setHidden(false);
    }
    else {
        ui.saslPlainContents->setDisabled(false);
        // Directly re-enabling causes the widgets to ignore the parent "Use SASL Authentication"
        // state to indicate whether or not it's disabled.  To workaround this, keep track of
        // whether or not "Use SASL Authentication" is enabled, then quickly uncheck/recheck the
        // group box.
        if (!ui.sasl->isChecked()) {
            // SASL is not enabled, uncheck/recheck the group box to re-disable saslPlainContents.
            // Leaving saslPlainContents disabled doesn't work as that prevents it from re-enabling if
            // sasl is later checked.
            ui.sasl->setChecked(true);
            ui.sasl->setChecked(false);
        }
        ui.saslExtInfo->setHidden(true);
    }
    // Update whether SASL PLAIN or SASL EXTERNAL is used to detect SASL status
    if (currentId != 0) {
        setNetworkCapStates(currentId);
    }
}

/*** Network list ***/

void NetworksSettingsPage::on_networkList_itemSelectionChanged()
{
    if (currentId != 0) {
        saveToNetworkInfo(networkInfos[currentId]);
    }
    if (ui.networkList->selectedItems().count()) {
        NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        currentId = id;
        displayNetwork(id);
        ui.serverList->setCurrentRow(0);
    }
    else {
        currentId = 0;
    }
    setWidgetStates();
}

void NetworksSettingsPage::on_addNetwork_clicked()
{
    QStringList existing;
    for (int i = 0; i < ui.networkList->count(); i++)
        existing << ui.networkList->item(i)->text();
    NetworkAddDlg dlg(existing, this);
    if (dlg.exec() == QDialog::Accepted) {
        NetworkInfo info = dlg.networkInfo();
        if (info.networkName.isEmpty())
            return;  // sanity check

        NetworkId id;
        for (id = 1; id <= networkInfos.count(); id++) {
            widgetHasChanged();
            if (!networkInfos.keys().contains(-id.toInt()))
                break;
        }
        id = -id.toInt();
        info.networkId = id;
        info.identity = defaultIdentity();
        networkInfos[id] = info;
        QListWidgetItem* item = insertNetwork(info);
        ui.networkList->setCurrentItem(item);
        setWidgetStates();
    }
}

void NetworksSettingsPage::on_deleteNetwork_clicked()
{
    if (ui.networkList->selectedItems().count()) {
        NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        int ret
            = QMessageBox::question(this,
                                    tr("Delete Network?"),
                                    tr("Do you really want to delete the network \"%1\" and all related settings, including the backlog?")
                                        .arg(networkInfos[netid].networkName),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            currentId = 0;
            networkInfos.remove(netid);
            delete ui.networkList->takeItem(ui.networkList->row(ui.networkList->selectedItems()[0]));
            ui.networkList->setCurrentRow(qMin(ui.networkList->currentRow() + 1, ui.networkList->count() - 1));
            setWidgetStates();
            widgetHasChanged();
        }
    }
}

void NetworksSettingsPage::on_renameNetwork_clicked()
{
    if (!ui.networkList->selectedItems().count())
        return;
    QString old = ui.networkList->selectedItems()[0]->text();
    QStringList existing;
    for (int i = 0; i < ui.networkList->count(); i++)
        existing << ui.networkList->item(i)->text();
    NetworkEditDlg dlg(old, existing, this);
    if (dlg.exec() == QDialog::Accepted) {
        ui.networkList->selectedItems()[0]->setText(dlg.networkName());
        NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        networkInfos[netid].networkName = dlg.networkName();
        widgetHasChanged();
    }
}

/*
void NetworksSettingsPage::on_connectNow_clicked() {
  if(!ui.networkList->selectedItems().count()) return;
  NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
  const Network *net = Client::network(id);
  if(!net) return;
  if(net->connectionState() == Network::Disconnected) net->requestConnect();
  else net->requestDisconnect();
}
*/

/*** Server list ***/

void NetworksSettingsPage::on_serverList_itemSelectionChanged()
{
    setWidgetStates();
}

void NetworksSettingsPage::on_addServer_clicked()
{
    if (currentId == 0)
        return;
    ServerEditDlg dlg(Network::Server(), this);
    if (dlg.exec() == QDialog::Accepted) {
        networkInfos[currentId].serverList.append(dlg.serverData());
        displayNetwork(currentId);
        ui.serverList->setCurrentRow(ui.serverList->count() - 1);
        widgetHasChanged();
    }
}

void NetworksSettingsPage::on_editServer_clicked()
{
    if (currentId == 0)
        return;
    int cur = ui.serverList->currentRow();
    ServerEditDlg dlg(networkInfos[currentId].serverList[cur], this);
    if (dlg.exec() == QDialog::Accepted) {
        networkInfos[currentId].serverList[cur] = dlg.serverData();
        displayNetwork(currentId);
        ui.serverList->setCurrentRow(cur);
        widgetHasChanged();
    }
}

void NetworksSettingsPage::on_deleteServer_clicked()
{
    if (currentId == 0)
        return;
    int cur = ui.serverList->currentRow();
    networkInfos[currentId].serverList.removeAt(cur);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(qMin(cur, ui.serverList->count() - 1));
    widgetHasChanged();
}

void NetworksSettingsPage::on_upServer_clicked()
{
    int cur = ui.serverList->currentRow();
    Network::Server server = networkInfos[currentId].serverList.takeAt(cur);
    networkInfos[currentId].serverList.insert(cur - 1, server);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(cur - 1);
    widgetHasChanged();
}

void NetworksSettingsPage::on_downServer_clicked()
{
    int cur = ui.serverList->currentRow();
    Network::Server server = networkInfos[currentId].serverList.takeAt(cur);
    networkInfos[currentId].serverList.insert(cur + 1, server);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(cur + 1);
    widgetHasChanged();
}

void NetworksSettingsPage::on_editIdentities_clicked()
{
    SettingsPageDlg dlg(new IdentitiesSettingsPage(this), this);
    dlg.exec();
}

void NetworksSettingsPage::on_saslStatusDetails_clicked()
{
    if (ui.networkList->selectedItems().count()) {
        NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        QString& netName = networkInfos[netid].networkName;

        // If these strings are visible, one of the status messages wasn't detected below.
        QString saslStatusHeader = "[header unintentionally left blank]";
        QString saslStatusExplanation = "[explanation unintentionally left blank]";

        // If true, show a warning icon instead of an information icon
        bool useWarningIcon = false;

        // Determine which explanation to show
        switch (_capSaslStatusSelected) {
        case CapSupportStatus::Unknown:
            saslStatusHeader = tr("Could not check if SASL supported by network");
            saslStatusExplanation = tr("Quassel could not check if \"%1\" supports SASL.  This may "
                                       "be due to unsaved changes or an older Quassel core.  You "
                                       "can still try using SASL.")
                                        .arg(netName);
            break;
        case CapSupportStatus::Disconnected:
            saslStatusHeader = tr("Cannot check if SASL supported when disconnected");
            saslStatusExplanation = tr("Quassel cannot check if \"%1\" supports SASL when "
                                       "disconnected.  Connect to the network, or try using SASL "
                                       "anyways.")
                                        .arg(netName);
            break;
        case CapSupportStatus::MaybeUnsupported:
            if (displayedNetworkHasCertId()) {
                // SASL EXTERNAL is used
                saslStatusHeader = tr("SASL EXTERNAL not currently supported by network");
                saslStatusExplanation = tr("The network \"%1\" does not currently support SASL "
                                           "EXTERNAL for SSL certificate authentication.  However, "
                                           "support might be added later on.")
                                           .arg(netName);
            }
            else {
                // SASL PLAIN is used
                saslStatusHeader = tr("SASL not currently supported by network");
                saslStatusExplanation = tr("The network \"%1\" does not currently support SASL.  "
                                           "However, support might be added later on.")
                                           .arg(netName);
            }
            useWarningIcon = true;
            break;
        case CapSupportStatus::MaybeSupported:
            if (displayedNetworkHasCertId()) {
                // SASL EXTERNAL is used
                // With SASL v3.1, it's not possible to reliably tell if SASL EXTERNAL is supported,
                // or just SASL PLAIN.  Caution about this in the details dialog.
                saslStatusHeader = tr("SASL EXTERNAL may be supported by network");
                saslStatusExplanation = tr("The network \"%1\" may support SASL EXTERNAL for SSL "
                                           "certificate authentication.  In most cases, you should "
                                           "use SASL instead of NickServ identification.")
                                            .arg(netName);
            }
            else {
                // SASL PLAIN is used
                saslStatusHeader = tr("SASL supported by network");
                saslStatusExplanation = tr("The network \"%1\" supports SASL.  In most cases, you "
                                           "should use SASL instead of NickServ identification.")
                                            .arg(netName);
            }
            break;
        }

        // Process this in advance for reusability below
        const QString saslStatusMsgTitle = tr("SASL support for \"%1\"").arg(netName);
        const QString saslStatusMsgText = QString("<p><b>%1</b></p></br><p>%2</p></br><p><i>%3</i></p>")
                                              .arg(saslStatusHeader,
                                                   saslStatusExplanation,
                                                   tr("SASL is a standardized way to log in and identify yourself to "
                                                      "IRC servers."));

        if (useWarningIcon) {
            // Show as a warning dialog box
            QMessageBox::warning(this, saslStatusMsgTitle, saslStatusMsgText);
        }
        else {
            // Show as an information dialog box
            QMessageBox::information(this, saslStatusMsgTitle, saslStatusMsgText);
        }
    }
}

void NetworksSettingsPage::on_enableCapsStatusDetails_clicked()
{
    if (!Client::isConnected() || Client::isCoreFeatureEnabled(Quassel::Feature::SkipIrcCaps)) {
        // Either disconnected or IRCv3 capability skippping supported

        // Try to get a list of currently enabled features
        QStringList sortedCapsEnabled;
        // Check if a network is selected
        if (ui.networkList->selectedItems().count()) {
            // Get the underlying Network from the selected network
            NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
            const Network* net = Client::network(netid);
            if (net && Client::isCoreFeatureEnabled(Quassel::Feature::CapNegotiation)) {
                // Capability negotiation is supported, network exists.
                // If the network is disconnected, the list of enabled capabilities will be empty,
                // no need to check for that specifically.
                // Sorting isn't required, but it looks nicer.
                sortedCapsEnabled = net->capsEnabled();
                sortedCapsEnabled.sort();
            }
        }

        // Try to explain IRCv3 network features in a friendly way, including showing the currently
        // enabled features if available
        auto messageText = QString("<p>%1</p></br><p>%2</p>")
                .arg(tr("Quassel makes use of newer IRC features when supported by the IRC network."
                        "  If desired, you can disable unwanted or problematic features here."),
                     tr("The <a href=\"https://ircv3.net/irc/\">IRCv3 website</a> provides more "
                        "technical details on the IRCv3 capabilities powering these features."));

        if (!sortedCapsEnabled.isEmpty()) {
            // Format the capabilities within <code></code> blocks
            auto formattedCaps = QString("<code>%1</code>")
                    .arg(sortedCapsEnabled.join("</code>, <code>"));

            // Add the currently enabled capabilities to the list
            // This creates a new QString, but this code is not performance-critical.
            messageText = messageText.append(QString("<p><i>%1</i></p>").arg(
                                                 tr("Currently enabled IRCv3 capabilities for this "
                                                    "network: %1").arg(formattedCaps)));
        }

        QMessageBox::information(this, tr("Configuring network features"), messageText);
    }
    else {
        // Core does not IRCv3 capability skipping, show warning
        QMessageBox::warning(this, tr("Configuring network features unsupported"),
                             QString("<p><b>%1</b></p></br><p>%2</p>")
                             .arg(tr("Your Quassel core is too old to configure IRCv3 network features"),
                                  tr("You need a Quassel core v0.14.0 or newer to control what network "
                                     "features Quassel will use.")));
    }
}

void NetworksSettingsPage::on_enableCapsAdvanced_clicked()
{
    if (currentId == 0)
        return;

    CapsEditDlg dlg(networkInfos[currentId].skipCapsToString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        networkInfos[currentId].skipCapsFromString(dlg.skipCapsString());
        displayNetwork(currentId);
        widgetHasChanged();
    }
}

IdentityId NetworksSettingsPage::defaultIdentity() const
{
    IdentityId defaultId = 0;
    QList<IdentityId> ids = Client::identityIds();
    foreach (IdentityId id, ids) {
        if (defaultId == 0 || id < defaultId)
            defaultId = id;
    }
    return defaultId;
}

bool NetworksSettingsPage::displayedNetworkHasCertId() const
{
    // Check if the CertIdentity exists and that it has a non-null SSL key set
    return (_cid && !_cid->sslKey().isNull());
}

/**************************************************************************
 * NetworkAddDlg
 *************************************************************************/

NetworkAddDlg::NetworkAddDlg(QStringList exist, QWidget* parent)
    : QDialog(parent)
    , existing(std::move(exist))
{
    ui.setupUi(this);
    ui.useSSL->setIcon(icon::get("document-encrypt"));

    // Whenever useSSL is toggled, update the port number if not changed from the default
    connect(ui.useSSL, &QAbstractButton::toggled, this, &NetworkAddDlg::updateSslPort);
    // Do NOT call updateSslPort when loading settings, otherwise port settings may be overridden.
    // If useSSL is later changed to be checked by default, change port's default value, too.

    if (Client::isCoreFeatureEnabled(Quassel::Feature::VerifyServerSSL)) {
        // Synchronize requiring SSL with the use SSL checkbox
        ui.sslVerify->setEnabled(ui.useSSL->isChecked());
        connect(ui.useSSL, &QAbstractButton::toggled, ui.sslVerify, &QWidget::setEnabled);
    }
    else {
        // Core isn't new enough to allow requiring SSL; disable checkbox and uncheck
        ui.sslVerify->setEnabled(false);
        ui.sslVerify->setChecked(false);
        // Split up the message to allow re-using translations:
        // [Original tool-tip]
        // [Bold 'does not support feature' message]
        // [Specific version needed and feature details]
        ui.sslVerify->setToolTip(QString("%1<br/><b>%2</b><br/>%3")
                                     .arg(ui.sslVerify->toolTip(),
                                          tr("Your Quassel core does not support this feature"),
                                          tr("You need a Quassel core v0.13.0 or newer in order to "
                                             "verify connection security.")));
    }

    // read preset networks
    QStringList networks = PresetNetworks::names();
    foreach (QString s, existing)
        networks.removeAll(s);
    if (networks.count())
        ui.presetList->addItems(networks);
    else {
        ui.useManual->setChecked(true);
        ui.usePreset->setEnabled(false);
    }
    connect(ui.networkName, &QLineEdit::textChanged, this, &NetworkAddDlg::setButtonStates);
    connect(ui.serverAddress, &QLineEdit::textChanged, this, &NetworkAddDlg::setButtonStates);
    connect(ui.usePreset, &QRadioButton::toggled, this, &NetworkAddDlg::setButtonStates);
    connect(ui.useManual, &QRadioButton::toggled, this, &NetworkAddDlg::setButtonStates);
    setButtonStates();
}

NetworkInfo NetworkAddDlg::networkInfo() const
{
    if (ui.useManual->isChecked()) {
        NetworkInfo info;
        info.networkName = ui.networkName->text().trimmed();
        info.serverList << Network::Server(ui.serverAddress->text().trimmed(),
                                           ui.port->value(),
                                           ui.serverPassword->text(),
                                           ui.useSSL->isChecked(),
                                           ui.sslVerify->isChecked());
        return info;
    }
    else
        return PresetNetworks::networkInfo(ui.presetList->currentText());
}

void NetworkAddDlg::setButtonStates()
{
    bool ok = false;
    if (ui.usePreset->isChecked() && ui.presetList->count())
        ok = true;
    else if (ui.useManual->isChecked()) {
        ok = !ui.networkName->text().trimmed().isEmpty() && !existing.contains(ui.networkName->text().trimmed())
             && !ui.serverAddress->text().isEmpty();
    }
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void NetworkAddDlg::updateSslPort(bool isChecked)
{
    // "Use encrypted connection" was toggled, check the state...
    if (isChecked && ui.port->value() == Network::PORT_PLAINTEXT) {
        // Had been using the plain-text port, use the SSL default
        ui.port->setValue(Network::PORT_SSL);
    }
    else if (!isChecked && ui.port->value() == Network::PORT_SSL) {
        // Had been using the SSL port, use the plain-text default
        ui.port->setValue(Network::PORT_PLAINTEXT);
    }
}

/**************************************************************************
 * NetworkEditDlg
 *************************************************************************/

NetworkEditDlg::NetworkEditDlg(const QString& old, QStringList exist, QWidget* parent)
    : QDialog(parent)
    , existing(std::move(exist))
{
    ui.setupUi(this);

    if (old.isEmpty()) {
        // new network
        setWindowTitle(tr("Add Network"));
        on_networkEdit_textChanged("");  // disable ok button
    }
    else
        ui.networkEdit->setText(old);
}

QString NetworkEditDlg::networkName() const
{
    return ui.networkEdit->text().trimmed();
}

void NetworkEditDlg::on_networkEdit_textChanged(const QString& text)
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || existing.contains(text.trimmed()));
}

/**************************************************************************
 * ServerEditDlg
 *************************************************************************/
ServerEditDlg::ServerEditDlg(const Network::Server& server, QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    ui.useSSL->setIcon(icon::get("document-encrypt"));
    ui.host->setText(server.host);
    ui.host->setFocus();
    ui.port->setValue(server.port);
    ui.password->setText(server.password);
    ui.useSSL->setChecked(server.useSsl);
    ui.sslVerify->setChecked(server.sslVerify);
    ui.sslVersion->setCurrentIndex(server.sslVersion);
    ui.useProxy->setChecked(server.useProxy);
    ui.proxyType->setCurrentIndex(server.proxyType == QNetworkProxy::Socks5Proxy ? 0 : 1);
    ui.proxyHost->setText(server.proxyHost);
    ui.proxyPort->setValue(server.proxyPort);
    ui.proxyUsername->setText(server.proxyUser);
    ui.proxyPassword->setText(server.proxyPass);

    // This is a dirty hack to display the core->IRC SSL protocol dropdown
    // only if the core won't use autonegotiation to determine the best
    // protocol.  When autonegotiation was introduced, it would have been
    // a good idea to use the CoreFeatures enum to accomplish this.
    // However, since multiple versions have been released since then, that
    // is no longer possible.  Instead, we rely on the fact that the
    // Datastream protocol was introduced in the same version (0.10) as SSL
    // autonegotiation.  Because of that, we can display the dropdown only
    // if the Legacy protocol is in use.  If any other RemotePeer protocol
    // is in use, that means a newer protocol is in use and therefore the
    // core will use autonegotiation.
    if (Client::coreConnection()->peer()->protocol() != Protocol::LegacyProtocol) {
        ui.label_3->hide();
        ui.sslVersion->hide();
    }

    // Whenever useSSL is toggled, update the port number if not changed from the default
    connect(ui.useSSL, &QAbstractButton::toggled, this, &ServerEditDlg::updateSslPort);
    // Do NOT call updateSslPort when loading settings, otherwise port settings may be overridden.
    // If useSSL is later changed to be checked by default, change port's default value, too.

    if (Client::isCoreFeatureEnabled(Quassel::Feature::VerifyServerSSL)) {
        // Synchronize requiring SSL with the use SSL checkbox
        ui.sslVerify->setEnabled(ui.useSSL->isChecked());
        connect(ui.useSSL, &QAbstractButton::toggled, ui.sslVerify, &QWidget::setEnabled);
    }
    else {
        // Core isn't new enough to allow requiring SSL; disable checkbox and uncheck
        ui.sslVerify->setEnabled(false);
        ui.sslVerify->setChecked(false);
        // Split up the message to allow re-using translations:
        // [Original tool-tip]
        // [Bold 'does not support feature' message]
        // [Specific version needed and feature details]
        ui.sslVerify->setToolTip(QString("%1<br/><b>%2</b><br/>%3")
                                     .arg(ui.sslVerify->toolTip(),
                                          tr("Your Quassel core does not support this feature"),
                                          tr("You need a Quassel core v0.13.0 or newer in order to "
                                             "verify connection security.")));
    }

    on_host_textChanged();
}

Network::Server ServerEditDlg::serverData() const
{
    Network::Server server(ui.host->text().trimmed(), ui.port->value(), ui.password->text(), ui.useSSL->isChecked(), ui.sslVerify->isChecked());
    server.sslVersion = ui.sslVersion->currentIndex();
    server.useProxy = ui.useProxy->isChecked();
    server.proxyType = ui.proxyType->currentIndex() == 0 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy;
    server.proxyHost = ui.proxyHost->text();
    server.proxyPort = ui.proxyPort->value();
    server.proxyUser = ui.proxyUsername->text();
    server.proxyPass = ui.proxyPassword->text();
    return server;
}

void ServerEditDlg::on_host_textChanged()
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(ui.host->text().trimmed().isEmpty());
}

void ServerEditDlg::updateSslPort(bool isChecked)
{
    // "Use encrypted connection" was toggled, check the state...
    if (isChecked && ui.port->value() == Network::PORT_PLAINTEXT) {
        // Had been using the plain-text port, use the SSL default
        ui.port->setValue(Network::PORT_SSL);
    }
    else if (!isChecked && ui.port->value() == Network::PORT_SSL) {
        // Had been using the SSL port, use the plain-text default
        ui.port->setValue(Network::PORT_PLAINTEXT);
    }
}

/**************************************************************************
 * CapsEditDlg
 *************************************************************************/

CapsEditDlg::CapsEditDlg(const QString& oldSkipCapsString, QWidget* parent)
    : QDialog(parent)
    , oldSkipCapsString(oldSkipCapsString)
{
    ui.setupUi(this);

    // Connect to the reset button to reset the text
    // This provides an explicit way to "get back to defaults" in case someone changes settings to
    // experiment
    QPushButton* defaultsButton = ui.buttonBox->button(QDialogButtonBox::RestoreDefaults);
    connect(defaultsButton, &QPushButton::clicked, this, &CapsEditDlg::defaultSkipCaps);

    if (oldSkipCapsString.isEmpty()) {
        // Disable Reset button
        on_skipCapsEdit_textChanged("");
    }
    else {
        ui.skipCapsEdit->setText(oldSkipCapsString);
    }
}


QString CapsEditDlg::skipCapsString() const
{
    return ui.skipCapsEdit->text();
}

void CapsEditDlg::defaultSkipCaps()
{
    ui.skipCapsEdit->setText("");
}

void CapsEditDlg::on_skipCapsEdit_textChanged(const QString& text)
{
    ui.buttonBox->button(QDialogButtonBox::RestoreDefaults)->setDisabled(text.isEmpty());
}

/**************************************************************************
 * SaveNetworksDlg
 *************************************************************************/

SaveNetworksDlg::SaveNetworksDlg(const QList<NetworkInfo>& toCreate,
                                 const QList<NetworkInfo>& toUpdate,
                                 const QList<NetworkId>& toRemove,
                                 QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    numevents = toCreate.count() + toUpdate.count() + toRemove.count();
    rcvevents = 0;
    if (numevents) {
        ui.progressBar->setMaximum(numevents);
        ui.progressBar->setValue(0);

        connect(Client::instance(), &Client::networkCreated, this, &SaveNetworksDlg::clientEvent);
        connect(Client::instance(), &Client::networkRemoved, this, &SaveNetworksDlg::clientEvent);

        foreach (NetworkId id, toRemove) {
            Client::removeNetwork(id);
        }
        foreach (NetworkInfo info, toCreate) {
            Client::createNetwork(info);
        }
        foreach (NetworkInfo info, toUpdate) {
            const Network* net = Client::network(info.networkId);
            if (!net) {
                qWarning() << "Invalid client network!";
                numevents--;
                continue;
            }
            // FIXME this only checks for one changed item rather than all!
            connect(net, &SyncableObject::updatedRemotely, this, &SaveNetworksDlg::clientEvent);
            Client::updateNetwork(info);
        }
    }
    else {
        qWarning() << "Sync dialog called without stuff to change!";
        accept();
    }
}

void SaveNetworksDlg::clientEvent()
{
    ui.progressBar->setValue(++rcvevents);
    if (rcvevents >= numevents)
        accept();
}
