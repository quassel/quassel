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

#include "identityeditwidget.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QUrl>
#include <QMessageBox>

#include "client.h"
#include "iconloader.h"

IdentityEditWidget::IdentityEditWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    ui.addNick->setIcon(SmallIcon("list-add"));
    ui.deleteNick->setIcon(SmallIcon("edit-delete"));
    ui.renameNick->setIcon(SmallIcon("edit-rename"));
    ui.nickUp->setIcon(SmallIcon("go-up"));
    ui.nickDown->setIcon(SmallIcon("go-down"));

    // We need to know whenever the state of input widgets changes...
    connect(ui.realName, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.nicknameList, SIGNAL(itemChanged(QListWidgetItem *)), this, SIGNAL(widgetHasChanged()));
    connect(ui.awayNick, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.awayReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.autoAwayEnabled, SIGNAL(clicked(bool)), this, SIGNAL(widgetHasChanged()));
    connect(ui.autoAwayTime, SIGNAL(valueChanged(int)), this, SIGNAL(widgetHasChanged()));
    connect(ui.autoAwayReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.autoAwayReasonEnabled, SIGNAL(clicked(bool)), this, SIGNAL(widgetHasChanged()));
    connect(ui.detachAwayEnabled, SIGNAL(clicked(bool)), this, SIGNAL(widgetHasChanged()));
    connect(ui.detachAwayReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.ident, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.kickReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.partReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));
    connect(ui.quitReason, SIGNAL(textEdited(const QString &)), this, SIGNAL(widgetHasChanged()));

    setWidgetStates();
    connect(ui.nicknameList, SIGNAL(itemSelectionChanged()), this, SLOT(setWidgetStates()));

    connect(ui.continueUnsecured, SIGNAL(clicked()), this, SIGNAL(requestEditSsl()));

    // we would need this if we enabled drag and drop in the nicklist...
    //connect(ui.nicknameList, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(setWidgetStates()));
    //connect(ui.nicknameList->model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(nicklistHasChanged()));

    // disabling unused stuff
    ui.autoAwayEnabled->hide();
    ui.awayNick->hide();
    ui.awayNickLabel->hide();

    ui.detachAwayEnabled->setVisible(!Client::internalCore());

#ifdef HAVE_SSL
    ui.sslKeyGroupBox->setAcceptDrops(true);
    ui.sslKeyGroupBox->installEventFilter(this);
    ui.sslCertGroupBox->setAcceptDrops(true);
    ui.sslCertGroupBox->installEventFilter(this);
#endif
}


void IdentityEditWidget::setWidgetStates()
{
    if (ui.nicknameList->selectedItems().count()) {
        ui.renameNick->setEnabled(true);
        ui.nickUp->setEnabled(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]) > 0);
        ui.nickDown->setEnabled(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]) < ui.nicknameList->count()-1);
    }
    else {
        ui.renameNick->setDisabled(true);
        ui.nickUp->setDisabled(true);
        ui.nickDown->setDisabled(true);
    }
    ui.deleteNick->setEnabled(ui.nicknameList->count() > 1);
}


void IdentityEditWidget::displayIdentity(CertIdentity *id, CertIdentity *saveId)
{
    if (saveId) {
        saveToIdentity(saveId);
    }

    if (!id)
        return;

    ui.realName->setText(id->realName());
    ui.nicknameList->clear();
    ui.nicknameList->addItems(id->nicks());
    //for(int i = 0; i < ui.nicknameList->count(); i++) {
    //  ui.nicknameList->item(i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    //}
    if (ui.nicknameList->count()) ui.nicknameList->setCurrentRow(0);
    ui.awayNick->setText(id->awayNick());
    ui.awayReason->setText(id->awayReason());
    ui.autoAwayEnabled->setChecked(id->autoAwayEnabled());
    ui.autoAwayTime->setValue(id->autoAwayTime());
    ui.autoAwayReason->setText(id->autoAwayReason());
    ui.autoAwayReasonEnabled->setChecked(id->autoAwayReasonEnabled());
    ui.detachAwayEnabled->setChecked(id->detachAwayEnabled());
    ui.detachAwayReason->setText(id->detachAwayReason());
    ui.ident->setText(id->ident());
    ui.kickReason->setText(id->kickReason());
    ui.partReason->setText(id->partReason());
    ui.quitReason->setText(id->quitReason());
#ifdef HAVE_SSL
    showKeyState(id->sslKey());
    showCertState(id->sslCert());
#endif
}


void IdentityEditWidget::saveToIdentity(CertIdentity *id)
{
    QRegExp linebreaks = QRegExp("[\\r\\n]");
    id->setRealName(ui.realName->text());
    QStringList nicks;
    for (int i = 0; i < ui.nicknameList->count(); i++) {
        nicks << ui.nicknameList->item(i)->text();
    }
    id->setNicks(nicks);
    id->setAwayNick(ui.awayNick->text());
    id->setAwayNickEnabled(true);
    id->setAwayReason(ui.awayReason->text().remove(linebreaks));
    id->setAwayReasonEnabled(true);
    id->setAutoAwayEnabled(ui.autoAwayEnabled->isChecked());
    id->setAutoAwayTime(ui.autoAwayTime->value());
    id->setAutoAwayReason(ui.autoAwayReason->text().remove(linebreaks));
    id->setAutoAwayReasonEnabled(ui.autoAwayReasonEnabled->isChecked());
    id->setDetachAwayEnabled(ui.detachAwayEnabled->isChecked());
    id->setDetachAwayReason(ui.detachAwayReason->text().remove(linebreaks));
    id->setDetachAwayReasonEnabled(true);
    id->setIdent(ui.ident->text());
    id->setKickReason(ui.kickReason->text().remove(linebreaks));
    id->setPartReason(ui.partReason->text().remove(linebreaks));
    id->setQuitReason(ui.quitReason->text().remove(linebreaks));
#ifdef HAVE_SSL
    id->setSslKey(QSslKey(ui.keyTypeLabel->property("sslKey").toByteArray(), (QSsl::KeyAlgorithm)(ui.keyTypeLabel->property("sslKeyType").toInt())));
    id->setSslCert(QSslCertificate(ui.certOrgLabel->property("sslCert").toByteArray()));
#endif
}


void IdentityEditWidget::on_addNick_clicked()
{
    QStringList existing;
    for (int i = 0; i < ui.nicknameList->count(); i++) existing << ui.nicknameList->item(i)->text();
    NickEditDlg dlg(QString(), existing, this);
    if (dlg.exec() == QDialog::Accepted) {
        ui.nicknameList->addItem(dlg.nick());
        ui.nicknameList->setCurrentRow(ui.nicknameList->count()-1);
        setWidgetStates();
        emit widgetHasChanged();
    }
}


void IdentityEditWidget::on_deleteNick_clicked()
{
    // no confirmation, since a nickname is really nothing hard to recreate
    if (ui.nicknameList->selectedItems().count()) {
        delete ui.nicknameList->takeItem(ui.nicknameList->row(ui.nicknameList->selectedItems()[0]));
        ui.nicknameList->setCurrentRow(qMin(ui.nicknameList->currentRow()+1, ui.nicknameList->count()-1));
        setWidgetStates();
        emit widgetHasChanged();
    }
}


void IdentityEditWidget::on_renameNick_clicked()
{
    if (!ui.nicknameList->selectedItems().count()) return;
    QString old = ui.nicknameList->selectedItems()[0]->text();
    QStringList existing;
    for (int i = 0; i < ui.nicknameList->count(); i++) existing << ui.nicknameList->item(i)->text();
    NickEditDlg dlg(old, existing, this);
    if (dlg.exec() == QDialog::Accepted) {
        ui.nicknameList->selectedItems()[0]->setText(dlg.nick());
    }
}


void IdentityEditWidget::on_nickUp_clicked()
{
    if (!ui.nicknameList->selectedItems().count()) return;
    int row = ui.nicknameList->row(ui.nicknameList->selectedItems()[0]);
    if (row > 0) {
        ui.nicknameList->insertItem(row-1, ui.nicknameList->takeItem(row));
        ui.nicknameList->setCurrentRow(row-1);
        setWidgetStates();
        emit widgetHasChanged();
    }
}


void IdentityEditWidget::on_nickDown_clicked()
{
    if (!ui.nicknameList->selectedItems().count()) return;
    int row = ui.nicknameList->row(ui.nicknameList->selectedItems()[0]);
    if (row < ui.nicknameList->count()-1) {
        ui.nicknameList->insertItem(row+1, ui.nicknameList->takeItem(row));
        ui.nicknameList->setCurrentRow(row+1);
        setWidgetStates();
        emit widgetHasChanged();
    }
}


void IdentityEditWidget::showAdvanced(bool advanced)
{
    int idx = ui.tabWidget->indexOf(ui.advancedTab);
    if (advanced) {
        if (idx != -1)
            return;  // already added
        ui.tabWidget->addTab(ui.advancedTab, tr("Advanced"));
    }
    else {
        if (idx == -1)
            return;  // already removed
        ui.tabWidget->removeTab(idx);
    }
}


void IdentityEditWidget::setSslState(SslState state)
{
    switch (state) {
    case NoSsl:
        ui.keyAndCertSettings->setCurrentIndex(0);
        break;
    case UnsecureSsl:
        ui.keyAndCertSettings->setCurrentIndex(1);
        break;
    case AllowSsl:
        ui.keyAndCertSettings->setCurrentIndex(2);
        break;
    }
}


#ifdef HAVE_SSL
bool IdentityEditWidget::eventFilter(QObject *watched, QEvent *event)
{
    bool isCert = (watched == ui.sslCertGroupBox);
    switch (event->type()) {
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


void IdentityEditWidget::sslDragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("text/uri")) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}


void IdentityEditWidget::sslDropEvent(QDropEvent *event, bool isCert)
{
    QByteArray rawUris;
    if (event->mimeData()->hasFormat("text/uri-list"))
        rawUris = event->mimeData()->data("text/uri-list");
    else
        rawUris = event->mimeData()->data("text/uri");

    QTextStream uriStream(rawUris);
    QString filename = QUrl(uriStream.readLine()).toLocalFile();

    if (isCert) {
        QSslCertificate cert = certByFilename(filename);
        if (cert.isValid())
            showCertState(cert);
    }
    else {
        QSslKey key = keyByFilename(filename);
        if (!key.isNull())
            showKeyState(key);
    }
    event->accept();
    emit widgetHasChanged();
}


void IdentityEditWidget::on_clearOrLoadKeyButton_clicked()
{
    QSslKey key;

    if (ui.keyTypeLabel->property("sslKey").toByteArray().isEmpty())
        key = keyByFilename(QFileDialog::getOpenFileName(this, tr("Load a Key"), QDesktopServices::storageLocation(QDesktopServices::HomeLocation)));

    showKeyState(key);
    emit widgetHasChanged();
}


QSslKey IdentityEditWidget::keyByFilename(const QString &filename)
{
    QSslKey key;

    QFile keyFile(filename);
    keyFile.open(QIODevice::ReadOnly);
    QByteArray keyRaw = keyFile.read(2 << 20);
    keyFile.close();

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            key = QSslKey(keyRaw, (QSsl::KeyAlgorithm)j, (QSsl::EncodingFormat)i);
            if (!key.isNull())
                goto returnKey;
        }
    }
    QMessageBox::information(this, tr("Failed to read key"), tr("Failed to read the key file. It is either incompatible or invalid. Note that the key file must not have a passphrase."));
returnKey:
    return key;
}


void IdentityEditWidget::showKeyState(const QSslKey &key)
{
    if (key.isNull()) {
        ui.keyTypeLabel->setText(tr("No Key loaded"));
        ui.clearOrLoadKeyButton->setText(tr("Load"));
    }
    else {
        switch (key.algorithm()) {
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


void IdentityEditWidget::on_clearOrLoadCertButton_clicked()
{
    QSslCertificate cert;

    if (ui.certOrgLabel->property("sslCert").toByteArray().isEmpty())
        cert = certByFilename(QFileDialog::getOpenFileName(this, tr("Load a Certificate"), QDesktopServices::storageLocation(QDesktopServices::HomeLocation)));

    showCertState(cert);
    emit widgetHasChanged();
}


QSslCertificate IdentityEditWidget::certByFilename(const QString &filename)
{
    QSslCertificate cert;
    QFile certFile(filename);
    certFile.open(QIODevice::ReadOnly);
    QByteArray certRaw = certFile.read(2 << 20);
    certFile.close();

    for (int i = 0; i < 2; i++) {
        cert = QSslCertificate(certRaw, (QSsl::EncodingFormat)i);
        if (cert.isValid())
            break;
    }
    return cert;
}


void IdentityEditWidget::showCertState(const QSslCertificate &cert)
{
    if (!cert.isValid()) {
        ui.certOrgLabel->setText(tr("No Certificate loaded"));
        ui.certCNameLabel->setText(tr("No Certificate loaded"));
        ui.clearOrLoadCertButton->setText(tr("Load"));
    }
    else {
        ui.certOrgLabel->setText(cert.subjectInfo(QSslCertificate::Organization));
        ui.certCNameLabel->setText(cert.subjectInfo(QSslCertificate::CommonName));
        ui.clearOrLoadCertButton->setText(tr("Clear"));
    }
    ui.certOrgLabel->setProperty("sslCert", cert.toPem());
}


#endif //HAVE_SSL
