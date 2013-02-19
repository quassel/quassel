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

#include <QHeaderView>
#include <QMessageBox>
#include <QTextCodec>

#include "networkssettingspage.h"

#include "client.h"
#include "iconloader.h"
#include "identity.h"
#include "network.h"
#include "settingspagedlg.h"
#include "util.h"

#include "settingspages/identitiessettingspage.h"

NetworksSettingsPage::NetworksSettingsPage(QWidget *parent)
    : SettingsPage(tr("IRC"), tr("Networks"), parent)
#ifdef HAVE_SSL
      , _cid(0)
#endif
{
    ui.setupUi(this);

    // hide SASL options for older cores
    if (!(Client::coreFeatures() & Quassel::SaslAuthentication))
        ui.sasl->hide();
    if (!(Client::coreFeatures() & Quassel::SaslExternal))
        ui.saslExtInfo->hide();
#ifndef HAVE_SSL
    ui.saslExtInfo->hide();
#endif

    // set up icons
    ui.renameNetwork->setIcon(SmallIcon("edit-rename"));
    ui.addNetwork->setIcon(SmallIcon("list-add"));
    ui.deleteNetwork->setIcon(SmallIcon("edit-delete"));
    ui.addServer->setIcon(SmallIcon("list-add"));
    ui.deleteServer->setIcon(SmallIcon("edit-delete"));
    ui.editServer->setIcon(SmallIcon("configure"));
    ui.upServer->setIcon(SmallIcon("go-up"));
    ui.downServer->setIcon(SmallIcon("go-down"));
    ui.editIdentities->setIcon(SmallIcon("configure"));

    _ignoreWidgetChanges = false;

    connectedIcon = SmallIcon("network-connect");
    connectingIcon = SmallIcon("network-wired"); // FIXME network-connecting
    disconnectedIcon = SmallIcon("network-disconnect");

    foreach(int mib, QTextCodec::availableMibs()) {
        QByteArray codec = QTextCodec::codecForMib(mib)->name();
        ui.sendEncoding->addItem(codec);
        ui.recvEncoding->addItem(codec);
        ui.serverEncoding->addItem(codec);
    }
    ui.sendEncoding->model()->sort(0);
    ui.recvEncoding->model()->sort(0);
    ui.serverEncoding->model()->sort(0);
    currentId = 0;
    setEnabled(Client::isConnected()); // need a core connection!
    setWidgetStates();
    connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
    connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientNetworkAdded(NetworkId)));
    connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));
    connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientIdentityAdded(IdentityId)));
    connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientIdentityRemoved(IdentityId)));

    connect(ui.identityList, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
    //connect(ui.randomServer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.performEdit, SIGNAL(textChanged()), this, SLOT(widgetHasChanged()));
    connect(ui.autoIdentify, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.autoIdentifyService, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
    connect(ui.autoIdentifyPassword, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
    connect(ui.sasl, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.saslAccount, SIGNAL(textEdited(QString)), this, SLOT(widgetHasChanged()));
    connect(ui.saslPassword, SIGNAL(textEdited(QString)), this, SLOT(widgetHasChanged()));
    connect(ui.useCustomEncodings, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.sendEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.recvEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.serverEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.autoReconnect, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.reconnectInterval, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.reconnectRetries, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.unlimitedRetries, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.rejoinOnReconnect, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    //connect(ui., SIGNAL(), this, SLOT(widgetHasChanged()));
    //connect(ui., SIGNAL(), this, SLOT(widgetHasChanged()));

    foreach(IdentityId id, Client::identityIds()) {
        clientIdentityAdded(id);
    }
}


void NetworksSettingsPage::save()
{
    setEnabled(false);
    if (currentId != 0) saveToNetworkInfo(networkInfos[currentId]);

    QList<NetworkInfo> toCreate, toUpdate;
    QList<NetworkId> toRemove;
    QHash<NetworkId, NetworkInfo>::iterator i = networkInfos.begin();
    while (i != networkInfos.end()) {
        NetworkId id = (*i).networkId;
        if (id < 0) {
            toCreate.append(*i);
            //if(id == currentId) currentId = 0;
            //QList<QListWidgetItem *> items = ui.networkList->findItems((*i).networkName, Qt::MatchExactly);
            //if(items.count()) {
            //  Q_ASSERT(items[0]->data(Qt::UserRole).value<NetworkId>() == id);
            //  delete items[0];
            //}
            //i = networkInfos.erase(i);
            ++i;
        }
        else {
            if ((*i) != Client::network((*i).networkId)->networkInfo()) {
                toUpdate.append(*i);
            }
            ++i;
        }
    }
    foreach(NetworkId id, Client::networkIds()) {
        if (!networkInfos.contains(id)) toRemove.append(id);
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
    foreach(NetworkId netid, Client::networkIds()) {
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
    if (currentId != 0) saveToNetworkInfo(networkInfos[currentId]);
    QList<int> errors;
    foreach(NetworkInfo info, networkInfos.values()) {
        if (!info.serverList.count()) errors.append(1);
    }
    if (!errors.count()) return true;
    QString error(tr("<b>The following problems need to be corrected before your changes can be applied:</b><ul>"));
    if (errors.contains(1)) error += tr("<li>All networks need at least one server defined</li>");
    error += tr("</ul>");
    QMessageBox::warning(this, tr("Invalid Network Settings"), error);
    return false;
}


void NetworksSettingsPage::widgetHasChanged()
{
    if (_ignoreWidgetChanges) return;
    bool changed = testHasChanged();
    if (changed != hasChanged()) setChangedState(changed);
}


bool NetworksSettingsPage::testHasChanged()
{
    if (currentId != 0) {
        saveToNetworkInfo(networkInfos[currentId]);
    }
    if (Client::networkIds().count() != networkInfos.count()) return true;
    foreach(NetworkId id, networkInfos.keys()) {
        if (id < 0) return true;
        if (Client::network(id)->networkInfo() != networkInfos[id]) return true;
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
        //ui.connectNow->setEnabled(false);
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


void NetworksSettingsPage::setItemState(NetworkId id, QListWidgetItem *item)
{
    if (!item && !(item = networkItem(id))) return;
    const Network *net = Client::network(id);
    if (!net || net->isInitialized()) item->setFlags(item->flags() | Qt::ItemIsEnabled);
    else item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
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
        QList<QListWidgetItem *> items = ui.networkList->findItems(net->networkName(), Qt::MatchExactly);
        if (items.count()) {
            foreach(QListWidgetItem *i, items) {
                NetworkId oldid = i->data(Qt::UserRole).value<NetworkId>();
                if (oldid > 0) continue;  // only locally created nets should be replaced
                if (oldid == currentId) {
                    select = true;
                    currentId = 0;
                    ui.networkList->clearSelection();
                }
                int row = ui.networkList->row(i);
                if (row >= 0) {
                    QListWidgetItem *olditem = ui.networkList->takeItem(row);
                    Q_ASSERT(olditem);
                    delete olditem;
                }
                networkInfos.remove(oldid);
                break;
            }
        }
        item->setText(net->networkName());
        if (select) item->setSelected(true);
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
        //currentId = 0;
    }
}


void NetworksSettingsPage::clientIdentityAdded(IdentityId id)
{
    const Identity *identity = Client::identity(id);
    connect(identity, SIGNAL(updatedRemotely()), this, SLOT(clientIdentityUpdated()));

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
    const Identity *identity = qobject_cast<const Identity *>(sender());
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
    if (currentId != 0) saveToNetworkInfo(networkInfos[currentId]);
    foreach(NetworkInfo info, networkInfos.values()) {
        if (info.identity == id) {
            if (info.networkId == currentId)
                ui.identityList->setCurrentIndex(0);
            info.identity = defaultId;
            networkInfos[info.networkId] = info;
            if (info.networkId > 0) Client::updateNetwork(info);
        }
    }
    ui.identityList->removeItem(ui.identityList->findData(id.toInt()));
    widgetHasChanged();
}


QListWidgetItem *NetworksSettingsPage::networkItem(NetworkId id) const
{
    for (int i = 0; i < ui.networkList->count(); i++) {
        QListWidgetItem *item = ui.networkList->item(i);
        if (item->data(Qt::UserRole).value<NetworkId>() == id) return item;
    }
    return 0;
}


void NetworksSettingsPage::clientNetworkAdded(NetworkId id)
{
    insertNetwork(id);
    //connect(Client::network(id), SIGNAL(updatedRemotely()), this, SLOT(clientNetworkUpdated()));
    connect(Client::network(id), SIGNAL(configChanged()), this, SLOT(clientNetworkUpdated()));

    connect(Client::network(id), SIGNAL(connectionStateSet(Network::ConnectionState)), this, SLOT(networkConnectionStateChanged(Network::ConnectionState)));
    connect(Client::network(id), SIGNAL(connectionError(const QString &)), this, SLOT(networkConnectionError(const QString &)));
}


void NetworksSettingsPage::clientNetworkUpdated()
{
    const Network *net = qobject_cast<const Network *>(sender());
    if (!net) {
        qWarning() << "Update request for unknown network received!";
        return;
    }
    networkInfos[net->networkId()] = net->networkInfo();
    setItemState(net->networkId());
    if (net->networkId() == currentId) displayNetwork(net->networkId());
    setWidgetStates();
    widgetHasChanged();
}


void NetworksSettingsPage::clientNetworkRemoved(NetworkId id)
{
    if (!networkInfos.contains(id)) return;
    if (id == currentId) displayNetwork(0);
    NetworkInfo info = networkInfos.take(id);
    QList<QListWidgetItem *> items = ui.networkList->findItems(info.networkName, Qt::MatchExactly);
    foreach(QListWidgetItem *item, items) {
        if (item->data(Qt::UserRole).value<NetworkId>() == id)
            delete ui.networkList->takeItem(ui.networkList->row(item));
    }
    setWidgetStates();
    widgetHasChanged();
}


void NetworksSettingsPage::networkConnectionStateChanged(Network::ConnectionState state)
{
    Q_UNUSED(state);
    const Network *net = qobject_cast<const Network *>(sender());
    if (!net) return;
    /*
    if(net->networkId() == currentId) {
      ui.connectNow->setEnabled(state == Network::Initialized || state == Network::Disconnected);
    }
    */
    setItemState(net->networkId());
    setWidgetStates();
}


void NetworksSettingsPage::networkConnectionError(const QString &)
{
}


QListWidgetItem *NetworksSettingsPage::insertNetwork(NetworkId id)
{
    NetworkInfo info = Client::network(id)->networkInfo();
    networkInfos[id] = info;
    return insertNetwork(info);
}


QListWidgetItem *NetworksSettingsPage::insertNetwork(const NetworkInfo &info)
{
    QListWidgetItem *item = 0;
    QList<QListWidgetItem *> items = ui.networkList->findItems(info.networkName, Qt::MatchExactly);
    if (!items.count()) item = new QListWidgetItem(disconnectedIcon, info.networkName, ui.networkList);
    else {
        // we overwrite an existing net if it a) has the same name and b) has a negative ID meaning we created it locally before
        // -> then we can be sure that this is the core-side replacement for the net we created
        foreach(QListWidgetItem *i, items) {
            NetworkId id = i->data(Qt::UserRole).value<NetworkId>();
            if (id < 0) { item = i; break; }
        }
        if (!item) item = new QListWidgetItem(disconnectedIcon, info.networkName, ui.networkList);
    }
    item->setData(Qt::UserRole, QVariant::fromValue<NetworkId>(info.networkId));
    setItemState(info.networkId, item);
    widgetHasChanged();
    return item;
}


void NetworksSettingsPage::displayNetwork(NetworkId id)
{
    _ignoreWidgetChanges = true;
    if (id != 0) {
        NetworkInfo info = networkInfos[id];

#ifdef HAVE_SSL
        // this is only needed when the core supports SASL EXTERNAL
        if (Client::coreFeatures() & Quassel::SaslExternal) {
            if (_cid) {
                disconnect(_cid, SIGNAL(sslSettingsUpdated()), this, SLOT(sslUpdated()));
                delete _cid;
            }
            _cid = new CertIdentity(*Client::identity(info.identity), this);
            _cid->enableEditSsl(true);
            connect(_cid, SIGNAL(sslSettingsUpdated()), this, SLOT(sslUpdated()));
        }
#endif

        ui.identityList->setCurrentIndex(ui.identityList->findData(info.identity.toInt()));
        ui.serverList->clear();
        foreach(Network::Server server, info.serverList) {
            QListWidgetItem *item = new QListWidgetItem(QString("%1:%2").arg(server.host).arg(server.port));
            if (server.useSsl)
                item->setIcon(SmallIcon("document-encrypt"));
            ui.serverList->addItem(item);
        }
        //setItemState(id);
        //ui.randomServer->setChecked(info.useRandomServer);
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
    }
    else {
        // just clear widgets
#ifdef HAVE_SSL
        if (_cid) {
            disconnect(_cid, SIGNAL(sslSettingsUpdated()), this, SLOT(sslUpdated()));
            delete _cid;
        }
#endif
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


void NetworksSettingsPage::saveToNetworkInfo(NetworkInfo &info)
{
    info.identity = ui.identityList->itemData(ui.identityList->currentIndex()).toInt();
    //info.useRandomServer = ui.randomServer->isChecked();
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
}


#ifdef HAVE_SSL
void NetworksSettingsPage::sslUpdated()
{
    if (_cid && !_cid->sslKey().isNull()) {
        ui.saslAccount->setDisabled(true);
        ui.saslAccountLabel->setDisabled(true);
        ui.saslPassword->setDisabled(true);
        ui.saslPasswordLabel->setDisabled(true);
        ui.saslExtInfo->setHidden(false);
    } else {
        ui.saslAccount->setDisabled(false);
        ui.saslAccountLabel->setDisabled(false);
        ui.saslPassword->setDisabled(false);
        ui.saslPasswordLabel->setDisabled(false);
        ui.saslExtInfo->setHidden(true);
    }
}
#endif


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
    for (int i = 0; i < ui.networkList->count(); i++) existing << ui.networkList->item(i)->text();
    NetworkAddDlg dlg(existing, this);
    if (dlg.exec() == QDialog::Accepted) {
        NetworkInfo info = dlg.networkInfo();
        if (info.networkName.isEmpty())
            return;  // sanity check

        NetworkId id;
        for (id = 1; id <= networkInfos.count(); id++) {
            widgetHasChanged();
            if (!networkInfos.keys().contains(-id.toInt())) break;
        }
        id = -id.toInt();
        info.networkId = id;
        info.identity = defaultIdentity();
        networkInfos[id] = info;
        QListWidgetItem *item = insertNetwork(info);
        ui.networkList->setCurrentItem(item);
        setWidgetStates();
    }
}


void NetworksSettingsPage::on_deleteNetwork_clicked()
{
    if (ui.networkList->selectedItems().count()) {
        NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
        int ret = QMessageBox::question(this, tr("Delete Network?"),
            tr("Do you really want to delete the network \"%1\" and all related settings, including the backlog?").arg(networkInfos[netid].networkName),
            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            currentId = 0;
            networkInfos.remove(netid);
            delete ui.networkList->takeItem(ui.networkList->row(ui.networkList->selectedItems()[0]));
            ui.networkList->setCurrentRow(qMin(ui.networkList->currentRow()+1, ui.networkList->count()-1));
            setWidgetStates();
            widgetHasChanged();
        }
    }
}


void NetworksSettingsPage::on_renameNetwork_clicked()
{
    if (!ui.networkList->selectedItems().count()) return;
    QString old = ui.networkList->selectedItems()[0]->text();
    QStringList existing;
    for (int i = 0; i < ui.networkList->count(); i++) existing << ui.networkList->item(i)->text();
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
    if (currentId == 0) return;
    ServerEditDlg dlg(Network::Server(), this);
    if (dlg.exec() == QDialog::Accepted) {
        networkInfos[currentId].serverList.append(dlg.serverData());
        displayNetwork(currentId);
        ui.serverList->setCurrentRow(ui.serverList->count()-1);
        widgetHasChanged();
    }
}


void NetworksSettingsPage::on_editServer_clicked()
{
    if (currentId == 0) return;
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
    if (currentId == 0) return;
    int cur = ui.serverList->currentRow();
    networkInfos[currentId].serverList.removeAt(cur);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(qMin(cur, ui.serverList->count()-1));
    widgetHasChanged();
}


void NetworksSettingsPage::on_upServer_clicked()
{
    int cur = ui.serverList->currentRow();
    Network::Server server = networkInfos[currentId].serverList.takeAt(cur);
    networkInfos[currentId].serverList.insert(cur-1, server);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(cur-1);
    widgetHasChanged();
}


void NetworksSettingsPage::on_downServer_clicked()
{
    int cur = ui.serverList->currentRow();
    Network::Server server = networkInfos[currentId].serverList.takeAt(cur);
    networkInfos[currentId].serverList.insert(cur+1, server);
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(cur+1);
    widgetHasChanged();
}


void NetworksSettingsPage::on_editIdentities_clicked()
{
    SettingsPageDlg dlg(new IdentitiesSettingsPage(this), this);
    dlg.exec();
}


IdentityId NetworksSettingsPage::defaultIdentity() const
{
    IdentityId defaultId = 0;
    QList<IdentityId> ids = Client::identityIds();
    foreach(IdentityId id, ids) {
        if (defaultId == 0 || id < defaultId)
            defaultId = id;
    }
    return defaultId;
}


/**************************************************************************
* NetworkAddDlg
*************************************************************************/

NetworkAddDlg::NetworkAddDlg(const QStringList &exist, QWidget *parent) : QDialog(parent), existing(exist)
{
    ui.setupUi(this);
    ui.useSSL->setIcon(SmallIcon("document-encrypt"));

    // read preset networks
    QStringList networks = Network::presetNetworks();
    foreach(QString s, existing)
    networks.removeAll(s);
    if (networks.count())
        ui.presetList->addItems(networks);
    else {
        ui.useManual->setChecked(true);
        ui.usePreset->setEnabled(false);
    }
    connect(ui.networkName, SIGNAL(textChanged(const QString &)), SLOT(setButtonStates()));
    connect(ui.serverAddress, SIGNAL(textChanged(const QString &)), SLOT(setButtonStates()));
    setButtonStates();
}


NetworkInfo NetworkAddDlg::networkInfo() const
{
    if (ui.useManual->isChecked()) {
        NetworkInfo info;
        info.networkName = ui.networkName->text().trimmed();
        info.serverList << Network::Server(ui.serverAddress->text().trimmed(), ui.port->value(), ui.serverPassword->text(), ui.useSSL->isChecked());
        return info;
    }
    else
        return Network::networkInfoFromPreset(ui.presetList->currentText());
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


/**************************************************************************
 * NetworkEditDlg
 *************************************************************************/

NetworkEditDlg::NetworkEditDlg(const QString &old, const QStringList &exist, QWidget *parent) : QDialog(parent), existing(exist)
{
    ui.setupUi(this);

    if (old.isEmpty()) {
        // new network
        setWindowTitle(tr("Add Network"));
        on_networkEdit_textChanged(""); // disable ok button
    }
    else ui.networkEdit->setText(old);
}


QString NetworkEditDlg::networkName() const
{
    return ui.networkEdit->text().trimmed();
}


void NetworkEditDlg::on_networkEdit_textChanged(const QString &text)
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || existing.contains(text.trimmed()));
}


/**************************************************************************
 * ServerEditDlg
 *************************************************************************/
ServerEditDlg::ServerEditDlg(const Network::Server &server, QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.useSSL->setIcon(SmallIcon("document-encrypt"));
    ui.host->setText(server.host);
    ui.port->setValue(server.port);
    ui.password->setText(server.password);
    ui.useSSL->setChecked(server.useSsl);
    ui.sslVersion->setCurrentIndex(server.sslVersion);
    ui.useProxy->setChecked(server.useProxy);
    ui.proxyType->setCurrentIndex(server.proxyType == QNetworkProxy::Socks5Proxy ? 0 : 1);
    ui.proxyHost->setText(server.proxyHost);
    ui.proxyPort->setValue(server.proxyPort);
    ui.proxyUsername->setText(server.proxyUser);
    ui.proxyPassword->setText(server.proxyPass);
    on_host_textChanged();
}


Network::Server ServerEditDlg::serverData() const
{
    Network::Server server(ui.host->text().trimmed(), ui.port->value(), ui.password->text(), ui.useSSL->isChecked());
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


/**************************************************************************
 * SaveNetworksDlg
 *************************************************************************/

SaveNetworksDlg::SaveNetworksDlg(const QList<NetworkInfo> &toCreate, const QList<NetworkInfo> &toUpdate, const QList<NetworkId> &toRemove, QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    numevents = toCreate.count() + toUpdate.count() + toRemove.count();
    rcvevents = 0;
    if (numevents) {
        ui.progressBar->setMaximum(numevents);
        ui.progressBar->setValue(0);

        connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientEvent()));
        connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientEvent()));

        foreach(NetworkId id, toRemove) {
            Client::removeNetwork(id);
        }
        foreach(NetworkInfo info, toCreate) {
            Client::createNetwork(info);
        }
        foreach(NetworkInfo info, toUpdate) {
            const Network *net = Client::network(info.networkId);
            if (!net) {
                qWarning() << "Invalid client network!";
                numevents--;
                continue;
            }
            // FIXME this only checks for one changed item rather than all!
            connect(net, SIGNAL(updatedRemotely()), this, SLOT(clientEvent()));
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
    if (rcvevents >= numevents) accept();
}
