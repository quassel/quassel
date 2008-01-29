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
#include <QMessageBox>

#include "networkssettingspage.h"

#include "client.h"
#include "global.h"
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
  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(clientNetworkAdded(NetworkId)));
  connect(Client::instance(), SIGNAL(networkRemoved(NetworkId)), this, SLOT(clientNetworkRemoved(NetworkId)));
  connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientIdentityAdded(IdentityId)));
  connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientIdentityRemoved(IdentityId)));

  connect(ui.identityList, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
  //connect(ui., SIGNAL(), this, SLOT(widgetHasChanged()));
  //connect(ui., SIGNAL(), this, SLOT(widgetHasChanged()));

  foreach(IdentityId id, Client::identityIds()) {
    clientIdentityAdded(id);
  }
}

void NetworksSettingsPage::save() {
  if(currentId != 0) saveToNetworkInfo(networkInfos[currentId]);

  // First, remove the temporarily created networks
  QList<NetworkInfo> toCreate, toUpdate;
  QList<NetworkId> toRemove;
  QHash<NetworkId, NetworkInfo>::iterator i = networkInfos.begin();
  while(i != networkInfos.end()) {
    if((*i).networkId < 0) {
      toCreate.append(*i);
      i = networkInfos.erase(i);
    } else {
      if((*i) != Client::network((*i).networkId)->networkInfo()) {
        toUpdate.append(*i);
      }
      ++i;
    }
  }
  foreach(NetworkId id, Client::networkIds()) {
    if(!networkInfos.contains(id)) toRemove.append(id);
  }
  SaveNetworksDlg dlg(toCreate, toUpdate, toRemove, this);
  int ret = dlg.exec();
  if(ret == QDialog::Rejected) {
    // canceled -> reload everything to be safe
    load();
  }
}

void NetworksSettingsPage::load() {
  reset();
  foreach(NetworkId netid, Client::networkIds()) {
    clientNetworkAdded(netid);
  }
  ui.networkList->setCurrentRow(0);
  setChangedState(false);
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

bool NetworksSettingsPage::aboutToSave() {

  return true; // FIXME
}

void NetworksSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool NetworksSettingsPage::testHasChanged() {
  if(currentId != 0) {
    saveToNetworkInfo(networkInfos[currentId]);
  }
  if(Client::networkIds().count() != networkInfos.count()) return true;
  foreach(NetworkId id, networkInfos.keys()) {
    if(id < 0) return true;
    if(Client::network(id)->networkInfo() != networkInfos[id]) return true;
  }
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
    if(Client::network(id) && Client::network(id)->isConnected()) {
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
  if(net->isInitialized()) item->setFlags(item->flags() | Qt::ItemIsEnabled);
  else item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
  if(net->isConnected()) {
    item->setIcon(connectedIcon);
  } else {
    item->setIcon(disconnectedIcon);
  }
}

void NetworksSettingsPage::clientNetworkRemoved(NetworkId) {

}

QListWidgetItem *NetworksSettingsPage::insertNetwork(NetworkId id) {
  NetworkInfo info = Client::network(id)->networkInfo();
  networkInfos[id] = info;
  return insertNetwork(info);
}

QListWidgetItem *NetworksSettingsPage::insertNetwork(const NetworkInfo &info) {
  QListWidgetItem *item = new QListWidgetItem(disconnectedIcon, info.networkName);
  item->setData(Qt::UserRole, QVariant::fromValue<NetworkId>(info.networkId));
  ui.networkList->addItem(item);
  const Network *net = Client::network(info.networkId);
  if(net->isInitialized()) item->setFlags(item->flags() | Qt::ItemIsEnabled);
  else item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
  if(net && net->isConnected()) {
    item->setIcon(connectedIcon);
  } else {
    item->setIcon(disconnectedIcon);
  }
  widgetHasChanged();
  return item;
}

void NetworksSettingsPage::displayNetwork(NetworkId id, bool dontsave) {
  Q_UNUSED(dontsave);
  NetworkInfo info = networkInfos[id];
  ui.identityList->setCurrentIndex(ui.identityList->findData(info.identity.toInt()));
  ui.serverList->clear();
  foreach(QVariantMap v, info.serverList) {
    ui.serverList->addItem(QString("%1:%2").arg(v["Host"].toString()).arg(v["Port"].toUInt()));
  }
}

void NetworksSettingsPage::saveToNetworkInfo(NetworkInfo &info) {
  info.identity = ui.identityList->itemData(ui.identityList->currentIndex()).toInt();
}
/*** Network list ***/

void NetworksSettingsPage::on_networkList_itemSelectionChanged() {
  if(currentId != 0) {
    saveToNetworkInfo(networkInfos[currentId]);
  }
  if(ui.networkList->selectedItems().count()) {
    NetworkId id = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
    currentId = id;
    displayNetwork(id);
    ui.serverList->setCurrentRow(0);
  } else {
    currentId = 0;
  }
  setWidgetStates();
}

void NetworksSettingsPage::on_addNetwork_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.networkList->count(); i++) existing << ui.networkList->item(i)->text();
  NetworkEditDlgNew dlg(QString(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    NetworkId id;
    for(id = 1; id <= networkInfos.count(); id++) {
      widgetHasChanged();
      if(!networkInfos.keys().contains(-id.toInt())) break;
    }
    id = -id.toInt();
    NetworkInfo info;
    info.networkId = id;
    info.networkName = dlg.networkName();
    info.identity = 1;
    networkInfos[id] = info;
    QListWidgetItem *item = insertNetwork(info);
    ui.networkList->setCurrentItem(item);
    setWidgetStates();
  }
}

void NetworksSettingsPage::on_deleteNetwork_clicked() {
  if(ui.networkList->selectedItems().count()) {
    NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
    int ret = QMessageBox::question(this, tr("Delete Network?"),
                                    tr("Do you really want to delete the network \"%1\" and all related settings, including the backlog?"
                                       "<br><br><em>NOTE: Backlog deletion hasn't actually been implemented yet.</em>").arg(networkInfos[netid].networkName),
                                    QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if(ret == QMessageBox::Yes) {
      currentId = 0;
      networkInfos.remove(netid); qDebug() << netid << networkInfos.count();
      delete ui.networkList->selectedItems()[0];
      ui.networkList->setCurrentRow(qMin(ui.networkList->currentRow()+1, ui.networkList->count()-1));
      setWidgetStates();
      widgetHasChanged();
    }
  }
}

void NetworksSettingsPage::on_renameNetwork_clicked() {
  if(!ui.networkList->selectedItems().count()) return;
  QString old = ui.networkList->selectedItems()[0]->text();
  QStringList existing;
  for(int i = 0; i < ui.networkList->count(); i++) existing << ui.networkList->item(i)->text();
  NetworkEditDlgNew dlg(old, existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    ui.networkList->selectedItems()[0]->setText(dlg.networkName());
    NetworkId netid = ui.networkList->selectedItems()[0]->data(Qt::UserRole).value<NetworkId>();
    networkInfos[netid].networkName = dlg.networkName();
    widgetHasChanged();
  }
}

/*** Server list ***/

void NetworksSettingsPage::on_serverList_itemSelectionChanged() {
  setWidgetStates();
}

void NetworksSettingsPage::on_addServer_clicked() {
  if(currentId == 0) return;
  ServerEditDlgNew dlg(QVariantMap(), this);
  if(dlg.exec() == QDialog::Accepted) {
    networkInfos[currentId].serverList.append(dlg.serverData());
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(ui.serverList->count()-1);
    widgetHasChanged();
  }

}

void NetworksSettingsPage::on_editServer_clicked() {
  if(currentId == 0) return;
  int cur = ui.serverList->currentRow();
  ServerEditDlgNew dlg(networkInfos[currentId].serverList[cur], this);
  if(dlg.exec() == QDialog::Accepted) {
    networkInfos[currentId].serverList[cur] = dlg.serverData();
    displayNetwork(currentId);
    ui.serverList->setCurrentRow(cur);
    widgetHasChanged();
  }
}

void NetworksSettingsPage::on_deleteServer_clicked() {
  if(currentId == 0) return;
  int cur = ui.serverList->currentRow();
  networkInfos[currentId].serverList.removeAt(cur);
  displayNetwork(currentId);
  ui.serverList->setCurrentRow(qMin(cur, ui.serverList->count()-1));
  widgetHasChanged();
}

void NetworksSettingsPage::on_upServer_clicked() {
  int cur = ui.serverList->currentRow();
  QVariantMap foo = networkInfos[currentId].serverList.takeAt(cur);
  networkInfos[currentId].serverList.insert(cur-1, foo);
  displayNetwork(currentId);
  ui.serverList->setCurrentRow(cur-1);
  widgetHasChanged();
}

void NetworksSettingsPage::on_downServer_clicked() {
  int cur = ui.serverList->currentRow();
  QVariantMap foo = networkInfos[currentId].serverList.takeAt(cur);
  networkInfos[currentId].serverList.insert(cur+1, foo);
  displayNetwork(currentId);
  ui.serverList->setCurrentRow(cur+1);
  widgetHasChanged();
}

/**************************************************************************
 * NetworkEditDlg
 *************************************************************************/

NetworkEditDlgNew::NetworkEditDlgNew(const QString &old, const QStringList &exist, QWidget *parent) : QDialog(parent), existing(exist) {
  ui.setupUi(this);

  if(old.isEmpty()) {
    // new network
    setWindowTitle(tr("Add Network"));
    on_networkEdit_textChanged(""); // disable ok button
  } else ui.networkEdit->setText(old);
}

QString NetworkEditDlgNew::networkName() const {
  return ui.networkEdit->text();

}

void NetworkEditDlgNew::on_networkEdit_textChanged(const QString &text) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || existing.contains(text));
}


/**************************************************************************
 * ServerEditDlg
 *************************************************************************/

ServerEditDlgNew::ServerEditDlgNew(const QVariantMap &serverData, QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  if(serverData.count()) {
    ui.host->setText(serverData["Host"].toString());
    ui.port->setValue(serverData["Port"].toUInt());
    ui.password->setText(serverData["Password"].toString());
    ui.useSSL->setChecked(serverData["UseSSL"].toBool());
  } else {
    ui.port->setValue(Global::defaultPort);
  }
  on_host_textChanged();
}

QVariantMap ServerEditDlgNew::serverData() const {
  QVariantMap _serverData;
  _serverData["Host"] = ui.host->text();
  _serverData["Port"] = ui.port->value();
  _serverData["Password"] = ui.password->text();
  _serverData["UseSSL"] = ui.useSSL->isChecked();
  return _serverData;
}

void ServerEditDlgNew::on_host_textChanged() {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(ui.host->text().isEmpty());
}

/**************************************************************************
 * SaveNetworksDlg
 *************************************************************************/

SaveNetworksDlg::SaveNetworksDlg(const QList<NetworkInfo> &toCreate, const QList<NetworkInfo> &toUpdate, const QList<NetworkId> &toRemove, QWidget *parent) : QDialog(parent)
{
  ui.setupUi(this);

}

