/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Blank Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Blank Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Blank Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "blanksettingspage.h"

#include "qtui.h"
#include "uisettings.h"


BlankSettingsPage::BlankSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("Blank"), parent) {
  ui.setupUi(this);

  connect(ui.exampleCheckbox, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
}

bool BlankSettingsPage::hasDefaults() const {
  return true;
}

void BlankSettingsPage::defaults() {
  ui.exampleCheckbox->setChecked(true);

  widgetHasChanged();
}

void BlankSettingsPage::load() {
  UiSettings uiSettings;

  settings["exampleCheckbox"] = uiSettings.value("exampleCheckbox", QVariant(false));
  ui.exampleCheckbox->setChecked(settings["exampleCheckbox"].toBool());

  setChangedState(false);
}

void BlankSettingsPage::save() {
  UiSettings uiSettings;
  uiSettings.setValue("exampleCheckbox", ui.exampleCheckbox->isChecked());

  load();
  setChangedState(false);
}

void BlankSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool BlankSettingsPage::testHasChanged() {
  if(settings["exampleCheckbox"].toBool() != ui.exampleCheckbox->isChecked()) return true;

  return false;
}




