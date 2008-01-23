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

#include <QHeaderView>

#include "networkssettingspage.h"

#include "client.h"
#include "identity.h"
#include "network.h"


NetworksSettingsPage::NetworksSettingsPage(QWidget *parent) : SettingsPage(tr("General"), tr("Networks"), parent) {
  ui.setupUi(this);

  connectedIcon = QIcon(":/22x22/actions/network-connect");
  disconnectedIcon = QIcon(":/22x22/actions/network-disconnect");

  currentId = 0;
  setEnabled(false);  // need a core connection!
  setWidgetStates();
  connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
  connect(Client::instance(), SIGNAL(networkAdded(NetworkId)), this, SLOT(clientNetworkAdded(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));
  connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientIdentityAdded(IdentityId)));
  connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientIdentityRemoved(IdentityId)));

  //connect(ui.networkList, SIGNAL(itemSelectionChanged()), this, SLOT(setWidgetStates()));
  //connect(ui.serverList, SIGNAL(itemSelectionChanged()), this, SLOT(setWidgetStates()));

  foreach(IdentityId id, Client::identityIds()) {
    clientIdentityAdded(id);
  }
}

void NetworksSettingsPage::save() {


}

void NetworksSettingsPage::load() {
  reset();
  foreach(NetworkId netid, Client::networkIds()) {
    clientNetworkAdded(netid);
  }
  ui.networkList->setCurrentRow(0);
  changeState(false);
}

void NetworksSettingsPage::reset() {
  currentId = 0;
  ui.networkList->clear();
  networkInfos.clear();

  /*
  foreach(Identity *identity, identities.values()) {
    identity->deleteLater();
  }
  identities.clear();
  deletedIdentities.clear();
  changedIdentities.clear();
  ui.identityList->clear();
  */
}

void NetworksSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) changeState(changed);
}

bool NetworksSettingsPage::testHasChanged() {

  return false;
}

void NetworksSettingsPage::setWidgetStates() {
  // network list
  if(ui.networkList->selectedItems().count()) {
    NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
    ui.detailsBox->setEnabled(true);
    ui.renameNetwork->setEnabled(true);
    ui.deleteNetwork->setEnabled(true);
    ui.connectNow->setEnabled(true);
    if(Client::network(id)->isConnected()) {
      ui.connectNow->setIcon(disconnectedIcon);
      ui.connectNow->setText(tr("Disconnect"));
    } else {
      ui.connectNow->setIcon(connectedIcon);
      ui.connectNow->setText(tr("Connect"));
    }
  } else {
    ui.renameNetwork->setEnabled(false);
    ui.deleteNetwork->setEnabled(false);
    ui.connectNow->setEnabled(false);
    ui.detailsBox->setEnabled(false);
  }
  // network details
  if(ui.serverList->selectedItems().count()) {
    ui.editServer->setEnabled(true);
    ui.deleteServer->setEnabled(true);
    ui.upServer->setEnabled(ui.serverList->currentRow() > 0);
    ui.downServer->setEnabled(ui.serverList->currentRow() < ui.serverList->count() - 1);
  } else {
    ui.editServer->setEnabled(false);
    ui.deleteServer->setEnabled(false);
    ui.upServer->setEnabled(false);
    ui.downServer->setEnabled(false);
  }
}

void NetworksSettingsPage::coreConnectionStateChanged(bool state) {
  this->setEnabled(state);
  if(state) {
    load();
  } else {
    // reset
    //currentId = 0;
  }
}

QListWidgetItem *NetworksSettingsPage::networkItem(NetworkId id) const {
  for(int i = 0; i < ui.networkList->count(); i++) {
    QListWidgetItem *item = ui.networkList->item(i);
    if(item->data(Qt::UserRole).value<NetworkId>() == id) return item;
  }
  return 0;
}

void NetworksSettingsPage::clientIdentityAdded(IdentityId id) {
  const Identity * identity = Client::identity(id);
  connect(identity, SIGNAL(updatedRemotely()), this, SLOT(clientIdentityUpdated()));

  if(id == 1) {
    // default identity is always the first one!
    ui.identityList->insertItem(0, identity->identityName(), id.toInt());
  } else {
    QString name = identity->identityName();
    for(int j = 0; j < ui.identityList->count(); j++) {
      if((j>0 || ui.identityList->itemData(0).toInt() != 1) && name.localeAwareCompare(ui.identityList->itemText(j)) < 0) {
        ui.identityList->insertItem(j, name, id.toInt());
        widgetHasChanged();
        return;
      }
    }
    // append
    ui.identityList->insertItem(ui.identityList->count(), name, id.toInt());
    widgetHasChanged();
  }
}

void NetworksSettingsPage::clientIdentityUpdated() {
  const Identity *identity = qobject_cast<const Identity *>(sender());
  if(!identity) {
    qWarning() << "NetworksSettingsPage: Invalid identity to update!";
    return;
  }
  int row = ui.identityList->findData(identity->id().toInt());
  if(row < 0) {
    qWarning() << "NetworksSettingsPage: Invalid identity to update!";
    return;
  }
  if(ui.identityList->itemText(row) != identity->identityName()) {
    ui.identityList->setItemText(row, identity->identityName());
  }
}

void NetworksSettingsPage::clientIdentityRemoved(IdentityId id) {
  ui.identityList->removeItem(ui.identityList->findData(id.toInt()));
  foreach(NetworkInfo info, networkInfos.values()) {
    if(info.identity == id) info.identity = 1; // set to default
  }
  widgetHasChanged();
}


void NetworksSettingsPage::clientNetworkAdded(NetworkId id) {
  insertNetwork(id);
  connect(Client::network(id), SIGNAL(updatedRemotely()), this, SLOT(clientNetworkUpdated()));
}

void NetworksSettingsPage::clientNetworkUpdated() {
  const Network *net = qobject_cast<const Network *>(sender());
  if(!net) {
    qWarning() << "Update request for unknown network received!";
    return;
  }
  QListWidgetItem *item = networkItem(net->networkId());
  if(!item) return;
  item->setText(net->networkName());
  if(net->isConnected()) {
    item->setIcon(connectedIcon);
  } else {
    item->setIcon(disconnectedIcon);
  }
}


void NetworksSettingsPage::insertNetwork(NetworkId id) {
  NetworkInfo info = Client::network(id)->networkInfo();
  networkInfos[id] = info;
  QListWidgetItem *item = new QListWidgetItem(disconnectedIcon, info.networkName);
  item->setData(Qt::UserRole, QVariant::fromValue<NetworkId>(id));
  ui.networkList->addItem(item);
  if(Client::network(id)->isConnected()) {
    item->setIcon(connectedIcon);
  } else {
    item->setIcon(disconnectedIcon);
  }
  widgetHasChanged();
}

void NetworksSettingsPage::displayNetwork(NetworkId id, bool dontsave) {
  NetworkInfo info = networkInfos[id];
  ui.serverList->clear();
  foreach(QVariantMap v, info.serverList) {
    ui.serverList->addItem(QString("%1:%2").arg(v["Address"].toString()).arg(v["Port"].toUInt()));
  }

}

/*** Network list ***/

void NetworksSettingsPage::on_networkList_itemSelectionChanged() {
  if(ui.networkList->selectedItems().count()) {
    NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
    displayNetwork(id);
  }
  setWidgetStates();
}








