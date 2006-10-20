/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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
#include "quassel.h"

/* NOTE: This dialog holds not only the server list, but also the identities.
 *       This makes perfect sense given the fact that connections are initiated from
 *       this dialog, and that the dialog exists during the lifetime of the program.
 */

ServerListDlg::ServerListDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);

  QSettings settings;
  settings.beginGroup("GUI");
  ui.showOnStartup->setChecked(settings.value("ShowServerListOnStartup", true).toBool());
  // create some default entries
  VarMap s1, s2, s3, s4;

  s1["group"] = "Private Servers";
  networks["mindNet"] = s1;
  s2["group"] = "Private Servers";
  networks["fooBar"] = s2;
  s3["group"] = "";
  networks["Single"] = s3;
  s4["group"] = "Public Servers";
  networks["public"] = s4;

  // load networks from QSettings here instead
  // [...]

  // Construct tree widget (and its items) from networks
  QStringList headers;
  headers << "Network" << "Autoconnect";
  ui.networkTree->setHeaderLabels(headers);
  QHash<QString, QTreeWidgetItem *> groups;
  foreach(QString net, networks.keys()) {
    VarMap s = networks[net].toMap();
    QString gr = s["group"].toString();
    QTreeWidgetItem *item = 0;
    if(gr == "") {
      item = new QTreeWidgetItem(ui.networkTree);
    } else {
      if(groups.contains(gr)) {
        item = new QTreeWidgetItem(groups[gr]);
      } else {
        QTreeWidgetItem *newgr = new QTreeWidgetItem(ui.networkTree);
        newgr->setText(0, gr);
        newgr->setFlags(newgr->flags() & ~Qt::ItemIsSelectable);
        ui.networkTree->expandItem(newgr);
        groups[gr] = newgr;
        item = new QTreeWidgetItem(newgr);
      }
    }
    item->setText(0, net);
    //item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(1, Qt::Unchecked);
  }
  connect(ui.networkTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));

  loadIdentities();
  settings.endGroup();
}

ServerListDlg::~ServerListDlg() {

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
  NetworkEditDlg dlg(this, VarMap(), identities);
  if(dlg.exec() == QDialog::Accepted) {

  }
}

void ServerListDlg::loadNetworks() {

}

void ServerListDlg::storeNetworks() {

}

void ServerListDlg::loadIdentities() {
  identities = global->getData("Identities", VarMap()).toMap();
  while(!identities.contains("Default")) {
    editIdentities();
  }
}

void ServerListDlg::storeIdentities() {
  global->putData("Identities", identities);
}

void ServerListDlg::editIdentities() {
  IdentitiesDlg dlg(this, identities);
  if(dlg.exec() == QDialog::Accepted) {
    identities = dlg.getIdentities();
    QMap<QString, QString> mapping = dlg.getNameMapping();
    // add mapping here  <-- well, I don't fucking know anymore what I meant by this back in 2005...

    //
    storeIdentities();
    storeNetworks();  // ? how to treat mapping and NOT save changes not yet applied to the server list?
  }
}

void ServerListDlg::on_showOnStartup_stateChanged(int) {
  QSettings s;
  s.setValue("GUI/ShowServerListOnStartup", ui.showOnStartup->isChecked());
}

/***************************************************************************/

NetworkEditDlg::NetworkEditDlg(QWidget *parent, VarMap _network, VarMap _identities) : QDialog(parent) {
  ui.setupUi(this);
  network = _network;
  identities = _identities;

  ui.identityList->addItem(tr("Default Identity"));

  foreach(QString id, identities.keys()) {
    if(id != "Default") ui.identityList->addItem(id);
  }

}

VarMap NetworkEditDlg::createDefaultNetwork() {
  VarMap net;

  net["group"] = "";

  return net;
}

/***************************************************************************/

IdentitiesDlg::IdentitiesDlg(QWidget *parent, VarMap _identities) : QDialog(parent) {
  ui.setupUi(this);
  connect(global, SIGNAL(dataUpdatedRemotely(QString)), this, SLOT(globalDataUpdated(QString)));

  connect(ui.enableAutoAway, SIGNAL(stateChanged(int)), this, SLOT(autoAwayChecked()));

  identities = _identities;
  foreach(QString name, identities.keys()) {
    nameMapping[name] = name;
  }
  if(identities.size() == 0) {
    VarMap id = createDefaultIdentity();
    id["IdName"] = "Default";
    identities["Default"] = id;
  }
  ui.identityList->addItem(tr("Default Identity"));

  foreach(QString id, identities.keys()) {
    if(id != "Default") ui.identityList->addItem(id);
  }
  updateWidgets();
  lastIdentity = getCurIdentity();
  connect(ui.identityList, SIGNAL(activated(QString)), this, SLOT(identityChanged(QString)));
  connect(ui.editIdentitiesButton, SIGNAL(clicked()), this, SLOT(editIdentities()));
  connect(ui.nickList, SIGNAL(itemSelectionChanged()), this, SLOT(nickSelectionChanged()));
  connect(ui.addNickButton, SIGNAL(clicked()), this, SLOT(addNick()));
  connect(ui.editNickButton, SIGNAL(clicked()), this, SLOT(editNick()));
  connect(ui.delNickButton, SIGNAL(clicked()), this, SLOT(delNick()));
  connect(ui.upNickButton, SIGNAL(clicked()), this, SLOT(upNick()));
  connect(ui.downNickButton, SIGNAL(clicked()), this, SLOT(downNick()));
}

void IdentitiesDlg::globalDataUpdated(QString key) {
  if(key == "Identities") {
    if(QMessageBox::warning(this, tr("Data changed remotely!"), tr("<b>Some other GUI client changed the identities data!</b><br>"
                                                                "Apply updated settings, losing all changes done locally?"),
                                                                QMessageBox::Apply|QMessageBox::Discard) == QMessageBox::Apply) {
      identities = global->getData(key).toMap();
      updateWidgets();
    }
  }
}

VarMap IdentitiesDlg::createDefaultIdentity() {
  VarMap id;
  id["RealName"] = "foo";
  id["Ident"] = "";
  id["NickList"] = QStringList();
  id["enableAwayNick"] = false;
  id["AwayNick"] = "";
  id["enableAwayReason"] = false;
  id["AwayReason"] = "";
  id["enableReturnMessage"] = false;
  id["ReturnMessage"] = "";
  id["enableAutoAway"] = false;
  id["AutoAwayTime"] = 10;
  id["enableAutoAwayReason"] = false;
  id["AutoAwayReason"] = "";
  id["enableAutoAwayReturn"] = false;
  id["AutoAwayReturn"] = "";
  id["PartReason"] = "Quasseling elsewhere.";
  id["QuitReason"] = "Every Quassel comes to its end.";
  id["KickReason"] = "No more quasseling for you!";

  return id;
}

QString IdentitiesDlg::getCurIdentity() {
  if(ui.identityList->currentIndex() == 0) return "Default";
  return ui.identityList->currentText();
}

void IdentitiesDlg::updateWidgets() {
  VarMap id = identities[getCurIdentity()].toMap();
  ui.realNameEdit->setText(id["RealName"].toString());
  ui.identEdit->setText(id["Ident"].toString());
  ui.nickList->clear();
  ui.nickList->addItems(id["NickList"].toStringList());
  if(ui.nickList->count()>0) ui.nickList->setCurrentRow(0);
  ui.enableAwayNick->setChecked(id["enableAwayNick"].toBool());
  ui.awayNickEdit->setText(id["AwayNick"].toString());
  ui.awayNickEdit->setEnabled(ui.enableAwayNick->isChecked());
  ui.enableAwayReason->setChecked(id["enableAwayReason"].toBool());
  ui.awayReasonEdit->setText(id["AwayReason"].toString());
  ui.awayReasonEdit->setEnabled(ui.enableAwayReason->isChecked());
  ui.enableReturnMessage->setChecked(id["enableReturnMessage"].toBool());
  ui.returnMessageEdit->setText(id["ReturnMessage"].toString());
  ui.returnMessageEdit->setEnabled(ui.enableReturnMessage->isChecked());
  ui.enableAutoAway->setChecked(id["enableAutoAway"].toBool());
  ui.autoAwayTime->setValue(id["AutoAwayTime"].toInt());
  ui.enableAutoAwayReason->setChecked(id["enableAutoAwayReason"].toBool());
  ui.autoAwayReasonEdit->setText(id["AutoAwayReason"].toString());
  ui.enableAutoAwayReturn->setChecked(id["enableAutoAwayReturn"].toBool());
  ui.autoAwayReturnEdit->setText(id["AutoAwayReturn"].toString());
  ui.partReasonEdit->setText(id["PartReason"].toString());
  ui.kickReasonEdit->setText(id["KickReason"].toString());
  ui.quitReasonEdit->setText(id["QuitReason"].toString());
  // set enabled states correctly
  autoAwayChecked();
  nickSelectionChanged();
}

void IdentitiesDlg::updateIdentity(QString idName) {
  VarMap id;
  id["RealName"] = ui.realNameEdit->text();
  id["Ident"] = ui.identEdit->text();
  QStringList nicks;
  for(int i = 0; i < ui.nickList->count(); i++) nicks << ui.nickList->item(i)->text();
  id["NickList"] = nicks;
  id["enableAwayNick"] = ui.enableAwayNick->isChecked();
  id["AwayNick"] = ui.awayNickEdit->text();
  id["enableAwayReason"] = ui.enableAwayReason->isChecked();
  id["AwayReason"] = ui.awayReasonEdit->text();
  id["enableReturnMessage"] = ui.enableReturnMessage->isChecked();
  id["ReturnMessage"] = ui.returnMessageEdit->text();
  id["enableAutoAway"] = ui.enableAutoAway->isChecked();
  id["AutoAwayTime"] = ui.autoAwayTime->value();
  id["enableAutoAwayReason"] = ui.enableAutoAwayReason->isChecked();
  id["AutoAwayReason"] = ui.autoAwayReasonEdit->text();
  id["enableAutoAwayReturn"] = ui.enableAutoAwayReturn->isChecked();
  id["AutoAwayReturn"] = ui.autoAwayReturnEdit->text();
  id["PartReason"] = ui.partReasonEdit->text();
  id["KickReason"] = ui.kickReasonEdit->text();
  id["QuitReason"] = ui.quitReasonEdit->text();

  id["IdName"] = idName;
  identities[idName] = id;
}

void IdentitiesDlg::identityChanged(QString) {
  updateIdentity(lastIdentity);
  lastIdentity = getCurIdentity();
  updateWidgets();
}

void IdentitiesDlg::autoAwayChecked() {
  if(ui.enableAutoAway->isChecked()) {
    ui.autoAwayLabel_1->setEnabled(1);
    ui.autoAwayLabel_2->setEnabled(1);
    ui.autoAwayTime->setEnabled(1);
    ui.enableAutoAwayReason->setEnabled(1);
    ui.enableAutoAwayReturn->setEnabled(1);
    ui.autoAwayReasonEdit->setEnabled(ui.enableAutoAwayReason->isChecked());
    ui.autoAwayReturnEdit->setEnabled(ui.enableAutoAwayReturn->isChecked());
  } else {
    ui.autoAwayLabel_1->setEnabled(0);
    ui.autoAwayLabel_2->setEnabled(0);
    ui.autoAwayTime->setEnabled(0);
    ui.enableAutoAwayReason->setEnabled(0);
    ui.enableAutoAwayReturn->setEnabled(0);
    ui.autoAwayReasonEdit->setEnabled(0);
    ui.autoAwayReturnEdit->setEnabled(0);
  }
}

void IdentitiesDlg::nickSelectionChanged() {
  int curidx = ui.nickList->currentRow();
  ui.editNickButton->setEnabled(curidx >= 0);
  ui.delNickButton->setEnabled(curidx >= 0);
  ui.upNickButton->setEnabled(curidx > 0);
  ui.downNickButton->setEnabled(curidx >= 0 && curidx < ui.nickList->count() - 1);
}

void IdentitiesDlg::addNick() {
  NickEditDlg dlg(this);
  if(dlg.exec() == QDialog::Accepted) {
    QListWidgetItem *item = new QListWidgetItem(ui.nickList);
    item->setText(dlg.getNick());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui.nickList->setCurrentItem(item);
    nickSelectionChanged();
  }
}

void IdentitiesDlg::editNick() {
  NickEditDlg dlg(this, ui.nickList->currentItem()->text());
  if(dlg.exec() == QDialog::Accepted) {
    ui.nickList->currentItem()->setText(dlg.getNick());
  }
}

void IdentitiesDlg::delNick() {
  int row = ui.nickList->currentRow();
  delete ui.nickList->takeItem(row);
  if(row <= ui.nickList->count() - 1) ui.nickList->setCurrentRow(row);
  else if(row > 0) ui.nickList->setCurrentRow(ui.nickList->count()-1);
  nickSelectionChanged();
}

void IdentitiesDlg::upNick() {
  int row = ui.nickList->currentRow();
  QListWidgetItem *item = ui.nickList->takeItem(row);
  ui.nickList->insertItem(row-1, item);
  ui.nickList->setCurrentRow(row-1);
  nickSelectionChanged();
}

void IdentitiesDlg::downNick() {
  int row = ui.nickList->currentRow();
  QListWidgetItem *item = ui.nickList->takeItem(row);
  ui.nickList->insertItem(row+1, item);
  ui.nickList->setCurrentRow(row+1);
  nickSelectionChanged();
}

void IdentitiesDlg::accept() {
  updateIdentity(getCurIdentity());
  QString result = checkValidity();
  if(result.length() == 0) QDialog::accept();
  else {
    QMessageBox::warning(this, tr("Invalid Identity!"),
                         tr("One or more of your identities do not contain all necessary information:\n\n%1\n"
                             "Please fill in any missing information.").arg(result));
  }
}

QString IdentitiesDlg::checkValidity() {
  QString reason;
  foreach(QString name, identities.keys()) {
    QString r;
    VarMap id = identities[name].toMap();
    if(name == "Default") name = tr("Default Identity");
    if(id["RealName"].toString().length() == 0) {
      r += tr(" You have not set a real name.");
    }
    if(id["Ident"].toString().length() == 0) {
      r += tr(" You have to specify an Ident.");
    }
    if(id["NickList"].toStringList().size() == 0) {
      r += tr(" You haven't entered any nicknames.");
    }
    if(r.length()>0) {
      reason += tr("[%1]%2\n").arg(name).arg(r);
    }
  }
  return reason;
}

void IdentitiesDlg::editIdentities() {
  updateIdentity(getCurIdentity());
  IdentitiesEditDlg dlg(this, identities, nameMapping, createDefaultIdentity());
  if(dlg.exec() == QDialog::Accepted) {
    identities = dlg.getIdentities();
    nameMapping = dlg.getMapping();
    ui.identityList->clear();
    ui.identityList->addItem(tr("Default Identity"));
    foreach(QString id, identities.keys()) {
      if(id != "Default") ui.identityList->addItem(id);
    }
    lastIdentity = getCurIdentity();
    updateWidgets();
  }
}

/******************************************************************************/

IdentitiesEditDlg::IdentitiesEditDlg(QWidget *parent, VarMap _identities, QMap<QString, QString> _mapping, VarMap templ)
  : QDialog(parent) {
  ui.setupUi(this);
  identities = _identities;
  mapping = _mapping;
  identTemplate = templ;

  foreach(QString name, identities.keys()) {
    if(name == "Default") continue;
    ui.identList->addItem(name);
  }
  ui.identList->sortItems();
  ui.identList->insertItem(0, tr("Default Identity"));
  ui.identList->setCurrentRow(0);
  selectionChanged();
  connect(ui.identList, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));
  connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addIdentity()));
  connect(ui.duplicateButton, SIGNAL(clicked()), this, SLOT(duplicateIdentity()));
  connect(ui.renameButton, SIGNAL(clicked()), this, SLOT(renameIdentity()));
  connect(ui.deleteButton, SIGNAL(clicked()), this, SLOT(deleteIdentity()));
}

void IdentitiesEditDlg::selectionChanged() {
  int idx = ui.identList->currentRow();
  ui.duplicateButton->setEnabled(idx >= 0);
  ui.renameButton->setEnabled(idx > 0);
  ui.deleteButton->setEnabled(idx > 0);

}

void IdentitiesEditDlg::addIdentity() {
  RenameIdentityDlg dlg(this, identities.keys());
  if(dlg.exec() == QDialog::Accepted) {
    VarMap id = identTemplate;
    identities[dlg.getName()] = id;
    QListWidgetItem *item = new QListWidgetItem(dlg.getName(), ui.identList);
    sortList();
    ui.identList->setCurrentItem(item);
    selectionChanged();
  }
}

void IdentitiesEditDlg::duplicateIdentity() {
  RenameIdentityDlg dlg(this, identities.keys());
  if(dlg.exec() == QDialog::Accepted) {
    QString curname = ui.identList->currentRow() == 0 ? "Default" : ui.identList->currentItem()->text();
    QVariant id = identities[curname];
    identities[dlg.getName()] = id;
    QListWidgetItem *item = new QListWidgetItem(dlg.getName(), ui.identList);
    sortList();
    ui.identList->setCurrentItem(item);
    selectionChanged();
  }
}

void IdentitiesEditDlg::renameIdentity() {
  QList<QString> names;
  QString curname = ui.identList->currentItem()->text();
  foreach(QString n, identities.keys()) {
    if(n != curname) names.append(n);
  }
  RenameIdentityDlg dlg(this, names, curname);
  if(dlg.exec() == QDialog::Accepted) {
    QString newname = dlg.getName();
    foreach(QString key, mapping.keys()) {
      if(mapping[key] == curname) {
        mapping[key] = newname;
        break;
      }
    }
    QVariant id = identities.take(curname);
    identities[newname] = id;
    QListWidgetItem *item = ui.identList->currentItem();
    item->setText(newname);
    sortList();
    ui.identList->setCurrentItem(item);
    selectionChanged();
  }
}

void IdentitiesEditDlg::deleteIdentity() {
  QString curname = ui.identList->currentItem()->text();
  if(QMessageBox::question(this, tr("Delete Identity?"),
     tr("Do you really want to delete identity \"%1\"?\nNetworks using this identity "
        "will be reset to use the default identity.").arg(curname),
    tr("&Delete"), tr("&Cancel"), QString(), 1, 1) == 0) {
      delete ui.identList->takeItem(ui.identList->currentRow());
      foreach(QString key, mapping.keys()) {
        if(mapping[key] == curname) {
          mapping.remove(key); break;
        }
      }
      identities.remove(curname);
      selectionChanged();
  }
}

void IdentitiesEditDlg::sortList() {
  QListWidgetItem *def = ui.identList->takeItem(0);
  ui.identList->sortItems();
  ui.identList->insertItem(0, def);
}

/******************************************************************************/

NickEditDlg::NickEditDlg(QWidget *parent, QString nick) : QDialog(parent) {
  ui.setupUi(this);
  ui.lineEdit->setText(nick);
  connect(ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
  textChanged(nick);
}

void NickEditDlg::textChanged(QString text) {
  ui.okButton->setDisabled(text.isEmpty() || text == "");
}

QString NickEditDlg::getNick() {
  return ui.lineEdit->text();
}

/*******************************************************************************/

RenameIdentityDlg::RenameIdentityDlg(QWidget *parent, QList<QString> _reserved, QString name) : QDialog(parent) {
  ui.setupUi(this);
  reserved = _reserved;
  //ui.NickEditDlg->setWindowTitle(tr("Edit Identity Name"));  // why does this not work?
  ui.label->setText(tr("Identity:"));
  ui.lineEdit->setText(name);
  connect(ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
  textChanged(name);
}

void RenameIdentityDlg::textChanged(QString text) {
  if(text.length() == 0) { ui.okButton->setEnabled(0); return; }
  ui.okButton->setDisabled(reserved.contains(text));
}

QString RenameIdentityDlg::getName() {
  return ui.lineEdit->text();
}

