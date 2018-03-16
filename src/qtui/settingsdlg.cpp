/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include <QIcon>
#include <QMessageBox>
#include <QPushButton>

#include "settingsdlg.h"

#include "client.h"

SettingsDlg::SettingsDlg(QWidget *parent)
    : QDialog(parent),
    _currentPage(0)
{
    ui.setupUi(this);
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowIcon(QIcon::fromTheme("configure"));

    updateGeometry();

    ui.settingsTree->setRootIsDecorated(false);

    connect(ui.settingsTree, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelected()));
    connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));

    connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), SLOT(coreConnectionStateChanged()));

    setButtonStates();

    // Some settings panes can take a good bit of space.  To avoid squashing the settings tree, try
    // to resize the dialog.  If needed, it can always be resized by the user to take less space.
    //
    // Only try to resize if the sizes are valid.  This shouldn't happen.. but better to be safe.
    // See http://www.qtcentre.org/threads/3427-invalid-sizeHint()
    if (ui.settingsTree->sizeHint().isValid() && ui.settingsTree->size().isValid()) {
        // Find out how much width would make the settings tree happy
        int wantedExtraWidth = ui.settingsTree->sizeHint().width()
                - ui.settingsTree->size().width();
        // If more space is needed, try to resize to allow for it.  Qt should keep the dialog within
        // the bounds of the screen.
        if (wantedExtraWidth > 0) {
            this->resize(this->width() + wantedExtraWidth, this->height());
        }
    }
}


void SettingsDlg::coreConnectionStateChanged()
{
    for (int i = 0; i < ui.settingsTree->topLevelItemCount(); i++) {
        QTreeWidgetItem *catItem = ui.settingsTree->topLevelItem(i);
        for (int j = 0; j < catItem->childCount(); j++) {
            QTreeWidgetItem *item = catItem->child(j);
            setItemState(item);
        }
        setItemState(catItem);
    }
}


void SettingsDlg::setItemState(QTreeWidgetItem *item)
{
    SettingsPage *sp = qobject_cast<SettingsPage *>(item->data(0, SettingsPageRole).value<QObject *>());
    Q_ASSERT(sp);
    bool disabledDueToConnection = !Client::isConnected() && sp->needsCoreConnection();
    bool disabledDueToOwnChoice = !sp->isSelectable();
    item->setDisabled(disabledDueToConnection || disabledDueToOwnChoice);
}


void SettingsDlg::registerSettingsPage(SettingsPage *sp)
{
    sp->setParent(ui.settingsStack);
    ui.settingsStack->addWidget(sp);

    connect(sp, SIGNAL(changed(bool)), this, SLOT(setButtonStates()));

    QTreeWidgetItem *cat;
    QList<QTreeWidgetItem *> cats = ui.settingsTree->findItems(sp->category(), Qt::MatchExactly);
    if (!cats.count()) {
        cat = new QTreeWidgetItem(ui.settingsTree, QStringList(sp->category()));
        cat->setExpanded(true);
        cat->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    else {
        cat = cats[0];
    }

    QTreeWidgetItem *item;
    if (sp->title().isEmpty())
        item = cat;
    else
        item = new QTreeWidgetItem(cat, QStringList(sp->title()));

    item->setData(0, SettingsPageRole, QVariant::fromValue<QObject *>(sp));
    ui.settingsTree->setMinimumWidth(ui.settingsTree->header()->sectionSizeHint(0) + 5);
    pageIsLoaded[sp] = false;
    if (!ui.settingsTree->selectedItems().count())
        ui.settingsTree->setCurrentItem(item);

    setItemState(item);
}


void SettingsDlg::selectPage(SettingsPage *sp)
{
    if (!sp) {
        _currentPage = 0;
        ui.settingsStack->setCurrentIndex(0);
        ui.pageTitle->setText(tr("Settings"));
        return;
    }

    if (!pageIsLoaded[sp]) {
        sp->load();
        pageIsLoaded[sp] = true;
    }

    if (sp != currentPage() && currentPage() != 0 && currentPage()->hasChanged()) {
        int ret = QMessageBox::warning(this, tr("Save changes"),
            tr("There are unsaved changes on the current configuration page. Would you like to apply your changes now?"),
            QMessageBox::Discard|QMessageBox::Save|QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            if (!applyChanges()) sp = currentPage();
        }
        else if (ret == QMessageBox::Discard) {
            undoChanges();
        }
        else sp = currentPage();
    }

    if (sp != currentPage()) {
        if (sp->title().isEmpty()) {
            ui.pageTitle->setText(sp->category());
            setWindowTitle(tr("Configure %1").arg(sp->category()));
        }
        else {
            ui.pageTitle->setText(sp->title());
            setWindowTitle(tr("Configure %1").arg(sp->title()));
        }

        ui.settingsStack->setCurrentWidget(sp);
        _currentPage = sp;
    }
    setButtonStates();
}


void SettingsDlg::itemSelected()
{
    QList<QTreeWidgetItem *> items = ui.settingsTree->selectedItems();
    SettingsPage *sp = 0;
    if (!items.isEmpty()) {
        sp = qobject_cast<SettingsPage *>(items[0]->data(0, SettingsPageRole).value<QObject *>());
    }
    selectPage(sp);
}


void SettingsDlg::setButtonStates()
{
    SettingsPage *sp = currentPage();
    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(sp && sp->hasChanged());
    ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(sp && sp->hasChanged());
    ui.buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(sp && sp->hasDefaults());
}


void SettingsDlg::buttonClicked(QAbstractButton *button)
{
    switch (ui.buttonBox->standardButton(button)) {
    case QDialogButtonBox::Ok:
        if (currentPage() && currentPage()->hasChanged()) {
            if (applyChanges()) accept();
        }
        else accept();
        break;
    case QDialogButtonBox::Apply:
        applyChanges();
        break;
    case QDialogButtonBox::Cancel:
        undoChanges();
        reject();
        break;
    case QDialogButtonBox::Reset:
        reload();
        break;
    case QDialogButtonBox::RestoreDefaults:
        loadDefaults();
        break;
    default:
        break;
    }
}


bool SettingsDlg::applyChanges()
{
    if (!currentPage()) return false;
    if (currentPage()->aboutToSave()) {
        currentPage()->save();
        return true;
    }
    return false;
}


void SettingsDlg::undoChanges()
{
    if (currentPage()) {
        currentPage()->load();
    }
}


void SettingsDlg::reload()
{
    if (!currentPage()) return;
    int ret = QMessageBox::question(this, tr("Reload Settings"), tr("Do you like to reload the settings, undoing your changes on this page?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        currentPage()->load();
    }
}


void SettingsDlg::loadDefaults()
{
    if (!currentPage()) return;
    int ret = QMessageBox::question(this, tr("Restore Defaults"), tr("Do you like to restore the default values for this page?"),
        QMessageBox::RestoreDefaults|QMessageBox::Cancel, QMessageBox::Cancel);
    if (ret == QMessageBox::RestoreDefaults) {
        currentPage()->defaults();
    }
}
