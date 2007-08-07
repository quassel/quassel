/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                                *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "serverlist.h"
#include "identities.h"
#include "client.h"
#include "clientproxy.h"

/* NOTE: This dialog holds not only the server list, but also the identities.
 *       This makes perfect sense given the fact that connections are initiated from
 *       this dialog, and that the dialog exists during the lifetime of the program.
 */

ServerListDlg::ServerListDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);

  QSettings settings;
  settings.beginGroup("GUI");
  ui.showOnStartup->setChecked(settings.value("ShowServerListOnStartup", true).toBool());

  updateNetworkTree();
  connect(ui.networkTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));
  connect(Client::instance(), SIGNAL(sessionDataChanged(const QString &)), this, SLOT(updateNetworkTree()));

  settings.endGroup();

  connect(this, SIGNAL(requestConnect(QStringList)), ClientProxy::instance(), SLOT(gsRequestConnect(QStringList)));

  // Autoconnect
  /* Should not be the client's task... :-P
  QStringList list;
  VarMap networks = Client::retrieveSessionData("Networks").toMap();
  foreach(QString net, networks.keys()) {
    if(networks[net].toMap()["AutoConnect"].toBool()) {
      list << net;
    }
  }
  if(!list.isEmpty()) emit requestConnect(list);
  */
}

ServerListDlg::~ServerListDlg() {

}

void ServerListDlg::updateNetworkTree() {
  VarMap networks = Client::retrieveSessionData("Networks").toMap();
  //QStringList headers;
  //headers << "Network" << "Autoconnect";
  ui.networkTree->clear();
  //ui.networkTree->setHeaderLabels(headers);
  ui.networkTree->setHeaderLabel("Networks");
  QHash<QString, QTreeWidgetItem *> groups;
  foreach(QString net, networks.keys()) {
    VarMap s = networks[net].toMap();
    QString gr = s["Group"].toString();
    QTreeWidgetItem *item = 0;
    if(gr.isEmpty()) {
      item = new QTreeWidgetItem(ui.networkTree);
    } else {
      if(groups.contains(gr)) {
        item = new QTreeWidgetItem(groups[gr]);
      } else {
        QTreeWidgetItem *newgr = new QTreeWidgetItem(ui.networkTree);
        //ui.networkTree->addTopLevelItem(newgr);
        newgr->setText(0, gr);
        newgr->setFlags(newgr->flags() & ~Qt::ItemIsSelectable);
        groups[gr] = newgr;
        item = new QTreeWidgetItem(newgr);
        newgr->setExpanded(true);
        ui.networkTree->addTopLevelItem(newgr);
        //ui.networkTree->expandItem(newgr); //<-- buggy Qt?
      }
    }
    item->setText(0, net);
    item->setToolTip(0, s["Description"].toString());
    //item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    //item->setCheckState(1, Qt::Unchecked);
  }
  ui.networkTree->sortItems(0, Qt::AscendingOrder);
  updateButtons();
}

void ServerListDlg::updateButtons() {
  QList<QTreeWidgetItem *> selected = ui.networkTree->selectedItems();
  ui.editButton->setEnabled(selected.size() == 1);
  ui.deleteButton->setEnabled(selected.size() >= 1);
  ui.connectButton->setEnabled(selected.size() >= 1);

}

bool ServerListDlg::showOnStartup() {
  return ui.showOnStartup->isChecked();
}

void ServerListDlg::on_addButton_clicked() {
  NetworkEditDlg dlg(this, VarMap());
  if(dlg.exec() == QDialog::Accepted) {
    VarMap networks = Client::retrieveSessionData("Networks").toMap();
    VarMap net = dlg.getNetwork();
    networks[net["Name"].toString()] = net;
    Client::storeSessionData("Networks", networks);
    updateNetworkTree();
  }
}

void ServerListDlg::on_editButton_clicked() {
  QString curnet = ui.networkTree->currentItem()->text(0);
  VarMap networks = Client::retrieveSessionData("Networks").toMap();
  NetworkEditDlg dlg(this, networks[curnet].toMap());
  if(dlg.exec() == QDialog::Accepted) {
    VarMap net = dlg.getNetwork();
    networks.remove(curnet);
    networks[net["Name"].toString()] = net;
    Client::storeSessionData("Networks", networks);
    updateNetworkTree();
  }
}

void ServerListDlg::on_deleteButton_clicked() {
  if(QMessageBox::warning(this, tr("Remove Network?"), tr("Are you sure you want to delete the selected network(s)?"),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    VarMap networks = Client::retrieveSessionData("Networks").toMap();
    QList<QTreeWidgetItem *> sel = ui.networkTree->selectedItems();
    foreach(QTreeWidgetItem *item, sel) {
      networks.remove(item->text(0));
    }
    Client::storeSessionData("Networks", networks);
    updateNetworkTree();
  }
}

void ServerListDlg::editIdentities(bool end) {
  IdentitiesDlg dlg(this);
  if(dlg.exec() == QDialog::Accepted) {
    /* Should now all be handled within the dialog class. Global settings rulez0rs. */
    //identities = dlg.getIdentities();
    //QMap<QString, QString> mapping = dlg.getNameMapping();
    // add mapping here  <-- well, I don't fucking know anymore what I meant by this back in 2005...

    //
    //storeIdentities();
    //storeNetworks();  // ? how to treat mapping and NOT save changes not yet applied to the server list?
  }
  else if(end) exit(0);
}

void ServerListDlg::on_showOnStartup_stateChanged(int) {
  QSettings s;
  s.setValue("GUI/ShowServerListOnStartup", ui.showOnStartup->isChecked());
}

void ServerListDlg::accept() {
  QStringList nets;
  QList<QTreeWidgetItem *> list = ui.networkTree->selectedItems();
  foreach(QTreeWidgetItem *item, list) {
    nets << item->text(0);
  }
  emit requestConnect(nets);
  QDialog::accept();
}

/***************************************************************************/

NetworkEditDlg::NetworkEditDlg(QWidget *parent, VarMap _network) : QDialog(parent) {
  ui.setupUi(this);
  network = _network;
  oldName = network["Name"].toString();

  connect(ui.serverList, SIGNAL(itemSelectionChanged()), this, SLOT(updateServerButtons()));

  VarMap identities = Client::retrieveSessionData("Identities").toMap();

  ui.identityList->addItem(tr("Default Identity"));
  foreach(QString id, identities.keys()) {
    if(id != "Default") ui.identityList->addItem(id);
  }
  QStringList groups; groups << "";
  VarMap nets = Client::retrieveSessionData("Networks").toMap();
  foreach(QString net, nets.keys()) {
    QString gr = nets[net].toMap()["Group"].toString();
    if(!groups.contains(gr) && !gr.isEmpty()) {
      groups.append(gr);
    }
  }
  ui.networkGroup->addItems(groups);
  if(network.size() == 0) network = createDefaultNetwork();

  ui.networkName->setText(network["Name"].toString());
  if(network["Group"].toString().isEmpty()) ui.networkGroup->setCurrentIndex(0);
  else ui.networkGroup->setCurrentIndex(ui.networkGroup->findText(network["Group"].toString()));
  if(network["Identity"].toString().isEmpty() || network["Identity"].toString() == "Default") ui.identityList->setCurrentIndex(0);
  else ui.identityList->setCurrentIndex(ui.identityList->findText(network["Identity"].toString()));
  ui.enableAutoConnect->setChecked(network["AutoConnect"].toBool());
  updateWidgets();

  on_networkName_textChanged(ui.networkName->text());
  ui.networkName->setFocus();
}

VarMap NetworkEditDlg::createDefaultNetwork() {
  VarMap net;

  net["Name"] = QString();
  net["Group"] = QString();
  net["Identity"] = QString("Default");

  return net;
}

void NetworkEditDlg::updateWidgets() {
  ui.serverList->clear();
  foreach(QVariant s, network["Servers"].toList()) {
    VarMap server = s.toMap();
    QString entry = QString("%1:%2").arg(server["Address"].toString()).arg(server["Port"].toInt());
    QListWidgetItem *item = new QListWidgetItem(entry);
    //if(server["Exclude"].toBool()) item->setCheckState(Qt::Checked);
    ui.serverList->addItem(item);
  }
  ui.performEdit->clear();
  ui.performEdit->setText( network["Perform"].toString() );
  updateServerButtons();
}

void NetworkEditDlg::updateServerButtons() {
  Q_ASSERT(ui.serverList->selectedItems().size() <= 1);
  int curidx;
  if(ui.serverList->selectedItems().isEmpty()) curidx = -1;
  else curidx = ui.serverList->row(ui.serverList->selectedItems()[0]);
  ui.editServer->setEnabled(curidx >= 0);
  ui.deleteServer->setEnabled(curidx >= 0);
  ui.upServer->setEnabled(curidx > 0);
  ui.downServer->setEnabled(curidx >= 0 && curidx < ui.serverList->count() - 1);

}

void NetworkEditDlg::on_networkName_textChanged(QString text) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
}

void NetworkEditDlg::accept() {
  QString reason = checkValidity();
  if(reason.isEmpty()) {
    network["Name"] = ui.networkName->text();
    network["Description"] = ui.networkDesc->text();
    /*if(ui.networkGroup->currentText() == "<none>") network["Group"] = "";
    else */ network["Group"] = ui.networkGroup->currentText();
    network["AutoConnect"] = ui.enableAutoConnect->isChecked();
    network["Perform"] = ui.performEdit->toPlainText();
    if(ui.identityList->currentIndex()) network["Identity"] = ui.identityList->currentText();
    else network["Identity"] = "Default";
    QDialog::accept();
  } else {
    QMessageBox::warning(this, tr("Invalid Network Settings!"),
                         tr("<b>Your network settings are invalid!</b><br>%1").arg(reason));
  }

}

QString NetworkEditDlg::checkValidity() {
  QString r;
  VarMap nets = Client::retrieveSessionData("Networks").toMap();
  if(ui.networkName->text() != oldName && nets.keys().contains(ui.networkName->text())) {
    r += tr(" Network name already exists.");
  }
  if(network["Servers"].toList().isEmpty()) {
    r += tr(" You need to enter at least one server for this network.");
  }
  return r;
}

void NetworkEditDlg::on_addServer_clicked() {
  ServerEditDlg dlg(this);
  if(dlg.exec() == QDialog::Accepted) {
    QList<QVariant> list = network["Servers"].toList();
    list.append(dlg.getServer());
    network["Servers"] = list;
    updateWidgets();
  }
}

void NetworkEditDlg::on_editServer_clicked() {
  int idx = ui.serverList->currentRow();
  ServerEditDlg dlg(this, network["Servers"].toList()[idx].toMap());
  if(dlg.exec() == QDialog::Accepted) {
    QList<QVariant> list = network["Servers"].toList();
    list[idx] = dlg.getServer();
    network["Servers"] = list;
    updateWidgets();
  }
}

void NetworkEditDlg::on_deleteServer_clicked() {
  int idx = ui.serverList->currentRow();
  QList<QVariant> list = network["Servers"].toList();
  list.removeAt(idx);
  network["Servers"] = list;
  updateWidgets();
  if(idx < ui.serverList->count()) ui.serverList->setCurrentRow(idx);
  else if(ui.serverList->count()) ui.serverList->setCurrentRow(ui.serverList->count()-1);
}

void NetworkEditDlg::on_upServer_clicked() {
  int idx = ui.serverList->currentRow();
  QList<QVariant> list = network["Servers"].toList();
  list.swap(idx, idx-1);
  network["Servers"] = list;
  updateWidgets();
  ui.serverList->setCurrentRow(idx-1);
}

void NetworkEditDlg::on_downServer_clicked() {
  int idx = ui.serverList->currentRow();
  QList<QVariant> list = network["Servers"].toList();
  list.swap(idx, idx+1);
  network["Servers"] = list;
  updateWidgets();
  ui.serverList->setCurrentRow(idx+1);
}

void NetworkEditDlg::on_editIdentities_clicked() {
  QString id;
  if(ui.identityList->currentIndex() > 0) id = ui.identityList->currentText();
  else id = "Default";
  IdentitiesDlg dlg(this, id);
  if(dlg.exec() == QDialog::Accepted) {
    VarMap identities = Client::retrieveSessionData("Identities").toMap();
    ui.identityList->clear();
    ui.identityList->addItem(tr("Default Identity"));
    foreach(QString i, identities.keys()) {
      if(i != "Default") ui.identityList->addItem(i);
    }
    QMap<QString, QString> mapping = dlg.getNameMapping();
    if(mapping.contains(id)) id = mapping[id];
    else id = "Default";
    if(id != "Default") ui.identityList->setCurrentIndex(ui.identityList->findText(id));
    else ui.identityList->setCurrentIndex(0);
    network["Identity"] = id;
  }
}

/***************************************************************************/

ServerEditDlg::ServerEditDlg(QWidget *parent, VarMap server) : QDialog(parent) {
  ui.setupUi(this);

  if(!server.isEmpty()) {
    ui.serverAddress->setText(server["Address"].toString());
    ui.serverPort->setValue(server["Port"].toInt());
  } else {
    ui.serverAddress->setText(QString());
    ui.serverPort->setValue(6667);
  }
  on_serverAddress_textChanged();
}

void ServerEditDlg::on_serverAddress_textChanged() {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!ui.serverAddress->text().isEmpty());
}

VarMap ServerEditDlg::getServer() {
  VarMap s;
  s["Address"] = ui.serverAddress->text();
  s["Port"] = ui.serverPort->text();
  return s;
}


/***************************************************************************/
