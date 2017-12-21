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

#include "highlightsettingspage.h"

#include "client.h"
#include "qtui.h"
#include "uisettings.h"

#include <QHeaderView>

HighlightSettingsPage::HighlightSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Local Highlights"), parent)
{
    ui.setupUi(this);
    ui.highlightTable->verticalHeader()->hide();
    ui.highlightTable->setShowGrid(false);

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setToolTip("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setWhatsThis("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setToolTip("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setWhatsThis("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::ChanColumn)->setToolTip("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive.");
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::ChanColumn)->setWhatsThis("<b>Channel</b>: This regular expression determines for which <b>channels</b> the highlight rule works. Leave blank to match any channel. Put <b>!</b> in the beginning to negate. Case insensitive.");

#if QT_VERSION < 0x050000
    ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
#else
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(HighlightSettingsPage::NameColumn, QHeaderView::Stretch);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(HighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(HighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(HighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents);
    ui.highlightTable->horizontalHeader()->setSectionResizeMode(HighlightSettingsPage::ChanColumn, QHeaderView::ResizeToContents);
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
}


bool HighlightSettingsPage::hasDefaults() const
{
    return true;
}


void HighlightSettingsPage::defaults()
{
    ui.highlightCurrentNick->setChecked(true);
    ui.nicksCaseSensitive->setChecked(false);
    emptyTable();

    widgetHasChanged();
}


void HighlightSettingsPage::addNewRow(QString name, bool regex, bool cs, bool enable, QString chanName, bool self)
{
    ui.highlightTable->setRowCount(ui.highlightTable->rowCount()+1);

    QTableWidgetItem *nameItem = new QTableWidgetItem(name);

    QTableWidgetItem *regexItem = new QTableWidgetItem("");
    if (regex)
        regexItem->setCheckState(Qt::Checked);
    else
        regexItem->setCheckState(Qt::Unchecked);
    regexItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    QTableWidgetItem *csItem = new QTableWidgetItem("");
    if (cs)
        csItem->setCheckState(Qt::Checked);
    else
        csItem->setCheckState(Qt::Unchecked);
    csItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    QTableWidgetItem *enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    QTableWidgetItem *chanNameItem = new QTableWidgetItem(chanName);

    int lastRow = ui.highlightTable->rowCount()-1;
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::NameColumn, nameItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::RegExColumn, regexItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::CsColumn, csItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::EnableColumn, enableItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::ChanColumn, chanNameItem);

    if (!self)
        ui.highlightTable->setCurrentItem(nameItem);

    QVariantMap highlightRule;
    highlightRule["Name"] = name;
    highlightRule["RegEx"] = regex;
    highlightRule["CS"] = cs;
    highlightRule["Enable"] = enable;
    highlightRule["Channel"] = chanName;

    highlightList.append(highlightRule);
}


void HighlightSettingsPage::removeSelectedRows()
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


void HighlightSettingsPage::selectRow(QTableWidgetItem *item)
{
    int row = item->row();
    bool selected = item->isSelected();
    ui.highlightTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, HighlightSettingsPage::ColumnCount-1), selected);
}


void HighlightSettingsPage::emptyTable()
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


void HighlightSettingsPage::tableChanged(QTableWidgetItem *item)
{
    if (item->row()+1 > highlightList.size())
        return;

    QVariantMap highlightRule = highlightList.value(item->row()).toMap();

    switch (item->column())
    {
    case HighlightSettingsPage::NameColumn:
        if (item->text() == "")
            item->setText(tr("this shouldn't be empty"));
        highlightRule["Name"] = item->text();
        break;
    case HighlightSettingsPage::RegExColumn:
        highlightRule["RegEx"] = (item->checkState() == Qt::Checked);
        break;
    case HighlightSettingsPage::CsColumn:
        highlightRule["CS"] = (item->checkState() == Qt::Checked);
        break;
    case HighlightSettingsPage::EnableColumn:
        highlightRule["Enable"] = (item->checkState() == Qt::Checked);
        break;
    case HighlightSettingsPage::ChanColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        highlightRule["Channel"] = item->text();
        break;
    }
    highlightList[item->row()] = highlightRule;
    emit widgetHasChanged();
}


void HighlightSettingsPage::load()
{
    NotificationSettings notificationSettings;

    emptyTable();

    foreach(QVariant highlight, notificationSettings.highlightList()) {
        QVariantMap highlightRule = highlight.toMap();
        QString name = highlightRule["Name"].toString();
        bool regex = highlightRule["RegEx"].toBool();
        bool cs = highlightRule["CS"].toBool();
        bool enable = highlightRule["Enable"].toBool();
        QString chanName = highlightRule["Channel"].toString();

        addNewRow(name, regex, cs, enable, chanName, true);
    }

    switch (notificationSettings.highlightNick())
    {
    case NotificationSettings::NoNick:
        ui.highlightNoNick->setChecked(true);
        break;
    case NotificationSettings::CurrentNick:
        ui.highlightCurrentNick->setChecked(true);
        break;
    case NotificationSettings::AllNicks:
        ui.highlightAllNicks->setChecked(true);
        break;
    }
    ui.nicksCaseSensitive->setChecked(notificationSettings.nicksCaseSensitive());

    setChangedState(false);
}


void HighlightSettingsPage::save()
{
    NotificationSettings notificationSettings;
    notificationSettings.setHighlightList(highlightList);

    NotificationSettings::HighlightNickType highlightNickType = NotificationSettings::NoNick;
    if (ui.highlightCurrentNick->isChecked())
        highlightNickType = NotificationSettings::CurrentNick;
    if (ui.highlightAllNicks->isChecked())
        highlightNickType = NotificationSettings::AllNicks;

    notificationSettings.setHighlightNick(highlightNickType);
    notificationSettings.setNicksCaseSensitive(ui.nicksCaseSensitive->isChecked());

    load();
    setChangedState(false);
}


void HighlightSettingsPage::widgetHasChanged()
{
    bool changed = testHasChanged();
    if (changed != hasChanged()) setChangedState(changed);
}


bool HighlightSettingsPage::testHasChanged()
{
    NotificationSettings notificationSettings;

    NotificationSettings::HighlightNickType highlightNickType = NotificationSettings::NoNick;
    if (ui.highlightCurrentNick->isChecked())
        highlightNickType = NotificationSettings::CurrentNick;
    if (ui.highlightAllNicks->isChecked())
        highlightNickType = NotificationSettings::AllNicks;

    if (notificationSettings.highlightNick() != highlightNickType)
        return true;
    if (notificationSettings.nicksCaseSensitive() != ui.nicksCaseSensitive->isChecked())
        return true;
    if (notificationSettings.highlightList() != highlightList)
        return true;

    return false;
}

bool HighlightSettingsPage::isSelectable() const {
    return !Client::isConnected() || !Client::coreFeatures().testFlag(Quassel::Feature::CoreSideHighlights);
}
