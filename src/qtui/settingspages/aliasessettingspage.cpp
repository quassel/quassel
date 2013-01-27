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

#include "aliasessettingspage.h"

#include <QHeaderView>
#include <QItemSelectionModel>

#include "iconloader.h"

AliasesSettingsPage::AliasesSettingsPage(QWidget *parent)
    : SettingsPage(tr("IRC"), tr("Aliases"), parent)
{
    ui.setupUi(this);
    ui.newAliasButton->setIcon(SmallIcon("list-add"));
    ui.deleteAliasButton->setIcon(SmallIcon("edit-delete"));

    ui.aliasesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.aliasesView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.aliasesView->setAlternatingRowColors(true);
    ui.aliasesView->setTabKeyNavigation(false);
    ui.aliasesView->setModel(&_aliasesModel);
    // ui.aliasesView->setSortingEnabled(true);
    ui.aliasesView->verticalHeader()->hide();
    ui.aliasesView->horizontalHeader()->setStretchLastSection(true);

    connect(ui.newAliasButton, SIGNAL(clicked()), &_aliasesModel, SLOT(newAlias()));
    connect(ui.deleteAliasButton, SIGNAL(clicked()), this, SLOT(deleteSelectedAlias()));
    connect(&_aliasesModel, SIGNAL(configChanged(bool)), this, SLOT(setChangedState(bool)));
    connect(&_aliasesModel, SIGNAL(modelReady(bool)), this, SLOT(enableDialog(bool)));

    enableDialog(_aliasesModel.isReady());
}


void AliasesSettingsPage::load()
{
    if (_aliasesModel.configChanged())
        _aliasesModel.revert();
}


void AliasesSettingsPage::defaults()
{
    _aliasesModel.loadDefaults();
}


void AliasesSettingsPage::save()
{
    if (_aliasesModel.configChanged())
        _aliasesModel.commit();
}


void AliasesSettingsPage::enableDialog(bool enabled)
{
    ui.newAliasButton->setEnabled(enabled);
    ui.deleteAliasButton->setEnabled(enabled);
    setEnabled(enabled);
}


void AliasesSettingsPage::deleteSelectedAlias()
{
    if (!ui.aliasesView->selectionModel()->hasSelection())
        return;

    _aliasesModel.removeAlias(ui.aliasesView->selectionModel()->selectedIndexes()[0].row());
}
