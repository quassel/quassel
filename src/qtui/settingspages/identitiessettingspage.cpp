/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "identitiessettingspage.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "client.h"
#include "iconloader.h"
#include "signalproxy.h"

IdentitiesSettingsPage::IdentitiesSettingsPage(QWidget *parent)
  : SettingsPage(tr("General"), tr("Identities"), parent),
    _editSsl(false)
{

  ui.setupUi(this);
  ui.renameIdentity->setIcon(BarIcon("edit-rename"));
  ui.addIdentity->setIcon(BarIcon("list-add-user"));
  ui.deleteIdentity->setIcon(BarIcon("list-remove-user"));
  ui.addNick->setIcon(SmallIcon("list-add"));
  ui.deleteNick->setIcon(SmallIcon("edit-delete"));
  ui.renameNick->setIcon(SmallIcon("edit-rename"));
  ui.nickUp->setIcon(SmallIcon("go-up"));
  ui.nickDown->setIcon(SmallIcon("go-down"));

  coreConnectionStateChanged(Client::isConnected());  // need a core connection!
  setWidgetStates();
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
  connect(ui.autoAwayEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayTime, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.autoAwayReasonEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.detachAwayEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.detachAwayReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.detachAwayReasonEnabled, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.ident, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.kickReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.partReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));
  connect(ui.quitReason, SIGNAL(textEdited(const QString &)), this, SLOT(widgetHasChanged()));

  connect(ui.nicknameList, SIGNAL(itemSelectionChanged()), this, SLOT(setWidgetStates()));

  // we would need this if we enabled drag and drop in the nicklist...
  //connect(ui.nicknameList, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(setWidgetStates()));
  //connect(ui.nicknameList->model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(nicklistHasChanged()));

#ifdef HAVE_SSL
  ui.sslKeyGroupBox->setAcceptDrops(true);
  ui.sslKeyGroupBox->installEventFilter(this);
  ui.sslCertGroupBox->setAcceptDrops(true);
  ui.sslCertGroupBox->installEventFilter(this);
#endif
}

void IdentitiesSettingsPage::setWidgetStates() {
  if(ui.nicknameList->selectedItems().count()) {
    ui.renameNick->setEnabled(true);
    ui.nickUp->setEnabled(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]) > 0);
    ui.nickDown->setEnabled(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]) < ui.nicknameList->count()-1);
  } else {
    ui.renameNick->setDisabled(true);
    ui.nickUp->setDisabled(true);
    ui.nickDown->setDisabled(true);
  }
  ui.deleteNick->setEnabled(ui.nicknameList->count() > 1);
}

void IdentitiesSettingsPage::coreConnectionStateChanged(bool connected) {
  setEnabled(connected);
  if(connected) {
#ifdef HAVE_SSL
    if(Client::signalProxy()->isSecure()) {
      ui.keyAndCertSettings->setCurrentIndex(2);
      _editSsl = true;
    } else {
      ui.keyAndCertSettings->setCurrentIndex(1);
      _editSsl = false;
    }
#else
    ui.keyAndCertSettings->setCurrentIndex(0);
#endif
    load();
  } else {
    // reset
    currentId = 0;
  }
}

void IdentitiesSettingsPage::save() {
  setEnabled(false);
  QList<CertIdentity *> toCreate, toUpdate;
  // we need to remove our temporarily created identities.
  // these are going to be re-added after the core has propagated them back...
  QHash<IdentityId, CertIdentity *>::iterator i = identities.begin();
  while(i != identities.end()) {
    if((*i)->id() < 0) {
      CertIdentity *temp = *i;
      i = identities.erase(i);
      toCreate.append(temp);
      ui.identityList->removeItem(ui.identityList->findData(temp->id().toInt()));
    } else {
      if(**i != *Client::identity((*i)->id()) || (*i)->isDirty()) {
        toUpdate.append(*i);
      }
      ++i;
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
  changedIdentities.clear();
  deletedIdentities.clear();
  setChangedState(false);
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
  setChangedState(false);
}

void IdentitiesSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool IdentitiesSettingsPage::testHasChanged() {
  if(deletedIdentities.count()) return true;
  if(currentId < 0) {
    return true; // new identity
  } else {
    if(currentId != 0) {
      changedIdentities.removeAll(currentId);
      CertIdentity temp(currentId, this);
      saveToIdentity(&temp);
      temp.setIdentityName(identities[currentId]->identityName());
      if(temp != *Client::identity(currentId) || temp.isDirty())
	changedIdentities.append(currentId);
    }
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
  CertIdentity *identity = new CertIdentity(*Client::identity(id), this);
#ifdef HAVE_SSL
  identity->enableEditSsl(_editSsl);
#endif
  insertIdentity(identity);
#ifdef HAVE_SSL
  connect(identity, SIGNAL(sslSettingsUpdated()), this, SLOT(clientIdentityUpdated()));
#endif
  connect(Client::identity(id), SIGNAL(updatedRemotely()), this, SLOT(clientIdentityUpdated()));
}

void IdentitiesSettingsPage::clientIdentityUpdated() {
  const Identity *clientIdentity = qobject_cast<Identity *>(sender());
  if(!clientIdentity) {
    qWarning() << "Invalid identity to update!";
    return;
  }
  if(!identities.contains(clientIdentity->id())) {
    qWarning() << "Unknown identity to update:" << clientIdentity->identityName();
    return;
  }

  CertIdentity *identity = identities[clientIdentity->id()];

  if(identity->identityName() != clientIdentity->identityName())
    renameIdentity(identity->id(), clientIdentity->identityName());

  identity->copyFrom(*clientIdentity);

  if(identity->id() == currentId)
    displayIdentity(identity, true);
}

void IdentitiesSettingsPage::clientIdentityRemoved(IdentityId id) {
  if(identities.contains(id)) {
    removeIdentity(identities[id]);
    changedIdentities.removeAll(id);
    deletedIdentities.removeAll(id);
  }
}

void IdentitiesSettingsPage::insertIdentity(CertIdentity *identity) {
  IdentityId id = identity->id();
  identities[id] = identity;
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

void IdentitiesSettingsPage::renameIdentity(IdentityId id, const QString &newName) {
  Identity *identity = identities[id];
  ui.identityList->setItemText(ui.identityList->findData(identity->id().toInt()), newName);
  identity->setIdentityName(newName);
}

void IdentitiesSettingsPage::removeIdentity(Identity *id) {
  identities.remove(id->id());
  ui.identityList->removeItem(ui.identityList->findData(id->id().toInt()));
  changedIdentities.removeAll(id->id());
  if(currentId == id->id()) currentId = 0;
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
    ui.renameIdentity->setEnabled(id != 1); // ...or renamed
  }
}

void IdentitiesSettingsPage::displayIdentity(CertIdentity *id, bool dontsave) {
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
    ui.autoAwayEnabled->setChecked(id->autoAwayEnabled());
    ui.autoAwayTime->setValue(id->autoAwayTime());
    ui.autoAwayReason->setText(id->autoAwayReason());
    ui.autoAwayReasonEnabled->setChecked(id->autoAwayReasonEnabled());
    ui.detachAwayEnabled->setChecked(id->detachAwayEnabled());
    ui.detachAwayReason->setText(id->detachAwayReason());
    ui.detachAwayReasonEnabled->setChecked(id->detachAwayReasonEnabled());
    ui.ident->setText(id->ident());
    ui.kickReason->setText(id->kickReason());
    ui.partReason->setText(id->partReason());
    ui.quitReason->setText(id->quitReason());
#ifdef HAVE_SSL
    showKeyState(id->sslKey());
    showCertState(id->sslCert());
#endif
  }
}

void IdentitiesSettingsPage::saveToIdentity(CertIdentity *id) {
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
  id->setAutoAwayEnabled(ui.autoAwayEnabled->isChecked());
  id->setAutoAwayTime(ui.autoAwayTime->value());
  id->setAutoAwayReason(ui.autoAwayReason->text());
  id->setAutoAwayReasonEnabled(ui.autoAwayReasonEnabled->isChecked());
  id->setDetachAwayEnabled(ui.detachAwayEnabled->isChecked());
  id->setDetachAwayReason(ui.detachAwayReason->text());
  id->setDetachAwayReasonEnabled(ui.detachAwayReasonEnabled->isChecked());
  id->setIdent(ui.ident->text());
  id->setKickReason(ui.kickReason->text());
  id->setPartReason(ui.partReason->text());
  id->setQuitReason(ui.quitReason->text());
#ifdef HAVE_SSL
  id->setSslKey(QSslKey(ui.keyTypeLabel->property("sslKey").toByteArray(), (QSsl::KeyAlgorithm)(ui.keyTypeLabel->property("sslKeyType").toInt())));
  id->setSslCert(QSslCertificate(ui.certOrgLabel->property("sslCert").toByteArray()));
#endif
}

void IdentitiesSettingsPage::on_addIdentity_clicked() {
  CreateIdentityDlg dlg(ui.identityList->model(), this);
  if(dlg.exec() == QDialog::Accepted) {
    // find a free (negative) ID
    IdentityId id;
    for(id = 1; id <= identities.count(); id++) {
      if(!identities.keys().contains(-id.toInt())) break;
    }
    id = -id.toInt();
    CertIdentity *newId = new CertIdentity(id, this);
#ifdef HAVE_SSL
    newId->enableEditSsl(_editSsl);
#endif
    if(dlg.duplicateId() != 0) {
      // duplicate
      newId->copyFrom(*identities[dlg.duplicateId()]);
      newId->setId(id);
    }
    newId->setIdentityName(dlg.identityName());
    identities[id] = newId;
    insertIdentity(newId);
    ui.identityList->setCurrentIndex(ui.identityList->findData(id.toInt()));
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
  currentId = 0;
  removeIdentity(id);
}

void IdentitiesSettingsPage::on_renameIdentity_clicked() {
  QString oldName = identities[currentId]->identityName();
  bool ok = false;
  QString name = QInputDialog::getText(this, tr("Rename Identity"),
                                       tr("Please enter a new name for the identity \"%1\"!").arg(oldName),
                                       QLineEdit::Normal, oldName, &ok);
  if(ok && !name.isEmpty()) {
    renameIdentity(currentId, name);
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::on_addNick_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.nicknameList->count(); i++) existing << ui.nicknameList->item(i)->text();
  NickEditDlg dlg(QString(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    ui.nicknameList->addItem(dlg.nick());
    ui.nicknameList->setCurrentRow(ui.nicknameList->count()-1);
    setWidgetStates();
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::on_deleteNick_clicked() {
  // no confirmation, since a nickname is really nothing hard to recreate
  if(ui.nicknameList->selectedItems().count()) {
    delete ui.nicknameList->takeItem(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]));
    ui.nicknameList->setCurrentRow(qMin(ui.nicknameList->currentRow()+1, ui.nicknameList->count()-1));
    setWidgetStates();
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::on_renameNick_clicked() {
  if(!ui.nicknameList->selectedItems().count()) return;
  QString old = ui.nicknameList->selectedItems()[0]->text();
  QStringList existing;
  for(int i = 0; i < ui.nicknameList->count(); i++) existing << ui.nicknameList->item(i)->text();
  NickEditDlg dlg(old, existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    ui.nicknameList->selectedItems()[0]->setText(dlg.nick());
  }

}

void IdentitiesSettingsPage::on_nickUp_clicked() {
  if(!ui.nicknameList->selectedItems().count()) return;
  int row = ui.nicknameList->row(ui.nicknameList->selectedItems()[0]);
  if(row > 0) {
    ui.nicknameList->insertItem(row-1, ui.nicknameList->takeItem(row));
    ui.nicknameList->setCurrentRow(row-1);
    setWidgetStates();
    widgetHasChanged();
  }
}

void IdentitiesSettingsPage::on_nickDown_clicked() {
  if(!ui.nicknameList->selectedItems().count()) return;
  int row = ui.nicknameList->row(ui.nicknameList->selectedItems()[0]);
  if(row < ui.nicknameList->count()-1) {
    ui.nicknameList->insertItem(row+1, ui.nicknameList->takeItem(row));
    ui.nicknameList->setCurrentRow(row+1);
    setWidgetStates();
    widgetHasChanged();
  }
}

#ifdef HAVE_SSL
void IdentitiesSettingsPage::on_continueUnsecured_clicked() {
  _editSsl = true;

  QHash<IdentityId, CertIdentity *>::iterator idIter;
  for(idIter = identities.begin(); idIter != identities.end(); idIter++) {
    idIter.value()->enableEditSsl();
  }

  ui.keyAndCertSettings->setCurrentIndex(2);
}

bool IdentitiesSettingsPage::eventFilter(QObject *watched, QEvent *event) {
  bool isCert = (watched == ui.sslCertGroupBox);
  switch(event->type()) {
  case QEvent::DragEnter:
    sslDragEnterEvent(static_cast<QDragEnterEvent *>(event));
    return true;
  case QEvent::Drop:
    sslDropEvent(static_cast<QDropEvent *>(event), isCert);
    return true;
  default:
    return false;
  }
}

void IdentitiesSettingsPage::sslDragEnterEvent(QDragEnterEvent *event) {
  if(event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("text/uri")) {
    event->setDropAction(Qt::CopyAction);
    event->accept();
  }
}

void IdentitiesSettingsPage::sslDropEvent(QDropEvent *event, bool isCert) {
  QByteArray rawUris;
  if(event->mimeData()->hasFormat("text/uri-list"))
    rawUris = event->mimeData()->data("text/uri-list");
  else
    rawUris = event->mimeData()->data("text/uri");

  QTextStream uriStream(rawUris);
  QString filename = QUrl(uriStream.readLine()).toLocalFile();

  if(isCert) {
    QSslCertificate cert = certByFilename(filename);
    if(cert.isValid())
      showCertState(cert);
  } else {
    QSslKey key = keyByFilename(filename);
    if(!key.isNull())
      showKeyState(key);
  }
  event->accept();
  widgetHasChanged();
}

void IdentitiesSettingsPage::on_clearOrLoadKeyButton_clicked() {
  QSslKey key;

  if(ui.keyTypeLabel->property("sslKey").toByteArray().isEmpty())
    key = keyByFilename(QFileDialog::getOpenFileName(this, tr("Load a Key"), QDesktopServices::storageLocation(QDesktopServices::HomeLocation)));

  showKeyState(key);
  widgetHasChanged();
}

QSslKey IdentitiesSettingsPage::keyByFilename(const QString &filename) {
  QSslKey key;

  QFile keyFile(filename);
  keyFile.open(QIODevice::ReadOnly);
  QByteArray keyRaw = keyFile.read(2 << 20);
  keyFile.close();

  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 2; j++) {
      key = QSslKey(keyRaw, (QSsl::KeyAlgorithm)j, (QSsl::EncodingFormat)i);
      if(!key.isNull())
	goto returnKey;
    }
  }
 returnKey:
  return key;
}

void IdentitiesSettingsPage::showKeyState(const QSslKey &key) {
  if(key.isNull()) {
    ui.keyTypeLabel->setText(tr("No Key loaded"));
    ui.clearOrLoadKeyButton->setText(tr("Load"));
  } else {
    switch(key.algorithm()) {
    case QSsl::Rsa:
      ui.keyTypeLabel->setText(tr("RSA"));
      break;
    case QSsl::Dsa:
      ui.keyTypeLabel->setText(tr("DSA"));
      break;
    default:
      ui.keyTypeLabel->setText(tr("No Key loaded"));
    }
    ui.clearOrLoadKeyButton->setText(tr("Clear"));
  }
  ui.keyTypeLabel->setProperty("sslKey", key.toPem());
  ui.keyTypeLabel->setProperty("sslKeyType", (int)key.algorithm());
}

void IdentitiesSettingsPage::on_clearOrLoadCertButton_clicked() {
  QSslCertificate cert;

  if(ui.certOrgLabel->property("sslCert").toByteArray().isEmpty())
    cert = certByFilename(QFileDialog::getOpenFileName(this, tr("Load a Certificate"), QDesktopServices::storageLocation(QDesktopServices::HomeLocation)));

  showCertState(cert);
  widgetHasChanged();
}

QSslCertificate IdentitiesSettingsPage::certByFilename(const QString &filename) {
  QSslCertificate cert;
  QFile certFile(filename);
  certFile.open(QIODevice::ReadOnly);
  QByteArray certRaw = certFile.read(2 << 20);
  certFile.close();

  for(int i = 0; i < 2; i++) {
    cert = QSslCertificate(certRaw, (QSsl::EncodingFormat)i);
    if(cert.isValid())
      break;
  }
  return cert;
}

void IdentitiesSettingsPage::showCertState(const QSslCertificate &cert) {
  if(!cert.isValid()) {
    ui.certOrgLabel->setText(tr("No Certificate loaded"));
    ui.certCNameLabel->setText(tr("No Certificate loaded"));
    ui.clearOrLoadCertButton->setText(tr("Load"));
  } else {
    ui.certOrgLabel->setText(cert.subjectInfo(QSslCertificate::Organization));
    ui.certCNameLabel->setText(cert.subjectInfo(QSslCertificate::CommonName));
    ui.clearOrLoadCertButton->setText(tr("Clear"));
  }
  ui.certOrgLabel->setProperty("sslCert", cert.toPem());
 }
#endif //HAVE_SSL

/*****************************************************************************************/

CreateIdentityDlg::CreateIdentityDlg(QAbstractItemModel *model, QWidget *parent)
  : QDialog(parent)
{
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

SaveIdentitiesDlg::SaveIdentitiesDlg(const QList<CertIdentity *> &toCreate, const QList<CertIdentity *> &toUpdate, const QList<IdentityId> &toRemove, QWidget *parent)
  : QDialog(parent)
{
  ui.setupUi(this);
  ui.abort->setIcon(SmallIcon("dialog-cancel"));

  numevents = toCreate.count() + toUpdate.count() + toRemove.count();
  rcvevents = 0;
  if(numevents) {
    ui.progressBar->setMaximum(numevents);
    ui.progressBar->setValue(0);

    connect(Client::instance(), SIGNAL(identityCreated(IdentityId)), this, SLOT(clientEvent()));
    connect(Client::instance(), SIGNAL(identityRemoved(IdentityId)), this, SLOT(clientEvent()));

    foreach(CertIdentity *id, toCreate) {
      Client::createIdentity(*id);
    }
    foreach(CertIdentity *id, toUpdate) {
      const Identity *cid = Client::identity(id->id());
      if(!cid) {
        qWarning() << "Invalid client identity!";
        numevents--;
        continue;
      }
      connect(cid, SIGNAL(updatedRemotely()), this, SLOT(clientEvent()));
      Client::updateIdentity(id->id(), id->toVariantMap());
#ifdef HAVE_SSL
      id->requestUpdateSslSettings();
#endif
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

/*************************************************************************************************/

NickEditDlg::NickEditDlg(const QString &old, const QStringList &exist, QWidget *parent)
  : QDialog(parent), oldNick(old), existing(exist) {
  ui.setupUi(this);

  // define a regexp for valid nicknames
  // TODO: add max nicklength according to ISUPPORT
  QString letter = "A-Za-z";
  QString special = "\x5b-\x60\x7b-\x7d";
  QRegExp rx(QString("[%1%2][%1%2\\d-]*").arg(letter, special));
  ui.nickEdit->setValidator(new QRegExpValidator(rx, ui.nickEdit));
  if(old.isEmpty()) {
    // new nick
    setWindowTitle(tr("Add Nickname"));
    on_nickEdit_textChanged(""); // disable ok button
  } else ui.nickEdit->setText(old);
}

QString NickEditDlg::nick() const {
  return ui.nickEdit->text();

}

void NickEditDlg::on_nickEdit_textChanged(const QString &text) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || existing.contains(text));
}



