/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "settingspage.h"

#include <QCheckBox>
#include <QVariant>

SettingsPage::SettingsPage(const QString &category, const QString &title, QWidget *parent)
  : QWidget(parent),
    _category(category),
    _title(title),
    _changed(false)
{
}

void SettingsPage::setChangedState(bool hasChanged) {
  if(hasChanged != _changed) {
    _changed = hasChanged;
    emit changed(hasChanged);
  }
}

void SettingsPage::load(QCheckBox *box, bool checked) {
  box->setProperty("StoredValue", checked);
  box->setChecked(checked);
}

bool SettingsPage::hasChanged(QCheckBox *box) {
  return box->property("StoredValue").toBool() == box->isChecked();
}
