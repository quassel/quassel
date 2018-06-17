/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QHeaderView>
#include <QMessageBox>

#include "client.h"
#include "icon.h"
#include "qtui.h"
#include "uisettings.h"


HighlightSettingsPage::HighlightSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"),
                   // In Monolithic mode, local highlights are replaced by remote highlights
                   Quassel::runMode() == Quassel::Monolithic ?
                       tr("Legacy Highlights") : tr("Local Highlights"),
                   parent)
{
    ui.setupUi(this);
    ui.highlightTable->verticalHeader()->hide();
    ui.highlightTable->setShowGrid(false);


    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::EnableColumn)->setToolTip(
                tr("Enable/disable this rule"));
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::EnableColumn)->setWhatsThis(
                ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::EnableColumn)->toolTip());

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::NameColumn)->setToolTip(
                tr("Phrase to match"));
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::NameColumn)->setWhatsThis(
                ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::NameColumn)->toolTip());

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setToolTip(
                tr("<b>RegEx</b>: This option determines if the highlight rule and <i>Channel</i> "
                   "should be interpreted as <b>regular expressions</b> or just as keywords."));
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setWhatsThis(
                ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->toolTip());

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setToolTip(
                tr("<b>CS</b>: This option determines if the highlight rule and <i>Channel</i> "
                   "should be interpreted <b>case sensitive</b>."));
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setWhatsThis(
                ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->toolTip());

    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::ChanColumn)->setToolTip(
                tr("<p><b>Channel</b>: Semicolon separated list of channel names, leave blank to "
                   "match any name.</p>"
                   "<p><i>Example:</i><br />"
                   "<i>#quassel*; #foobar; !#quasseldroid</i><br />"
                   "would match on <i>#foobar</i> and any channel starting with <i>#quassel</i> "
                   "except for <i>#quasseldroid</i><br />"
                   "<p>If only inverted names are specified, it will match anything except for "
                   "what's specified (implicit wildcard).</p>"
                   "<p><i>Example:</i><br />"
                   "<i>!#quassel*; !#foobar</i><br />"
                   "would match anything except for <i>#foobar</i> or any channel starting with "
                   "<i>#quassel</i></p>"));
    ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::ChanColumn)->setWhatsThis(
                ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::ChanColumn)->toolTip());

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

    // Information icon
    ui.localHighlightsIcon->setPixmap(icon::get("dialog-information").pixmap(16));

    // Set up client/monolithic local highlights information
    if (Quassel::runMode() == Quassel::Monolithic) {
        // We're running in Monolithic mode, core/client version in total sync.  Discourage the use
        // of local (legacy) highlights as it's identical to setting remote highlights.
        ui.localHighlightsLabel->setText(
                    tr("Legacy Highlights are replaced by Highlights"));
    } else {
        // We're running in client/split mode, allow for splitting the details.
        ui.localHighlightsLabel->setText(tr("Local Highlights apply to this device only"));
    }

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
    ui.highlightNoNick->setChecked(true);
    ui.nicksCaseSensitive->setChecked(false);
    emptyTable();

    widgetHasChanged();
}


void HighlightSettingsPage::addNewRow(QString name, bool regex, bool cs, bool enable, QString chanName, bool self)
{
    ui.highlightTable->setRowCount(ui.highlightTable->rowCount()+1);

    QTableWidgetItem *enableItem = new QTableWidgetItem("");
    if (enable)
        enableItem->setCheckState(Qt::Checked);
    else
        enableItem->setCheckState(Qt::Unchecked);
    enableItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

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

    QTableWidgetItem *chanNameItem = new QTableWidgetItem(chanName);

    enableItem->setToolTip(tr("Enable/disable this rule"));
    nameItem->setToolTip(tr("Phrase to match"));
    regexItem->setToolTip(
                tr("<b>RegEx</b>: This option determines if the highlight rule and <i>Channel</i> "
                   "should be interpreted as <b>regular expressions</b> or just as keywords."));
    csItem->setToolTip(
                tr("<b>CS</b>: This option determines if the highlight rule and <i>Channel</i> "
                   "should be interpreted <b>case sensitive</b>."));
    chanNameItem->setToolTip(
                tr("<p><b>Channel</b>: Semicolon separated list of channel names, leave blank to "
                   "match any name.</p>"
                   "<p><i>Example:</i><br />"
                   "<i>#quassel*; #foobar; !#quasseldroid</i><br />"
                   "would match on <i>#foobar</i> and any channel starting with <i>#quassel</i> "
                   "except for <i>#quasseldroid</i><br />"
                   "<p>If only inverted names are specified, it will match anything except for "
                   "what's specified (implicit wildcard).</p>"
                   "<p><i>Example:</i><br />"
                   "<i>!#quassel*; !#foobar</i><br />"
                   "would match anything except for <i>#foobar</i> or any channel starting with "
                   "<i>#quassel</i></p>"));

    int lastRow = ui.highlightTable->rowCount()-1;
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::EnableColumn, enableItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::NameColumn, nameItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::RegExColumn, regexItem);
    ui.highlightTable->setItem(lastRow, HighlightSettingsPage::CsColumn, csItem);
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
    case HighlightSettingsPage::EnableColumn:
        highlightRule["Enable"] = (item->checkState() == Qt::Checked);
        break;
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
    case HighlightSettingsPage::ChanColumn:
        if (!item->text().isEmpty() && item->text().trimmed().isEmpty())
            item->setText("");
        highlightRule["Channel"] = item->text();
        break;
    }
    highlightList[item->row()] = highlightRule;
    emit widgetHasChanged();
}


void HighlightSettingsPage::on_localHighlightsDetails_clicked()
{
    // Show information specific to client/monolithic differences
    if (Quassel::runMode() == Quassel::Monolithic) {
        // We're running in Monolithic mode, core/client version in total sync.  Discourage the use
        // of local (legacy) highlights as it's identical to setting remote highlights.
        QMessageBox::information(
                    this,
                    tr("Legacy Highlights vs. Highlights"),
                    QString("<p><b>%1</b></p></br><p>%2</p></br><p>%3</p>")
                    .arg(tr("Legacy Highlights are replaced by Highlights"),
                         tr("These highlights will keep working for now, but you should move to "
                            "the improved highlight rules when you can."),
                         tr("Configure the new style of highlights in "
                            "<i>%1</i>.").arg(tr("Highlights"))));
    } else {
        // We're running in client/split mode, allow for splitting the details.
        QMessageBox::information(
                    this,
                    tr("Local Highlights vs. Remote Highlights"),
                    QString("<p><b>%1</b></p></br><p>%2</p></br><p>%3</p>")
                    .arg(tr("Local Highlights apply to this device only"),
                         tr("Highlights configured on this page only apply to your current "
                            "device."),
                         tr("Configure highlights for all of your devices in "
                            "<i>%1</i>.").arg(tr("Remote Highlights").replace(" ", "&nbsp;"))));
        // Re-use translations of "Remote Highlights" as this is a word-for-word reference, forcing
        // all spaces to be non-breaking
    }
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
