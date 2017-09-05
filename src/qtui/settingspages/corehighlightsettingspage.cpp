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

#include "corehighlightsettingspage.h"

#include "qtui.h"
#include "uisettings.h"

#include <QHeaderView>
#include <client.h>

CoreHighlightSettingsPage::CoreHighlightSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Core-Side Highlights"), parent)
{
    ui.setupUi(this);
    ui.highlightTable->verticalHeader()->hide();
    ui.highlightTable->setShowGrid(false);

    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::RegExColumn)->setToolTip("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");
    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::RegExColumn)->setWhatsThis("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");

    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::CsColumn)->setToolTip("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");
    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::CsColumn)->setWhatsThis("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");

    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::ChanColumn)->setToolTip("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive.");
    ui.highlightTable->horizontalHeaderItem(CoreHighlightSettingsPage::ChanColumn)->setWhatsThis("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive.");

#if QT_VERSION < 0x050000
    ui.highlightTable->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    ui.highlightTable->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
#else
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::InverseColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
#endif

    connect(ui.add, SIGNAL(clicked(bool)), this, SLOT(addNewRow()));
    connect(ui.remove, SIGNAL(clicked(bool)), this, SLOT(removeSelectedRows()));
    //TODO: search for a better signal (one that emits everytime a selection has been changed for one item)
    connect(ui.highlightTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(selectRow(QTableWidgetItem *)));

    connect(ui.highlightAllNicks, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.highlightCurrentNick, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.highlightNoNick, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.nicksCaseSensitive, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
    connect(ui.add, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
    connect(ui.remove, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
    connect(ui.highlightTable, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(tableChanged(QTableWidgetItem *)));

    connect(Client::instance(), SIGNAL(connected()), this, SLOT(clientConnected()));
}

void CoreHighlightSettingsPage::clientConnected()
{
    connect(Client::highlightRuleManager(), SIGNAL(updated()), SLOT(revert()));
}


void CoreHighlightSettingsPage::revert()
{
    qWarning() << "revert()";

    if (!hasChanged())
        return;

    setChangedState(false);
    load();
}


bool CoreHighlightSettingsPage::hasDefaults() const
{
    return true;
}


void CoreHighlightSettingsPage::defaults()
{
    ui.highlightCurrentNick->setChecked(true);
    ui.nicksCaseSensitive->setChecked(false);
    emptyTable();

    widgetHasChanged();
}


void CoreHighlightSettingsPage::addNewRow(const QString &name, bool regex, bool cs, bool enable, bool inverse, const QString &sender,
                                          const QString &chanName, bool self)
{
    ui.highlightTable->setRowCount(ui.highlightTable->rowCount()+1);

    auto *nameItem = new QTableWidgetItem(name);

    auto *regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    auto *csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    auto *enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    auto *inverseItem = new QTableWidgetItem("");
    if (inverse)
        inverseItem->setCheckState(Qt::Checked);
    else
        inverseItem->setCheckState(Qt::Unchecked);
    inverseItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    auto *chanNameItem = new QTableWidgetItem(chanName);

    auto *senderItem = new QTableWidgetItem(sender);

    int lastRow = ui.highlightTable->rowCount()-1;
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::NameColumn, nameItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::RegExColumn, regexItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::CsColumn, csItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::EnableColumn, enableItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::InverseColumn, inverseItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::SenderColumn, senderItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.highlightTable->setCurrentItem(nameItem);

    highlightList << HighlightRuleManager::HighlightRule(name, regex, cs, enable, inverse, sender, chanName);
}


void CoreHighlightSettingsPage::removeSelectedRows()
{
    QList<int> selectedRows;
    QList<QTableWidgetItem *> selectedItemList = ui.highlightTable->selectedItems();
    foreach(QTableWidgetItem *selectedItem, selectedItemList) {
        selectedRows.append(selectedItem->row());
    }
    qSort(selectedRows.begin(), selectedRows.end(), qGreater<int>());
    int lastRow = -1;
    foreach(int row, selectedRows) {
        if (row != lastRow) {
            ui.highlightTable->removeRow(row);
            highlightList.removeAt(row);
        }
        lastRow = row;
    }
}


void CoreHighlightSettingsPage::selectRow(QTableWidgetItem *item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.highlightTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, CoreHighlightSettingsPage::ColumnCount-1), selected);
}


void CoreHighlightSettingsPage::emptyTable()
{
    // ui.highlight and highlightList should have the same size, but just to make sure.
    if (ui.highlightTable->rowCount() != highlightList.size()) {
        qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
    }
    while (ui.highlightTable->rowCount()) {
        ui.highlightTable->removeRow(0);
    }
    while (highlightList.size()) {
        highlightList.removeLast();
    }
}


void CoreHighlightSettingsPage::tableChanged(QTableWidgetItem *item)
{
    if (item->row()+1 > highlightList.size())
        return;

    auto highlightRule = highlightList.value(item->row());


    switch (item->column())
    {
    case CoreHighlightSettingsPage::NameColumn:
        if (item->text() == "")
            item->setText(tr("this shouldn't be empty"));
        highlightRule.name = item->text();
        break;
    case CoreHighlightSettingsPage::RegExColumn:
        highlightRule.isRegEx = (item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::CsColumn:
        highlightRule.isCaseSensitive = (item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::EnableColumn:
        highlightRule.isEnabled = (item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::InverseColumn:
        highlightRule.isInverse = (item->checkState() == Qt::Checked);
        break;
        case CoreHighlightSettingsPage::SenderColumn:
            if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
                item->setText("");
            highlightRule.sender = item->text();
            break;
    case CoreHighlightSettingsPage::ChanColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        highlightRule.chanName = item->text();
        break;
    }
    highlightList[item->row()] = highlightRule;
    emit widgetHasChanged();
}


void CoreHighlightSettingsPage::load()
{
    emptyTable();

    auto ruleManager = Client::highlightRuleManager();
    for (HighlightRuleManager::HighlightRule rule : ruleManager->highlightRuleList()) {
        addNewRow(rule.name, rule.isRegEx, rule.isCaseSensitive, rule.isEnabled, rule.isInverse, rule.sender, rule.chanName);
    }

    switch (ruleManager->highlightNick())
    {
    case HighlightRuleManager::NoNick:
        ui.highlightNoNick->setChecked(true);
        break;
    case HighlightRuleManager::CurrentNick:
        ui.highlightCurrentNick->setChecked(true);
        break;
    case HighlightRuleManager::AllNicks:
        ui.highlightAllNicks->setChecked(true);
        break;
    }
    ui.nicksCaseSensitive->setChecked(ruleManager->nicksCaseSensitive());

    setChangedState(false);
    _initialized = true;
}

void CoreHighlightSettingsPage::save()
{
    if (!hasChanged())
        return;

    if (!_initialized)
        return;

    auto ruleManager = Client::highlightRuleManager();
    if (ruleManager == nullptr)
        return;

    auto clonedManager = HighlightRuleManager();
    clonedManager.fromVariantMap(ruleManager->toVariantMap());
    clonedManager.clear();

    for (const HighlightRuleManager::HighlightRule &rule : highlightList) {
        clonedManager.addHighlightRule(rule.name, rule.isRegEx, rule.isCaseSensitive, rule.isEnabled, rule.isInverse,
                                       rule.sender, rule.chanName);
    }

    HighlightRuleManager::HighlightNickType highlightNickType = HighlightRuleManager::NoNick;
    if (ui.highlightCurrentNick->isChecked())
        highlightNickType = HighlightRuleManager::CurrentNick;
    if (ui.highlightAllNicks->isChecked())
        highlightNickType = HighlightRuleManager::AllNicks;


    clonedManager.setHighlightNick(highlightNickType);
    clonedManager.setNicksCaseSensitive(ui.nicksCaseSensitive->isChecked());

    ruleManager->requestUpdate(clonedManager.toVariantMap());
    setChangedState(false);
    load();
}


void CoreHighlightSettingsPage::widgetHasChanged()
{
    setChangedState(true);
}