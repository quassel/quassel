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
  ui.highlightTable->setColumnWidth( 0, 50 );
  ui.highlightTable->setColumnWidth( 2, 50 );
  ui.highlightTable->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed); 
  ui.highlightTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
  ui.highlightTable->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed); 

  connect(ui.add, SIGNAL(clicked(bool)), this, SLOT(addNewRow()));
  connect(ui.remove, SIGNAL(clicked(bool)), this, SLOT(removeSelectedRows()));
  //TODO: search for a better signal (one that emits everytime a selection has been changed for one item)
  connect(ui.highlightTable, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(selectRow(QTableWidgetItem *)));

  connect(ui.highlightCurrentNick, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
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

void HighlightSettingsPage::addNewRow(bool regex, QString name, bool enable) {
  ui.highlightTable->setRowCount(ui.highlightTable->rowCount()+1);
  QTableWidgetItem *regexItem = new QTableWidgetItem("");
  if(regex)
    regexItem->setCheckState(Qt::Checked);
  else
    regexItem->setCheckState(Qt::Unchecked);
  regexItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
  QTableWidgetItem *nameItem = new QTableWidgetItem(name);
  QTableWidgetItem *enableItem = new QTableWidgetItem("");
  if(enable)
    enableItem->setCheckState(Qt::Checked);
  else
    enableItem->setCheckState(Qt::Unchecked);
  enableItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

  int lastRow = ui.highlightTable->rowCount()-1;
  ui.highlightTable->setItem(lastRow, 0, regexItem);
  ui.highlightTable->setItem(lastRow, 1, nameItem);
  ui.highlightTable->setItem(lastRow, 2, enableItem);

  QVariantMap highlightRule;
  highlightRule["regex"] = regex;
  highlightRule["name"] = name;
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
  ui.highlightTable->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, 2), selected);
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
    case 0:
      highlightRule["regex"] = (item->checkState() == Qt::Checked);
      break;
    case 1:
      if(item->text() == "") 
        item->setText(tr("this shouldn't be empty"));
      highlightRule["name"] = item->text();
      break;
    case 2:
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
    bool regex = highlightRule["regex"].toBool();
    QString name = highlightRule["name"].toString();
    bool enable = highlightRule["enable"].toBool();

    addNewRow(regex, name, enable);
  }

  ui.highlightCurrentNick->setChecked(notificationSettings.highlightCurrentNick());

  setChangedState(false);
}

void HighlightSettingsPage::save() {
  NotificationSettings notificationSettings;
  notificationSettings.setHighlightList(highlightList);
  notificationSettings.setHighlightCurrentNick(ui.highlightCurrentNick->isChecked());

  load();
  setChangedState(false);
}

void HighlightSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool HighlightSettingsPage::testHasChanged() {
  NotificationSettings notificationSettings;

  if(notificationSettings.highlightCurrentNick() != ui.highlightCurrentNick->isChecked()) return true;
  if(notificationSettings.highlightList() != highlightList) return true;

  return true;
}




