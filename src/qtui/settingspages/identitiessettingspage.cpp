/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QMessageBox>

#include "identitiessettingspage.h"

#include "client.h"

IdentitiesSettingsPage::IdentitiesSettingsPage(QWidget *parent)
  : SettingsPage(tr("General"), tr("Identities"), parent) {

  ui.setupUi(this);
  setEnabled(false);  // need a core connection!
  connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));
  connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientIdentityCreated(IdentityId)));
  connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientIdentityRemoved(IdentityId)));

  currentId = 0;

  // We need to know whenever the state of input widgets changes...
  //connect(ui.identityList, SIGNAL(editTextChanged(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.realName, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.nicknameList, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(widgetHasChanged()));
  connect(ui.awayNick, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.awayNickEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.awayReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.awayReasonEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.returnMessage, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.returnMessageEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayTime, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayReasonEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.autoReturnMessage, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.autoReturnMessageEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.ident, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.kickReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.partReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.quitReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));

}

void IdentitiesSettingsPage::coreConnectionStateChanged(bool state) {
  this->setEnabled(state);
  if(state) {
    load();
  } else {
    // reset
    currentId = 0;
  }
}

void IdentitiesSettingsPage::save() {
  setEnabled(false);
  QList<Identity *> toCreate, toUpdate;
  // we need to remove our temporarily created identities.
  // these are going to be re-added after the core has propagated them back...
  for(QHash<IdentityId, Identity *>::iterator i = identities.begin(); i != identities.end(); ++i) {
    if((*i)->id() < 0) {
      Identity *temp = *i;
      i = identities.erase(i);
      toCreate.append(temp);
      ui.identityList->removeItem(ui.identityList->findData(temp->id()));
    } else {
      if(**i != *Client::identity((*i)->id())) {
        toUpdate.append(*i);
      }
    }
  }
  SaveIdentitiesDlg dlg(toCreate, toUpdate, deletedIdentities, this);
  int ret = dlg.exec();
  if(ret == QDialog::Rejected) {
    // canceled -> reload everything to be safe
    load();
  }
  foreach(Identity *id, toCreate) {
    id->deleteLater();
  }
  changeState(false);
  setEnabled(true);

}

void IdentitiesSettingsPage::load() {
  currentId = 0;
  foreach(Identity *identity, identities.values()) {
    identity->deleteLater();
  }
  identities.clear();
  deletedIdentities.clear();
  changedIdentities.clear();
  ui.identityList->clear();
  foreach(IdentityId id, Client::identityIds()) {
    clientIdentityCreated(id);
  }
  changeState(false);
}

void IdentitiesSettingsPage::defaults() {
  // TODO implement bool hasDefaults()
}

void IdentitiesSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) changeState(changed);
}

bool IdentitiesSettingsPage::testHasChanged() {
  if(deletedIdentities.count()) return true;
  if(currentId < 0) {
    return true; // new identity
  } else {
    changedIdentities.removeAll(currentId);
    Identity temp(currentId, this);
    saveToIdentity(&temp);
    if(temp != *identities[currentId]) changedIdentities.append(currentId);
    return changedIdentities.count();
  }
}

bool IdentitiesSettingsPage::aboutToSave() {
  saveToIdentity(identities[currentId]);
  QList<int> errors;
  foreach(Identity *id, identities.values()) {
    if(id->identityName().isEmpty()) errors.append(1);
    if(!id->nicks().count()) errors.append(2);
    if(id->realName().isEmpty()) errors.append(3);
    if(id->ident().isEmpty()) errors.append(4);
  }
  if(!errors.count()) return true;
  QString error(tr("<b>The following problems need to be corrected before your changes can be applied:</b><ul>"));
  if(errors.contains(1)) error += tr("<li>All identities need an identity name set</li>");
  if(errors.contains(2)) error += tr("<li>Every identity needs at least one nickname defined</li>");
  if(errors.contains(3)) error += tr("<li>You need to specify a real name for every identity</li>");
  if(errors.contains(4)) error += tr("<li>You need to specify an ident for every identity</li>");
  error += tr("</ul>");
  QMessageBox::warning(this, tr("One or more identities are invalid"), error);
  return false;
}

void IdentitiesSettingsPage::clientIdentityCreated(IdentityId id) {
  insertIdentity(new Identity(*Client::identity(id), this));
  connect(Client::identity(id), SIGNAL(updatedRemotely()), this, SLOT(clientIdentityUpdated()));
}

void IdentitiesSettingsPage::clientIdentityUpdated() {
  Identity *identity = qobject_cast<Identity *>(sender());
  if(!identity) {
    qWarning() << "Invalid identity to update!";
    return;
  }
  if(!identities.contains(identity->id())) {
    qWarning() << "Unknown identity to update:" << identity->identityName();
    return;
  }
  identities[identity->id()]->update(*identity);
  ui.identityList->setItemText(ui.identityList->findData(identity->id()), identity->identityName());
  if(identity->id() == currentId) displayIdentity(identity, true);
}

void IdentitiesSettingsPage::clientIdentityRemoved(IdentityId id) {
  if(identities.contains(id)) {
    removeIdentity(identities[id]);
    changedIdentities.removeAll(id);
    deletedIdentities.removeAll(id);
  }
}

void IdentitiesSettingsPage::insertIdentity(Identity *identity) {
  IdentityId id = identity->id();
  identities[id] = identity;
  if(id == 1) {
    // default identity is always the first one!
    ui.identityList->insertItem(0, identity->identityName(), id);
  } else {
    QString name = identity->identityName();
    for(int j = 0; j < ui.identityList->count(); j++) {
      if((j>0 || ui.identityList->itemData(0).toInt() != 1) && name.localeAwareCompare(ui.identityList->itemText(j)) < 0) {
        ui.identityList->insertItem(j, name, id);
        widgetHasChanged();
        return;
      }
    }
    // append
    ui.identityList->insertItem(ui.identityList->count(), name, id);
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::removeIdentity(Identity *id) {
  ui.identityList->removeItem(ui.identityList->findData(id->id()));
  identities.remove(id->id());
  id->deleteLater();
  widgetHasChanged();
}

void IdentitiesSettingsPage::on_identityList_currentIndexChanged(int index) {
  if(index < 0) {
    //ui.identityList->setEditable(false);
    displayIdentity(0);
  } else {
    IdentityId id = ui.identityList->itemData(index).toInt();
    if(identities.contains(id)) displayIdentity(identities[id]);
    ui.deleteIdentity->setEnabled(id != 1); // default identity cannot be deleted
    //ui.identityList->setEditable(id != 1);  // ...or renamed
  }
}

void IdentitiesSettingsPage::displayIdentity(Identity *id, bool dontsave) {
  if(currentId != 0 && !dontsave && identities.contains(currentId)) {
    saveToIdentity(identities[currentId]);
  }
  if(id) {
    currentId = id->id();
    ui.realName->setText(id->realName());
    ui.nicknameList->clear();
    ui.nicknameList->addItems(id->nicks());
    //for(int i = 0; i < ui.nicknameList->count(); i++) {
    //  ui.nicknameList->item(i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    //}
    if(ui.nicknameList->count()) ui.nicknameList->setCurrentRow(0);
    ui.awayNick->setText(id->awayNick());
    ui.awayNickEnabled->setChecked(id->awayNickEnabled());
    ui.awayReason->setText(id->awayReason());
    ui.awayReasonEnabled->setChecked(id->awayReasonEnabled());
    ui.returnMessage->setText(id->returnMessage());
    ui.returnMessageEnabled->setChecked(id->returnMessageEnabled());
    ui.autoAwayEnabled->setChecked(id->autoAwayEnabled());
    ui.autoAwayTime->setValue(id->autoAwayTime());
    ui.autoAwayReason->setText(id->autoAwayReason());
    ui.autoAwayReasonEnabled->setChecked(id->autoAwayReasonEnabled());
    ui.autoReturnMessage->setText(id->autoReturnMessage());
    ui.autoReturnMessageEnabled->setChecked(id->autoReturnMessageEnabled());
    ui.ident->setText(id->ident());
    ui.kickReason->setText(id->kickReason());
    ui.partReason->setText(id->partReason());
    ui.quitReason->setText(id->quitReason());
  }
}

void IdentitiesSettingsPage::saveToIdentity(Identity *id) {
  id->setIdentityName(ui.identityList->currentText());
  id->setRealName(ui.realName->text());
  QStringList nicks;
  for(int i = 0; i < ui.nicknameList->count(); i++) {
    nicks << ui.nicknameList->item(i)->text();
  }
  id->setNicks(nicks);
  id->setAwayNick(ui.awayNick->text());
  id->setAwayNickEnabled(ui.awayNickEnabled->isChecked());
  id->setAwayReason(ui.awayReason->text());
  id->setAwayReasonEnabled(ui.awayReasonEnabled->isChecked());
  id->setReturnMessage(ui.returnMessage->text());
  id->setReturnMessageEnabled(ui.returnMessageEnabled->isChecked());
  id->setAutoAwayEnabled(ui.autoAwayEnabled->isChecked());
  id->setAutoAwayTime(ui.autoAwayTime->value());
  id->setAutoAwayReason(ui.autoAwayReason->text());
  id->setAutoAwayReasonEnabled(ui.autoAwayReasonEnabled->isChecked());
  id->setAutoReturnMessage(ui.autoReturnMessage->text());
  id->setAutoReturnMessageEnabled(ui.autoReturnMessageEnabled->isChecked());
  id->setIdent(ui.ident->text());
  id->setKickReason(ui.kickReason->text());
  id->setPartReason(ui.partReason->text());
  id->setQuitReason(ui.quitReason->text());
}

void IdentitiesSettingsPage::on_addIdentity_clicked() {
  CreateIdentityDlg dlg(ui.identityList->model(), this);
  if(dlg.exec() == QDialog::Accepted) {
    // find a free (negative) ID
    IdentityId id;
    for(id = 1; id <= identities.count(); id++) {
      if(!identities.keys().contains(-id)) break;
    }
    id *= -1;
    Identity *newId = new Identity(id, this);
    if(dlg.duplicateId() != 0) {
      // duplicate
      newId->update(*identities[dlg.duplicateId()]);
      newId->setId(id);
    }
    newId->setIdentityName(dlg.identityName());
    identities[id] = newId;
    insertIdentity(newId);
    ui.identityList->setCurrentIndex(ui.identityList->findData(id));
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::on_deleteIdentity_clicked() {
  Identity *id = identities[currentId];
  int ret = QMessageBox::question(this, tr("Delete Identity?"),
                                  tr("Do you really want to delete identity \"%1\"?").arg(id->identityName()),
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(ret != QMessageBox::Yes) return;
  if(id->id() > 0) deletedIdentities.append(id->id());
  removeIdentity(id);
}

void IdentitiesSettingsPage::on_identityList_editTextChanged(const QString &text) {
  ui.identityList->setItemText(ui.identityList->currentIndex(), text);
}

/*****************************************************************************************/

CreateIdentityDlg::CreateIdentityDlg(QAbstractItemModel *model, QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);

  ui.identityList->setModel(model);  // now we use the identity list of the main page... Trolltech <3
  on_identityName_textChanged("");   // disable ok button :)
}

QString CreateIdentityDlg::identityName() const {
  return ui.identityName->text();
}

IdentityId CreateIdentityDlg::duplicateId() const {
  if(!ui.duplicateIdentity->isChecked()) return 0;
  if(ui.identityList->currentIndex() >= 0) {
    return ui.identityList->itemData(ui.identityList->currentIndex()).toInt();
  }
  return 0;
}

void CreateIdentityDlg::on_identityName_textChanged(const QString &text) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text.count());

}

/*********************************************************************************************/

SaveIdentitiesDlg::SaveIdentitiesDlg(QList<Identity *> tocreate, QList<Identity *> toupdate, QList<IdentityId> toremove, QWidget *parent)
  : QDialog(parent), toCreate(tocreate), toUpdate(toupdate), toRemove(toremove) {
  ui.setupUi(this);
  numevents = toCreate.count() + toUpdate.count() + toRemove.count();
  rcvevents = 0;
  if(numevents) {
    ui.progressBar->setMaximum(numevents);
    ui.progressBar->setValue(0);

    connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientEvent()));
    connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientEvent()));

    foreach(Identity *id, toCreate) {
      Client::createIdentity(*id);
    }
    foreach(Identity *id, toUpdate) {
      const Identity *cid = Client::identity(id->id());
      if(!cid) {
        qWarning() << "Invalid client identity!";
        numevents--;
        continue;
      }
      connect(cid, SIGNAL(updatedRemotely()), this, SLOT(clientEvent()));
      Client::updateIdentity(*id);
    }
    foreach(IdentityId id, toRemove) {
      Client::removeIdentity(id);
    }
  } else {
    qWarning() << "Sync dialog called without stuff to change!";
    accept();
  }
}

void SaveIdentitiesDlg::clientEvent() {
  ui.progressBar->setValue(++rcvevents);
  if(rcvevents >= numevents) accept();
}
