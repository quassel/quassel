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

#include <QHeaderView>
#include <QTableWidget>

#include "client.h"
#include "corehighlightsettingspage.h"
#include "qtui.h"

CoreHighlightSettingsPage::CoreHighlightSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Remote Highlights"), parent)
{
    ui.setupUi(this);

    setupRuleTable(ui.highlightTable);
    setupRuleTable(ui.ignoredTable);

    ui.highlightNicksComboBox->addItem(tr("All Nicks from Identity"), QVariant(HighlightRuleManager::AllNicks));
    ui.highlightNicksComboBox->addItem(tr("Current Nick"), QVariant(HighlightRuleManager::CurrentNick));
    ui.highlightNicksComboBox->addItem(tr("None"), QVariant(HighlightRuleManager::NoNick));

    coreConnectionStateChanged(Client::isConnected()); // need a core connection!
    connect(Client::instance(), SIGNAL(coreConnectionStateChanged(bool)), this, SLOT(coreConnectionStateChanged(bool)));

    connect(ui.highlightAdd, SIGNAL(clicked(bool)), this, SLOT(addNewHighlightRow()));
    connect(ui.highlightRemove, SIGNAL(clicked(bool)), this, SLOT(removeSelectedHighlightRows()));
    connect(ui.highlightImport, SIGNAL(clicked(bool)), this, SLOT(importRules()));

    connect(ui.ignoredAdd, SIGNAL(clicked(bool)), this, SLOT(addNewIgnoredRow()));
    connect(ui.ignoredRemove, SIGNAL(clicked(bool)), this, SLOT(removeSelectedIgnoredRows()));

    // TODO: search for a better signal (one that emits everytime a selection has been changed for one item)
    connect(ui.highlightTable,
            SIGNAL(itemClicked(QTableWidgetItem * )),
            this,
            SLOT(selectHighlightRow(QTableWidgetItem * )));
    connect(ui.ignoredTable,
            SIGNAL(itemClicked(QTableWidgetItem * )),
            this,
            SLOT(selectIgnoredRow(QTableWidgetItem * )));


    connect(ui.highlightNicksComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));
    connect(ui.nicksCaseSensitive, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

    connect(ui.highlightAdd, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
    connect(ui.highlightRemove, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

    connect(ui.ignoredAdd, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
    connect(ui.ignoredRemove, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

    connect(ui.highlightTable,
            SIGNAL(itemChanged(QTableWidgetItem * )),
            this,
            SLOT(highlightTableChanged(QTableWidgetItem * )));

    connect(ui.ignoredTable,
            SIGNAL(itemChanged(QTableWidgetItem * )),
            this,
            SLOT(ignoredTableChanged(QTableWidgetItem * )));

    connect(Client::instance(), SIGNAL(connected()), this, SLOT(clientConnected()));
}

void CoreHighlightSettingsPage::coreConnectionStateChanged(bool state)
{
    setEnabled(state);
    if (state) {
        load();
    } else {
        revert();
    }
}

void CoreHighlightSettingsPage::setupRuleTable(QTableWidget *table) const
{
    table->verticalHeader()->hide();
    table->setShowGrid(false);

    table->horizontalHeaderItem(CoreHighlightSettingsPage::RegExColumn)->setToolTip(
        tr("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword."));
    table->horizontalHeaderItem(CoreHighlightSettingsPage::RegExColumn)->setWhatsThis(
        tr("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword."));

    table->horizontalHeaderItem(CoreHighlightSettingsPage::CsColumn)->setToolTip(
        tr("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>."));
    table->horizontalHeaderItem(CoreHighlightSettingsPage::CsColumn)->setWhatsThis(
        tr("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>."));

    table->horizontalHeaderItem(CoreHighlightSettingsPage::ChanColumn)->setToolTip(
        tr("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive."));
    table->horizontalHeaderItem(CoreHighlightSettingsPage::ChanColumn)->setWhatsThis(
        tr("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive."));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    table->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    table->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setResizeMode(CoreHighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
#else
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
#endif
}

void CoreHighlightSettingsPage::clientConnected()
{
    connect(Client::highlightRuleManager(), SIGNAL(updated()), SLOT(revert()));
}

void CoreHighlightSettingsPage::revert()
{
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
    int highlightNickType = HighlightRuleManager::HighlightNickType::CurrentNick;
    int defaultIndex = ui.highlightNicksComboBox->findData(QVariant(highlightNickType));
    ui.highlightNicksComboBox->setCurrentIndex(defaultIndex);
    ui.nicksCaseSensitive->setChecked(false);
    emptyHighlightTable();
    emptyIgnoredTable();

    widgetHasChanged();
}

void CoreHighlightSettingsPage::addNewHighlightRow(bool enable, const QString &name, bool regex, bool cs,
                                                   const QString &sender, const QString &chanName, bool self)
{
    ui.highlightTable->setRowCount(ui.highlightTable->rowCount() + 1);

    auto *nameItem = new QTableWidgetItem(name);

    auto *regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *chanNameItem = new QTableWidgetItem(chanName);

    auto *senderItem = new QTableWidgetItem(sender);

    int lastRow = ui.highlightTable->rowCount() - 1;
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::NameColumn, nameItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::RegExColumn, regexItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::CsColumn, csItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::EnableColumn, enableItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::SenderColumn, senderItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.highlightTable->setCurrentItem(nameItem);

    highlightList << HighlightRuleManager::HighlightRule(name, regex, cs, enable, false, sender, chanName);
}

void CoreHighlightSettingsPage::addNewIgnoredRow(bool enable, const QString &name, bool regex, bool cs,
                                                 const QString &sender, const QString &chanName, bool self)
{
    ui.ignoredTable->setRowCount(ui.ignoredTable->rowCount() + 1);

    auto *nameItem = new QTableWidgetItem(name);

    auto *regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto *chanNameItem = new QTableWidgetItem(chanName);

    auto *senderItem = new QTableWidgetItem(sender);

    int lastRow = ui.ignoredTable->rowCount() - 1;
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::NameColumn, nameItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::RegExColumn, regexItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::CsColumn, csItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::EnableColumn, enableItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::SenderColumn, senderItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.ignoredTable->setCurrentItem(nameItem);

    ignoredList << HighlightRuleManager::HighlightRule(name, regex, cs, enable, true, sender, chanName);
}

void CoreHighlightSettingsPage::removeSelectedHighlightRows()
{
    QList<int> selectedRows;
    QList<QTableWidgetItem *> selectedItemList = ui.highlightTable->selectedItems();
    for (auto selectedItem : selectedItemList) {
        selectedRows.append(selectedItem->row());
    }
    qSort(selectedRows.begin(), selectedRows.end(), qGreater<int>());
    int lastRow = -1;
    for (auto row : selectedRows) {
        if (row != lastRow) {
            ui.highlightTable->removeRow(row);
            highlightList.removeAt(row);
        }
        lastRow = row;
    }
}

void CoreHighlightSettingsPage::removeSelectedIgnoredRows()
{
    QList<int> selectedRows;
    QList<QTableWidgetItem *> selectedItemList = ui.ignoredTable->selectedItems();
    for (auto selectedItem : selectedItemList) {
        selectedRows.append(selectedItem->row());
    }
    qSort(selectedRows.begin(), selectedRows.end(), qGreater<int>());
    int lastRow = -1;
    for (auto row : selectedRows) {
        if (row != lastRow) {
            ui.ignoredTable->removeRow(row);
            ignoredList.removeAt(row);
        }
        lastRow = row;
    }
}

void CoreHighlightSettingsPage::selectHighlightRow(QTableWidgetItem *item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.highlightTable
        ->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, CoreHighlightSettingsPage::ColumnCount - 1),
                           selected);
}

void CoreHighlightSettingsPage::selectIgnoredRow(QTableWidgetItem *item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.ignoredTable
        ->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, CoreHighlightSettingsPage::ColumnCount - 1),
                           selected);
}

void CoreHighlightSettingsPage::emptyHighlightTable()
{
    // ui.highlight and highlightList should have the same size, but just to make sure.
    if (ui.highlightTable->rowCount() != highlightList.size()) {
        qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
    }
    ui.highlightTable->clearContents();
    highlightList.clear();
}

void CoreHighlightSettingsPage::emptyIgnoredTable()
{
    // ui.highlight and highlightList should have the same size, but just to make sure.
    if (ui.ignoredTable->rowCount() != ignoredList.size()) {
        qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
    }
    ui.ignoredTable->clearContents();
    ignoredList.clear();
}

void CoreHighlightSettingsPage::highlightTableChanged(QTableWidgetItem *item)
{
    if (item->row() + 1 > highlightList.size())
        return;

    auto highlightRule = highlightList.value(item->row());


    switch (item->column()) {
        case CoreHighlightSettingsPage::EnableColumn:
            highlightRule.isEnabled = (item->checkState() == Qt::Checked);
            break;
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

void CoreHighlightSettingsPage::ignoredTableChanged(QTableWidgetItem *item)
{
    if (item->row() + 1 > ignoredList.size())
        return;

    auto ignoredRule = ignoredList.value(item->row());


    switch (item->column()) {
        case CoreHighlightSettingsPage::EnableColumn:
            ignoredRule.isEnabled = (item->checkState() == Qt::Checked);
            break;
        case CoreHighlightSettingsPage::NameColumn:
            if (item->text() == "")
                item->setText(tr("this shouldn't be empty"));
            ignoredRule.name = item->text();
            break;
        case CoreHighlightSettingsPage::RegExColumn:
            ignoredRule.isRegEx = (item->checkState() == Qt::Checked);
            break;
        case CoreHighlightSettingsPage::CsColumn:
            ignoredRule.isCaseSensitive = (item->checkState() == Qt::Checked);
            break;
        case CoreHighlightSettingsPage::SenderColumn:
            if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
                item->setText("");
            ignoredRule.sender = item->text();
            break;
        case CoreHighlightSettingsPage::ChanColumn:
            if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
                item->setText("");
            ignoredRule.chanName = item->text();
            break;
    }
    ignoredList[item->row()] = ignoredRule;
    emit widgetHasChanged();
}

void CoreHighlightSettingsPage::load()
{
    emptyHighlightTable();
    emptyIgnoredTable();

    auto ruleManager = Client::highlightRuleManager();
    if (ruleManager) {
        for (auto &rule : ruleManager->highlightRuleList()) {
            if (rule.isInverse) {
                addNewIgnoredRow(rule.isEnabled,
                                 rule.name,
                                 rule.isRegEx,
                                 rule.isCaseSensitive,
                                 rule.sender,
                                 rule.chanName);
            }
            else {
                addNewHighlightRow(rule.isEnabled, rule.name, rule.isRegEx, rule.isCaseSensitive, rule.sender,
                                   rule.chanName);
            }
        }

        int highlightNickType = ruleManager->highlightNick();
        ui.highlightNicksComboBox->setCurrentIndex(ui.highlightNicksComboBox->findData(QVariant(highlightNickType)));
        ui.nicksCaseSensitive->setChecked(ruleManager->nicksCaseSensitive());

        setChangedState(false);
        _initialized = true;
    } else {
        defaults();
    }
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

    for (auto &rule : highlightList) {
        clonedManager.addHighlightRule(rule.name, rule.isRegEx, rule.isCaseSensitive, rule.isEnabled, false,
                                       rule.sender, rule.chanName);
    }

    for (auto &rule : ignoredList) {
        clonedManager.addHighlightRule(rule.name, rule.isRegEx, rule.isCaseSensitive, rule.isEnabled, true,
                                       rule.sender, rule.chanName);
    }

    auto highlightNickType = ui.highlightNicksComboBox->itemData(ui.highlightNicksComboBox->currentIndex()).value<int>();

    clonedManager.setHighlightNick(HighlightRuleManager::HighlightNickType(highlightNickType));
    clonedManager.setNicksCaseSensitive(ui.nicksCaseSensitive->isChecked());

    ruleManager->requestUpdate(clonedManager.toVariantMap());
    setChangedState(false);
    load();
}

void CoreHighlightSettingsPage::widgetHasChanged()
{
    setChangedState(true);
}

void CoreHighlightSettingsPage::importRules() {
    NotificationSettings notificationSettings;

    auto clonedManager = HighlightRuleManager();
    clonedManager.fromVariantMap(Client::highlightRuleManager()->toVariantMap());

    for (const auto &variant : notificationSettings.highlightList()) {
        auto highlightRule = variant.toMap();

        clonedManager.addHighlightRule(
                highlightRule["Name"].toString(),
                highlightRule["RegEx"].toBool(),
                highlightRule["CS"].toBool(),
                highlightRule["Enable"].toBool(),
                false,
                "",
                highlightRule["Channel"].toString()
        );
    }

    Client::highlightRuleManager()->requestUpdate(clonedManager.toVariantMap());
    setChangedState(false);
    load();
}

bool CoreHighlightSettingsPage::isSelectable() const {
    return Client::isConnected() && Client::coreFeatures().testFlag(Quassel::Feature::CoreSideHighlights);
}
