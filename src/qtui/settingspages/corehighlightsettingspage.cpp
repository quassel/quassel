/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <algorithm>

#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>

#include "client.h"
#include "icon.h"
#include "qtui.h"
#include "util.h"

CoreHighlightSettingsPage::CoreHighlightSettingsPage(QWidget* parent)
    : SettingsPage(tr("Interface"), tr("Highlights"),
                   parent)
{
    ui.setupUi(this);

    setupRuleTable(ui.highlightTable);
    setupRuleTable(ui.ignoredTable);

    ui.highlightNicksComboBox->addItem(tr("All Nicks from Identity"), QVariant(HighlightRuleManager::AllNicks));
    ui.highlightNicksComboBox->addItem(tr("Current Nick"), QVariant(HighlightRuleManager::CurrentNick));
    ui.highlightNicksComboBox->addItem(tr("None"), QVariant(HighlightRuleManager::NoNick));

    coreConnectionStateChanged(Client::isConnected());  // need a core connection!
    connect(Client::instance(), &Client::coreConnectionStateChanged, this, &CoreHighlightSettingsPage::coreConnectionStateChanged);

    connect(ui.highlightAdd, &QAbstractButton::clicked, this, [this]() { addNewHighlightRow(); });
    connect(ui.highlightRemove, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::removeSelectedHighlightRows);
    connect(ui.highlightImport, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::importRules);

    connect(ui.ignoredAdd, &QAbstractButton::clicked, this, [this]() { addNewIgnoredRow(); });
    connect(ui.ignoredRemove, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::removeSelectedIgnoredRows);

    // TODO: search for a better signal (one that emits everytime a selection has been changed for one item)
    connect(ui.highlightTable, &QTableWidget::itemClicked, this, &CoreHighlightSettingsPage::selectHighlightRow);
    connect(ui.ignoredTable, &QTableWidget::itemClicked, this, &CoreHighlightSettingsPage::selectIgnoredRow);

    // Update the "Case sensitive" checkbox
    connect(ui.highlightNicksComboBox,
            selectOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &CoreHighlightSettingsPage::highlightNicksChanged);
    connect(ui.highlightNicksComboBox, selectOverload<int>(&QComboBox::currentIndexChanged), this, &CoreHighlightSettingsPage::widgetHasChanged);
    connect(ui.nicksCaseSensitive, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::widgetHasChanged);

    connect(ui.highlightAdd, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::widgetHasChanged);
    connect(ui.highlightRemove, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::widgetHasChanged);

    connect(ui.ignoredAdd, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::widgetHasChanged);
    connect(ui.ignoredRemove, &QAbstractButton::clicked, this, &CoreHighlightSettingsPage::widgetHasChanged);

    connect(ui.highlightTable, &QTableWidget::itemChanged, this, &CoreHighlightSettingsPage::highlightTableChanged);

    connect(ui.ignoredTable, &QTableWidget::itemChanged, this, &CoreHighlightSettingsPage::ignoredTableChanged);

    connect(Client::instance(), &Client::connected, this, &CoreHighlightSettingsPage::clientConnected);

    // Warning icon
    ui.coreUnsupportedIcon->setPixmap(icon::get("dialog-warning").pixmap(16));

    // Set up client/monolithic remote highlights information
    // Local highlights are considered legacy
    ui.highlightImport->setText(tr("Import Legacy"));
    ui.highlightImport->setToolTip(
        tr("Import highlight rules configured in <i>%1</i>.").arg(tr("Legacy Highlights").replace(" ", "&nbsp;")));
    // Re-use translations of "Legacy Highlights" as this is a word-for-word reference, forcing
    // all spaces to be non-breaking
}

void CoreHighlightSettingsPage::coreConnectionStateChanged(bool state)
{
    updateCoreSupportStatus(state);
    setEnabled(state);
    if (state) {
        load();
    }
    else {
        revert();
    }
}

void CoreHighlightSettingsPage::setupRuleTable(QTableWidget* table) const
{
    table->verticalHeader()->hide();
    table->setShowGrid(false);

    setupTableTooltips(table->horizontalHeaderItem(CoreHighlightSettingsPage::EnableColumn),
                       table->horizontalHeaderItem(CoreHighlightSettingsPage::NameColumn),
                       table->horizontalHeaderItem(CoreHighlightSettingsPage::RegExColumn),
                       table->horizontalHeaderItem(CoreHighlightSettingsPage::CsColumn),
                       table->horizontalHeaderItem(CoreHighlightSettingsPage::SenderColumn),
                       table->horizontalHeaderItem(CoreHighlightSettingsPage::ChanColumn));

    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::SenderColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(CoreHighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
}

QString CoreHighlightSettingsPage::getTableTooltip(column tableColumn) const
{
    switch (tableColumn) {
    case CoreHighlightSettingsPage::EnableColumn:
        return tr("Enable/disable this rule");

    case CoreHighlightSettingsPage::NameColumn:
        return tr("Phrase to match, leave blank to match any message");

    case CoreHighlightSettingsPage::RegExColumn:
        return tr("<b>RegEx</b>: This option determines if the highlight rule, <i>Sender</i>, and "
                  "<i>Channel</i> should be interpreted as <b>regular expressions</b> or just as "
                  "keywords.");

    case CoreHighlightSettingsPage::CsColumn:
        return tr("<b>CS</b>: This option determines if the highlight rule, <i>Sender</i>, and "
                  "<i>Channel</i> should be interpreted <b>case sensitive</b>.");

    case CoreHighlightSettingsPage::SenderColumn:
        return tr("<p><b>Sender</b>: Semicolon separated list of <i>nick!ident@host</i> names, "
                  "leave blank to match any nickname.</p>"
                  "<p><i>Example:</i><br />"
                  "<i>Alice!*; Bob!*@example.com; Carol*!*; !Caroline!*</i><br />"
                  "would match on <i>Alice</i>, <i>Bob</i> with hostmask <i>example.com</i>, and "
                  "any nickname starting with <i>Carol</i> except for <i>Caroline</i><br />"
                  "<p>If only inverted names are specified, it will match anything except for "
                  "what's specified (implicit wildcard).</p>"
                  "<p><i>Example:</i><br />"
                  "<i>!Announce*!*; !Wheatley!aperture@*</i><br />"
                  "would match anything except for <i>Wheatley</i> with ident <i>aperture</i> or "
                  "any nickname starting with <i>Announce</i></p>");

    case CoreHighlightSettingsPage::ChanColumn:
        return tr("<p><b>Channel</b>: Semicolon separated list of channel/query names, leave blank "
                  "to match any name.</p>"
                  "<p><i>Example:</i><br />"
                  "<i>#quassel*; #foobar; !#quasseldroid</i><br />"
                  "would match on <i>#foobar</i> and any channel starting with <i>#quassel</i> "
                  "except for <i>#quasseldroid</i><br />"
                  "<p>If only inverted names are specified, it will match anything except for "
                  "what's specified (implicit wildcard).</p>"
                  "<p><i>Example:</i><br />"
                  "<i>!#quassel*; !#foobar</i><br />"
                  "would match anything except for <i>#foobar</i> or any channel starting with "
                  "<i>#quassel</i></p>");

    default:
        // This shouldn't happen
        return "Invalid column type in CoreHighlightSettingsPage::getTableTooltip()";
    }
}

void CoreHighlightSettingsPage::setupTableTooltips(
    QWidget* enableWidget, QWidget* nameWidget, QWidget* regExWidget, QWidget* csWidget, QWidget* senderWidget, QWidget* chanWidget) const
{
    // Make sure everything's valid
    Q_ASSERT(enableWidget);
    Q_ASSERT(nameWidget);
    Q_ASSERT(regExWidget);
    Q_ASSERT(csWidget);
    Q_ASSERT(senderWidget);
    Q_ASSERT(chanWidget);

    // Set tooltips and "What's this?" prompts
    // Enabled
    enableWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::EnableColumn));
    enableWidget->setWhatsThis(enableWidget->toolTip());

    // Name
    nameWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::NameColumn));
    nameWidget->setWhatsThis(nameWidget->toolTip());

    // RegEx enabled
    regExWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::RegExColumn));
    regExWidget->setWhatsThis(regExWidget->toolTip());

    // Case-sensitivity
    csWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::CsColumn));
    csWidget->setWhatsThis(csWidget->toolTip());

    // Sender
    senderWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::SenderColumn));
    senderWidget->setWhatsThis(senderWidget->toolTip());

    // Channel/buffer name
    chanWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::ChanColumn));
    chanWidget->setWhatsThis(chanWidget->toolTip());
}

void CoreHighlightSettingsPage::setupTableTooltips(QTableWidgetItem* enableWidget,
                                                   QTableWidgetItem* nameWidget,
                                                   QTableWidgetItem* regExWidget,
                                                   QTableWidgetItem* csWidget,
                                                   QTableWidgetItem* senderWidget,
                                                   QTableWidgetItem* chanWidget) const
{
    // Make sure everything's valid
    Q_ASSERT(enableWidget);
    Q_ASSERT(nameWidget);
    Q_ASSERT(regExWidget);
    Q_ASSERT(csWidget);
    Q_ASSERT(senderWidget);
    Q_ASSERT(chanWidget);

    // Set tooltips and "What's this?" prompts
    // Enabled
    enableWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::EnableColumn));
    enableWidget->setWhatsThis(enableWidget->toolTip());

    // Name
    nameWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::NameColumn));
    nameWidget->setWhatsThis(nameWidget->toolTip());

    // RegEx enabled
    regExWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::RegExColumn));
    regExWidget->setWhatsThis(regExWidget->toolTip());

    // Case-sensitivity
    csWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::CsColumn));
    csWidget->setWhatsThis(csWidget->toolTip());

    // Sender
    senderWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::SenderColumn));
    senderWidget->setWhatsThis(senderWidget->toolTip());

    // Channel/buffer name
    chanWidget->setToolTip(getTableTooltip(CoreHighlightSettingsPage::ChanColumn));
    chanWidget->setWhatsThis(chanWidget->toolTip());
}

void CoreHighlightSettingsPage::updateCoreSupportStatus(bool state)
{
    // Assume connected state as enforced by the settings page UI
    if (!state || Client::isCoreFeatureEnabled(Quassel::Feature::CoreSideHighlights)) {
        // Either disconnected or core supports highlights, enable highlight configuration and hide
        // warning.  Don't show the warning needlessly when disconnected.
        ui.highlightsConfigWidget->setEnabled(true);
        ui.coreUnsupportedWidget->setVisible(false);
    }
    else {
        // Core does not support highlights, show warning and disable highlight configuration
        ui.highlightsConfigWidget->setEnabled(false);
        ui.coreUnsupportedWidget->setVisible(true);
    }
}

void CoreHighlightSettingsPage::clientConnected()
{
    connect(Client::highlightRuleManager(), &SyncableObject::updated, this, &CoreHighlightSettingsPage::revert);
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

void CoreHighlightSettingsPage::addNewHighlightRow(
    bool enable, int id, const QString& name, bool regex, bool cs, const QString& sender, const QString& chanName, bool self)
{
    ui.highlightTable->setRowCount(ui.highlightTable->rowCount() + 1);

    if (id < 0) {
        id = nextId();
    }

    auto* nameItem = new QTableWidgetItem(name);

    auto* regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* senderItem = new QTableWidgetItem(sender);

    auto* chanNameItem = new QTableWidgetItem(chanName);

    setupTableTooltips(enableItem, nameItem, regexItem, csItem, senderItem, chanNameItem);

    int lastRow = ui.highlightTable->rowCount() - 1;
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::NameColumn, nameItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::RegExColumn, regexItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::CsColumn, csItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::EnableColumn, enableItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::SenderColumn, senderItem);
    ui.highlightTable->setItem(lastRow, CoreHighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.highlightTable->setCurrentItem(nameItem);

    highlightList << HighlightRuleManager::HighlightRule(id, name, regex, cs, enable, false, sender, chanName);
}

void CoreHighlightSettingsPage::addNewIgnoredRow(
    bool enable, int id, const QString& name, bool regex, bool cs, const QString& sender, const QString& chanName, bool self)
{
    ui.ignoredTable->setRowCount(ui.ignoredTable->rowCount() + 1);

    if (id < 0) {
        id = nextId();
    }

    auto* nameItem = new QTableWidgetItem(name);

    auto* regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto* chanNameItem = new QTableWidgetItem(chanName);

    auto* senderItem = new QTableWidgetItem(sender);

    setupTableTooltips(enableItem, nameItem, regexItem, csItem, senderItem, chanNameItem);

    int lastRow = ui.ignoredTable->rowCount() - 1;
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::NameColumn, nameItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::RegExColumn, regexItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::CsColumn, csItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::EnableColumn, enableItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::SenderColumn, senderItem);
    ui.ignoredTable->setItem(lastRow, CoreHighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.ignoredTable->setCurrentItem(nameItem);

    ignoredList << HighlightRuleManager::HighlightRule(id, name, regex, cs, enable, true, sender, chanName);
}

void CoreHighlightSettingsPage::removeSelectedHighlightRows()
{
    QList<int> selectedRows;
    QList<QTableWidgetItem*> selectedItemList = ui.highlightTable->selectedItems();
    for (auto selectedItem : selectedItemList) {
        selectedRows.append(selectedItem->row());
    }
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<>());
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
    QList<QTableWidgetItem*> selectedItemList = ui.ignoredTable->selectedItems();
    for (auto selectedItem : selectedItemList) {
        selectedRows.append(selectedItem->row());
    }
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<>());
    int lastRow = -1;
    for (auto row : selectedRows) {
        if (row != lastRow) {
            ui.ignoredTable->removeRow(row);
            ignoredList.removeAt(row);
        }
        lastRow = row;
    }
}

void CoreHighlightSettingsPage::highlightNicksChanged(const int index)
{
    // Only allow toggling "Case sensitive" when a nickname will be highlighted
    auto highlightNickType = ui.highlightNicksComboBox->itemData(index).value<int>();
    ui.nicksCaseSensitive->setEnabled(highlightNickType != HighlightRuleManager::NoNick);
}

void CoreHighlightSettingsPage::selectHighlightRow(QTableWidgetItem* item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.highlightTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, CoreHighlightSettingsPage::ColumnCount - 1), selected);
}

void CoreHighlightSettingsPage::selectIgnoredRow(QTableWidgetItem* item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.ignoredTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, CoreHighlightSettingsPage::ColumnCount - 1), selected);
}

void CoreHighlightSettingsPage::emptyHighlightTable()
{
    // ui.highlight and highlightList should have the same size, but just to make sure.
    if (ui.highlightTable->rowCount() != highlightList.size()) {
        qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
    }
    while (ui.highlightTable->rowCount()) {
        ui.highlightTable->removeRow(0);
    }
    highlightList.clear();
}

void CoreHighlightSettingsPage::emptyIgnoredTable()
{
    // ui.highlight and highlightList should have the same size, but just to make sure.
    if (ui.ignoredTable->rowCount() != ignoredList.size()) {
        qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
    }
    while (ui.ignoredTable->rowCount()) {
        ui.ignoredTable->removeRow(0);
    }
    ignoredList.clear();
}

void CoreHighlightSettingsPage::highlightTableChanged(QTableWidgetItem* item)
{
    if (item->row() + 1 > highlightList.size())
        return;

    auto highlightRule = highlightList.value(item->row());

    switch (item->column()) {
    case CoreHighlightSettingsPage::EnableColumn:
        highlightRule.setIsEnabled(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::NameColumn:
        highlightRule.setContents(item->text());
        break;
    case CoreHighlightSettingsPage::RegExColumn:
        highlightRule.setIsRegEx(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::CsColumn:
        highlightRule.setIsCaseSensitive(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::SenderColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        highlightRule.setSender(item->text());
        break;
    case CoreHighlightSettingsPage::ChanColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        highlightRule.setChanName(item->text());
        break;
    }
    highlightList[item->row()] = highlightRule;
    emit widgetHasChanged();
}

void CoreHighlightSettingsPage::ignoredTableChanged(QTableWidgetItem* item)
{
    if (item->row() + 1 > ignoredList.size())
        return;

    auto ignoredRule = ignoredList.value(item->row());

    switch (item->column()) {
    case CoreHighlightSettingsPage::EnableColumn:
        ignoredRule.setIsEnabled(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::NameColumn:
        ignoredRule.setContents(item->text());
        break;
    case CoreHighlightSettingsPage::RegExColumn:
        ignoredRule.setIsRegEx(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::CsColumn:
        ignoredRule.setIsCaseSensitive(item->checkState() == Qt::Checked);
        break;
    case CoreHighlightSettingsPage::SenderColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        ignoredRule.setSender(item->text());
        break;
    case CoreHighlightSettingsPage::ChanColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        ignoredRule.setChanName(item->text());
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
        for (auto& rule : ruleManager->highlightRuleList()) {
            if (rule.isInverse()) {
                addNewIgnoredRow(rule.isEnabled(),
                                 rule.id(),
                                 rule.contents(),
                                 rule.isRegEx(),
                                 rule.isCaseSensitive(),
                                 rule.sender(),
                                 rule.chanName());
            }
            else {
                addNewHighlightRow(rule.isEnabled(),
                                   rule.id(),
                                   rule.contents(),
                                   rule.isRegEx(),
                                   rule.isCaseSensitive(),
                                   rule.sender(),
                                   rule.chanName());
            }
        }

        int highlightNickType = ruleManager->highlightNick();
        ui.highlightNicksComboBox->setCurrentIndex(ui.highlightNicksComboBox->findData(QVariant(highlightNickType)));
        // Trigger the initial update of nicksCaseSensitive being enabled or not
        highlightNicksChanged(ui.highlightNicksComboBox->currentIndex());
        ui.nicksCaseSensitive->setChecked(ruleManager->nicksCaseSensitive());

        setChangedState(false);
        _initialized = true;
    }
    else {
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

    HighlightRuleManager clonedManager;
    clonedManager.fromVariantMap(ruleManager->toVariantMap());
    clonedManager.clear();

    for (auto& rule : highlightList) {
        clonedManager.addHighlightRule(rule.id(),
                                       rule.contents(),
                                       rule.isRegEx(),
                                       rule.isCaseSensitive(),
                                       rule.isEnabled(),
                                       false,
                                       rule.sender(),
                                       rule.chanName());
    }

    for (auto& rule : ignoredList) {
        clonedManager.addHighlightRule(rule.id(),
                                       rule.contents(),
                                       rule.isRegEx(),
                                       rule.isCaseSensitive(),
                                       rule.isEnabled(),
                                       true,
                                       rule.sender(),
                                       rule.chanName());
    }

    auto highlightNickType = ui.highlightNicksComboBox->itemData(ui.highlightNicksComboBox->currentIndex()).value<int>();

    clonedManager.setHighlightNick(HighlightRuleManager::HighlightNickType(highlightNickType));
    clonedManager.setNicksCaseSensitive(ui.nicksCaseSensitive->isChecked());

    ruleManager->requestUpdate(clonedManager.toVariantMap());
    setChangedState(false);
    load();
}

int CoreHighlightSettingsPage::nextId()
{
    int max = 0;
    for (int i = 0; i < highlightList.count(); i++) {
        int id = highlightList[i].id();
        if (id > max) {
            max = id;
        }
    }
    for (int i = 0; i < ignoredList.count(); i++) {
        int id = ignoredList[i].id();
        if (id > max) {
            max = id;
        }
    }
    return max + 1;
}

void CoreHighlightSettingsPage::widgetHasChanged()
{
    setChangedState(true);
}

void CoreHighlightSettingsPage::on_coreUnsupportedDetails_clicked()
{
    // Re-use translations of "Legacy Highlights" as this is a word-for-word reference, forcing all
    // spaces to non-breaking
    const QString localHighlightsName = tr("Legacy Highlights").replace(" ", "&nbsp;");

    const QString remoteHighlightsMsgText = QString("<p><b>%1</b></p></br><p>%2</p></br><p>%3</p>")
                                                .arg(tr("Your Quassel core is too old to support remote highlights"),
                                                     tr("You need a Quassel core v0.13.0 or newer to configure remote "
                                                        "highlights."),
                                                     tr("You can still configure highlights for this device only in "
                                                        "<i>%1</i>.")
                                                         .arg(localHighlightsName));

    QMessageBox::warning(this, tr("Remote Highlights unsupported"), remoteHighlightsMsgText);
}

void CoreHighlightSettingsPage::importRules()
{
    NotificationSettings notificationSettings;

    const auto localHighlightList = notificationSettings.highlightList();

    // Re-use translations of "Legacy/Local Highlights" as this is a word-for-word reference,
    // forcing all spaces to non-breaking
    // "Local Highlights" has been removed; it's always called "Legacy" now.
    QString localHighlightsName = tr("Legacy Highlights").replace(" ", "&nbsp;");

    if (localHighlightList.count() == 0) {
        // No highlight rules exist to import, do nothing
        QMessageBox::information(this, tr("No highlights to import"), tr("No highlight rules in <i>%1</i>.").arg(localHighlightsName));
        return;
    }

    int ret = QMessageBox::question(this,
                                    tr("Import highlights?"),
                                    tr("Import all highlight rules from <i>%1</i>?").arg(localHighlightsName),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        // Only two options, Yes or No, return if not Yes
        return;
    }

    if (hasChanged()) {
        // Save existing changes first to avoid overwriting them
        save();
    }

    HighlightRuleManager clonedManager;
    clonedManager.fromVariantMap(Client::highlightRuleManager()->toVariantMap());

    for (const auto& variant : notificationSettings.highlightList()) {
        auto highlightRule = variant.toMap();

        clonedManager.addHighlightRule(clonedManager.nextId(),
                                       highlightRule["Name"].toString(),
                                       highlightRule["RegEx"].toBool(),
                                       highlightRule["CS"].toBool(),
                                       highlightRule["Enable"].toBool(),
                                       false,
                                       "",
                                       highlightRule["Channel"].toString());
    }

    // Copy nickname highlighting settings
    clonedManager.setNicksCaseSensitive(notificationSettings.nicksCaseSensitive());
    if (notificationSettings.highlightNick() == NotificationSettings::HighlightNickType::AllNicks) {
        clonedManager.setHighlightNick(HighlightRuleManager::HighlightNickType::AllNicks);
    }
    else if (notificationSettings.highlightNick() == NotificationSettings::HighlightNickType::CurrentNick) {
       clonedManager.setHighlightNick(HighlightRuleManager::HighlightNickType::CurrentNick);
    }
    // else - Don't copy "NoNick", "NoNick" is now default and should be ignored

    Client::highlightRuleManager()->requestUpdate(clonedManager.toVariantMap());
    setChangedState(false);
    load();

    // Give a heads-up that all succeeded, ask about removing old rules
    //
    // Hypothetically, someone might use a common set of highlight rules across multiple cores.
    // This won't matter once client highlights are disabled entirely on newer cores.
    //
    // Remove this once client-side highlights are disabled for newer cores.
    ret = QMessageBox::question(this, tr("Imported highlights"),
                                QString("<p>%1</p></br><p>%2</p>").arg(
                                    tr("%1 highlight rules successfully imported.").arg(QString::number(localHighlightList.count())),
                                    tr("Clean up old, duplicate highlight rules?")),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::Yes);

    if (ret != QMessageBox::Yes) {
        // Only two options, Yes or No, return if not Yes
        return;
    }

    // Remove all local highlight rules
    notificationSettings.setHighlightList({});
    // Disable local nickname highlighting
    notificationSettings.setHighlightNick(NotificationSettings::HighlightNickType::NoNick);
    // Disable nickname sensitivity
    // This isn't needed to disable local highlights, but it's part of appearing reset-to-default
    notificationSettings.setNicksCaseSensitive(false);

    // Refresh HighlightSettingsPage in case it was already loaded
    emit localHighlightsChanged();
}

bool CoreHighlightSettingsPage::isSelectable() const
{
    return Client::isConnected();
    // We check for Quassel::Feature::CoreSideHighlights when loading this page, allowing us to show
    // a friendly error message.
}
