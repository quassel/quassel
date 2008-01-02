/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "settingsdlg.h"

SettingsDlg::SettingsDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);

  //ui.settingsFrame->setWidgetResizable(true);
  //ui.settingsFrame->setWidget(ui.settingsStack);
  ui.settingsTree->setRootIsDecorated(false);

  connect(ui.settingsTree, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelected()));
  connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
}


void SettingsDlg::registerSettingsPage(SettingsPage *sp) {
  sp->setParent(ui.settingsStack);
  ui.settingsStack->addWidget(sp);

  QTreeWidgetItem *cat;
  QList<QTreeWidgetItem *> cats = ui.settingsTree->findItems(sp->category(), Qt::MatchExactly);
  if(!cats.count()) {
    cat = new QTreeWidgetItem(ui.settingsTree, QStringList(sp->category()));
    cat->setExpanded(true);
    cat->setFlags(Qt::ItemIsEnabled);
  } else cat = cats[0];
  new QTreeWidgetItem(cat, QStringList(sp->title()));
  pages[QString("%1$%2").arg(sp->category(), sp->title())] = sp;
  updateGeometry();
  // TESTING
  selectPage(sp->category(), sp->title());
}

void SettingsDlg::selectPage(const QString &cat, const QString &title) {
  QWidget *w = pages[QString("%1$%2").arg(cat, title)];
  Q_ASSERT(w);
  ui.settingsStack->setCurrentWidget(w);
}

void SettingsDlg::itemSelected() {
  QList<QTreeWidgetItem *> items = ui.settingsTree->selectedItems();
  if(!items.count()) {
    return;
  } else {
    QTreeWidgetItem *parent = items[0]->parent();
    if(!parent) return;
    QString cat = parent->text(0);
    QString title = items[0]->text(0);
    selectPage(cat, title);
    ui.pageTitle->setText(title);
  }
}

void SettingsDlg::buttonClicked(QAbstractButton *button) {
  switch(ui.buttonBox->standardButton(button)) {
    case QDialogButtonBox::Ok:
      applyChanges();
      accept();
      break;
    case QDialogButtonBox::Apply:
      applyChanges();
      break;
    case QDialogButtonBox::Cancel:
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

void SettingsDlg::applyChanges() {
  foreach(SettingsPage *page, pages.values()) {
    page->save();
  }
}

void SettingsDlg::reload() {
  SettingsPage *page = qobject_cast<SettingsPage *>(ui.settingsStack->currentWidget());
  if(page) page->load();
}

void SettingsDlg::loadDefaults() {
  SettingsPage *page = qobject_cast<SettingsPage *>(ui.settingsStack->currentWidget());
  if(page) page->defaults();
}
