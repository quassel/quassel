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

#include <QMessageBox>
#include <QPushButton>

#include "settingspagedlg.h"

#include "iconloader.h"

SettingsPageDlg::SettingsPageDlg(SettingsPage *page, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    _currentPage = page;
    page->setParent(this);

    // make it look more native under Mac OS X:
    setWindowFlags(Qt::Sheet);

    ui.pageTitle->setText(page->title());
    setWindowTitle(tr("Configure %1").arg(page->title()));
    setWindowIcon(SmallIcon("configure"));

    // make the scrollarea behave sanely
    ui.settingsFrame->setWidgetResizable(true);
    ui.settingsFrame->setWidget(page);

    updateGeometry();

    connect(page, SIGNAL(changed(bool)), this, SLOT(setButtonStates()));
    connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
    page->load();
    setButtonStates();
}


SettingsPage *SettingsPageDlg::currentPage() const
{
    return _currentPage;
}


void SettingsPageDlg::setButtonStates()
{
    SettingsPage *sp = currentPage();
    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(sp && sp->hasChanged());
    ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(sp && sp->hasChanged());
    ui.buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(sp && sp->hasDefaults());
}


void SettingsPageDlg::buttonClicked(QAbstractButton *button)
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


bool SettingsPageDlg::applyChanges()
{
    if (!currentPage()) return false;
    if (currentPage()->aboutToSave()) {
        currentPage()->save();
        return true;
    }
    return false;
}


void SettingsPageDlg::undoChanges()
{
    if (currentPage()) {
        currentPage()->load();
    }
}


void SettingsPageDlg::reload()
{
    if (!currentPage()) return;
    int ret = QMessageBox::question(this, tr("Reload Settings"), tr("Do you like to reload the settings, undoing your changes on this page?"),
        QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        currentPage()->load();
    }
}


void SettingsPageDlg::loadDefaults()
{
    if (!currentPage()) return;
    int ret = QMessageBox::question(this, tr("Restore Defaults"), tr("Do you like to restore the default values for this page?"),
        QMessageBox::RestoreDefaults|QMessageBox::Cancel, QMessageBox::Cancel);
    if (ret == QMessageBox::RestoreDefaults) {
        currentPage()->defaults();
    }
}
