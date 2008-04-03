/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Highlight Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Highlight Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Highlight Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "highlightsettingspage.h"

#include "qtui.h"
#include "uisettings.h"

#include <QHeaderView>


HighlightSettingsPage::HighlightSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("Highlight"), parent) {
  ui.setupUi(this);
  ui.highlightTable->verticalHeader()->hide();
  ui.highlightTable->setShowGrid(false);

  ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setToolTip("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");
  ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::RegExColumn)->setWhatsThis("<b>RegEx</b>: This option determines if the highlight rule should be interpreted as a <b>regular expression</b> or just as a keyword.");

  ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setToolTip("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");
  ui.highlightTable->horizontalHeaderItem(HighlightSettingsPage::CsColumn)->setWhatsThis("<b>CS</b>: This option determines if the highlight rule should be interpreted <b>case sensitive</b>.");

  ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::NameColumn, QHeaderView::Stretch);
  ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::RegExColumn, QHeaderView::ResizeToContents); 
  ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::CsColumn, QHeaderView::ResizeToContents); 
  ui.highlightTable->horizontalHeader()->setResizeMode(HighlightSettingsPage::EnableColumn, QHeaderView::ResizeToContents); 

  connect(ui.add, SIGNAL(clicked(bool)), this, SLOT(addNewRow()));
  connect(ui.remove, SIGNAL(clicked(bool)), this, SLOT(removeSelectedRows()));
  //TODO: search for a better signal (one that emits everytime a selection has been changed for one item)
  connect(ui.highlightTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(selectRow(QTableWidgetItem *)));

  connect(ui.highlightAllNicks, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.highlightCurrentNick, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.highlightNoNick, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.add, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.remove, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.highlightTable, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(tableChanged(QTableWidgetItem *)));
}

bool HighlightSettingsPage::hasDefaults() const {
  return true;
}

void HighlightSettingsPage::defaults() {
  ui.highlightCurrentNick->setChecked(true);
  emptyTable();

  widgetHasChanged();
}

void HighlightSettingsPage::addNewRow(QString name, bool regex, bool cs, bool enable) {
  ui.highlightTable->setRowCount(ui.highlightTable->rowCount()+1);

  QTableWidgetItem *nameItem = new QTableWidgetItem(name);

  QTableWidgetItem *regexItem = new QTableWidgetItem("");
  if(regex)
    regexItem->setCheckState(Qt::Checked);
  else
    regexItem->setCheckState(Qt::Unchecked);
  regexItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

  QTableWidgetItem *csItem = new QTableWidgetItem("");
  if(cs)
    csItem->setCheckState(Qt::Checked);
  else
    csItem->setCheckState(Qt::Unchecked);
  csItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

  QTableWidgetItem *enableItem = new QTableWidgetItem("");
  if(enable)
    enableItem->setCheckState(Qt::Checked);
  else
    enableItem->setCheckState(Qt::Unchecked);
  enableItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

  int lastRow = ui.highlightTable->rowCount()-1;
  ui.highlightTable->setItem(lastRow, HighlightSettingsPage::NameColumn, nameItem);
  ui.highlightTable->setItem(lastRow, HighlightSettingsPage::RegExColumn, regexItem);
  ui.highlightTable->setItem(lastRow, HighlightSettingsPage::CsColumn, csItem);
  ui.highlightTable->setItem(lastRow, HighlightSettingsPage::EnableColumn, enableItem);

  QVariantMap highlightRule;
  highlightRule["name"] = name;
  highlightRule["regex"] = regex;
  highlightRule["cs"] = cs;
  highlightRule["enable"] = enable;

  highlightList.append(highlightRule);
}

void HighlightSettingsPage::removeSelectedRows() {
  QList<int> selectedRows;
  QList<QTableWidgetItem *> selectedItemList = ui.highlightTable->selectedItems();
  foreach(QTableWidgetItem *selectedItem, selectedItemList) {
    selectedRows.append(selectedItem->row());
  }
  qSort(selectedRows.begin(), selectedRows.end(), qGreater<int>());
  int lastRow = -1;
  foreach(int row, selectedRows) {
    if(row != lastRow) {
      ui.highlightTable->removeRow(row);
      highlightList.removeAt(row);
    }
    lastRow = row;
  }
}

void HighlightSettingsPage::selectRow(QTableWidgetItem *item) {
  int row = item->row();
  bool selected = item->isSelected();
  ui.highlightTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, HighlightSettingsPage::ColumnCount-1), selected);
}

void HighlightSettingsPage::emptyTable() {
  // ui.highlight and highlightList should have the same size, but just to make sure.
  if(ui.highlightTable->rowCount() != highlightList.size()) {
    qDebug() << "something is wrong: ui.highlight and highlightList don't have the same size!";
  }
  while(ui.highlightTable->rowCount()) {
    ui.highlightTable->removeRow(0);
  }
  while(highlightList.size()) {
        highlightList.removeLast();
  }
}

void HighlightSettingsPage::tableChanged(QTableWidgetItem *item) {
  if(item->row()+1 > highlightList.size()) 
    return;

  QVariantMap highlightRule = highlightList.value(item->row()).toMap();

  switch(item->column())
  {
    case HighlightSettingsPage::NameColumn:
      if(item->text() == "")
        item->setText(tr("this shouldn't be empty"));
      highlightRule["name"] = item->text();
      break;
    case HighlightSettingsPage::RegExColumn:
      highlightRule["regex"] = (item->checkState() == Qt::Checked);
      break;
    case HighlightSettingsPage::CsColumn:
      highlightRule["cs"] = (item->checkState() == Qt::Checked);
      break;
    case HighlightSettingsPage::EnableColumn:
      highlightRule["enable"] = (item->checkState() == Qt::Checked);
      break;
  }
  highlightList[item->row()] = highlightRule;
  emit widgetHasChanged();
}

void HighlightSettingsPage::load() {
  NotificationSettings notificationSettings;

  emptyTable();

  foreach(QVariant highlight, notificationSettings.highlightList()) {
    QVariantMap highlightRule = highlight.toMap();
    QString name = highlightRule["name"].toString();
    bool regex = highlightRule["regex"].toBool();
    bool cs = highlightRule["cs"].toBool();
    bool enable = highlightRule["enable"].toBool();

    addNewRow(name, regex, cs, enable);
  }

  switch(notificationSettings.highlightNick())
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

  setChangedState(false);
}

void HighlightSettingsPage::save() {
  NotificationSettings notificationSettings;
  notificationSettings.setHighlightList(highlightList);

  NotificationSettings::HighlightNickType highlightNickType;
  if(ui.highlightNoNick->isChecked()) 
    highlightNickType = NotificationSettings::NoNick;
  if(ui.highlightCurrentNick->isChecked()) 
    highlightNickType = NotificationSettings::CurrentNick;
  if(ui.highlightAllNicks->isChecked()) 
    highlightNickType = NotificationSettings::AllNicks;

  notificationSettings.setHighlightNick(highlightNickType);

  load();
  setChangedState(false);
}

void HighlightSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool HighlightSettingsPage::testHasChanged() {
  NotificationSettings notificationSettings;

  NotificationSettings::HighlightNickType highlightNickType;
  if(ui.highlightNoNick->isChecked()) 
    highlightNickType = NotificationSettings::NoNick;
  if(ui.highlightCurrentNick->isChecked()) 
    highlightNickType = NotificationSettings::CurrentNick;
  if(ui.highlightAllNicks->isChecked()) 
    highlightNickType = NotificationSettings::AllNicks;

  if(notificationSettings.highlightNick() != highlightNickType) return true;

  if(notificationSettings.highlightList() != highlightList) return true;

  return true;
}




