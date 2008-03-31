/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Appearance Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Appearance Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Appearance Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "appearancesettingspage.h"

#include "qtui.h"
#include "uisettings.h"

#include <QStyleFactory>

AppearanceSettingsPage::AppearanceSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("General"), parent) {
  ui.setupUi(this);
  initStyleComboBox();

  connect(ui.styleComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(widgetHasChanged())); 
}

void AppearanceSettingsPage::initStyleComboBox() {
  QStringList styleList = QStyleFactory::keys();
  ui.styleComboBox->addItem("<default>");
  foreach(QString style, styleList) {
    ui.styleComboBox->addItem(style);
  }
}

bool AppearanceSettingsPage::hasDefaults() const {
  return true;
}

void AppearanceSettingsPage::defaults() {
  ui.styleComboBox->setCurrentIndex(0);

  widgetHasChanged();
}

void AppearanceSettingsPage::load() {
  UiSettings uiSettings;

  settings["Style"] = uiSettings.value("Style", QString(""));
  if(settings["Style"].toString() == "") {
    ui.styleComboBox->setCurrentIndex(0);
  } else {
    ui.styleComboBox->setCurrentIndex(ui.styleComboBox->findText(settings["Style"].toString(), Qt::MatchExactly));
    QApplication::setStyle(settings["Style"].toString());
  }

  setChangedState(false);
}

void AppearanceSettingsPage::save() {
  UiSettings uiSettings;

  if(ui.styleComboBox->currentIndex() < 1) {
    uiSettings.setValue("Style", QString(""));
  } else {
    uiSettings.setValue("Style", ui.styleComboBox->currentText());
  }

  load();
  setChangedState(false);
}

void AppearanceSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool AppearanceSettingsPage::testHasChanged() {
  if(settings["Style"].toString() != ui.styleComboBox->currentText()) return true;

  return false;
}




