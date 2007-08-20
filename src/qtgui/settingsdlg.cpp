/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

  ui.settingsFrame->setWidgetResizable(true);
  ui.settingsFrame->setWidget(ui.settingsStack);
  ui.settingsTree->setRootIsDecorated(false);

  connect(ui.settingsTree, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelected()));
  connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
}

void SettingsDlg::registerSettingsPage(SettingsPage *sp) {
  QWidget *w = sp->widget();
  w->setParent(this);
  ui.settingsStack->addWidget(w);

  QTreeWidgetItem *cat;
  QList<QTreeWidgetItem *> cats = ui.settingsTree->findItems(sp->category(), Qt::MatchExactly);
  if(!cats.count()) {
    cat = new QTreeWidgetItem(ui.settingsTree, QStringList(sp->category()));
    cat->setExpanded(true);
    cat->setFlags(Qt::ItemIsEnabled);
  } else cat = cats[0];
  QTreeWidgetItem *p = new QTreeWidgetItem(cat, QStringList(sp->title()));
  pages[QString("%1$%2").arg(sp->category()).arg(sp->title())] = sp;
}

void SettingsDlg::selectPage(const QString &cat, const QString &title) {
  QWidget *w = pages[QString("%1$%2").arg(cat).arg(title)]->widget();
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
  }
}

void SettingsDlg::buttonClicked(QAbstractButton *button) {
  switch(ui.buttonBox->buttonRole(button)) {
    case QDialogButtonBox::AcceptRole:
      applyChanges();
      accept();
      break;
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      break;
  }
}

void SettingsDlg::applyChanges() {
  //SettingsInterface *sp = qobject_cast<SettingsInterface *>(ui.settingsStack->currentWidget());
  //if(sp) sp->applyChanges();
}
