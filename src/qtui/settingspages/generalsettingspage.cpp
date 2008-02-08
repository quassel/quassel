/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "generalsettingspage.h"

#include "qtui.h"
#include "qtuisettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("General"), parent) {
  ui.setupUi(this);

  connect(ui.useSystemTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.minimizeOnMinimize, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.minimizeOnClose, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.userMessagesInStatusBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userMessagesInQueryBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userMessagesInCurrentBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
}

bool GeneralSettingsPage::hasDefaults() const {
  return true;
}

void GeneralSettingsPage::defaults() {
  qDebug() << "defaults in generalsettingspage";
  ui.useSystemTrayIcon->setChecked(true);
  ui.minimizeOnMinimize->setChecked(false);
  ui.minimizeOnClose->setChecked(false);

  ui.userMessagesInStatusBuffer->setChecked(true);
  ui.userMessagesInQueryBuffer->setChecked(false);
  ui.userMessagesInCurrentBuffer->setChecked(false);

  widgetHasChanged();
}

void GeneralSettingsPage::load() {
  QtUiSettings s;
  settings["UseSystemTrayIcon"] = s.value("UseSystemTrayIcon");
  ui.useSystemTrayIcon->setChecked(settings["UseSystemTrayIcon"].toBool());

  settings["MinimizeOnMinimize"] = s.value("MinimizeOnMinimize");
  ui.minimizeOnMinimize->setChecked(settings["MinimizeOnMinimize"].toBool());

  settings["MinimizeOnClose"] = s.value("MinimizeOnClose");
  ui.minimizeOnClose->setChecked(settings["MinimizeOnClose"].toBool());

  settings["UserMessagesInStatusBuffer"] = s.value("UserMessagesInStatusBuffer");
  ui.userMessagesInStatusBuffer->setChecked(settings["UserMessagesInStatusBuffer"].toBool());

  settings["UserMessagesInQueryBuffer"] = s.value("UserMessagesInQueryBuffer");
  ui.userMessagesInQueryBuffer->setChecked(settings["UserMessagesInQueryBuffer"].toBool());

  settings["UserMessagesInCurrentBuffer"] = s.value("UserMessagesInCurrentBuffer");
  ui.userMessagesInCurrentBuffer->setChecked(settings["UserMessagesInCurrentBuffer"].toBool());

  setChangedState(false);
}

void GeneralSettingsPage::save() {
  QtUiSettings s;
  s.setValue("UseSystemTrayIcon", ui.useSystemTrayIcon->isChecked());
  s.setValue("MinimizeOnMinimize",  ui.minimizeOnMinimize->isChecked());
  s.setValue("MinimizeOnClose", ui.minimizeOnClose->isChecked());

  s.setValue("UserMessagesInStatusBuffer", ui.userMessagesInStatusBuffer->isChecked());
  s.setValue("UserMessagesInQueryBuffer", ui.userMessagesInQueryBuffer->isChecked());
  s.setValue("UserMessagesInCurrentBuffer", ui.userMessagesInCurrentBuffer->isChecked());

  setChangedState(false);
}

void GeneralSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool GeneralSettingsPage::testHasChanged() {
  if(settings["UseSystemTrayIcon"] != ui.useSystemTrayIcon->isChecked()) return true; 
  if(settings["MinimizeOnMinimize"] != ui.minimizeOnMinimize->isChecked()) return true;
  if(settings["MinimizeOnClose"] != ui.minimizeOnClose->isChecked()) return true;
  if(settings["UserMessagesInStatusBuffer"] != ui.userMessagesInStatusBuffer->isChecked()) return true;
  if(settings["UserMessagesInQueryBuffer"] != ui.userMessagesInQueryBuffer->isChecked()) return true;
  if(settings["UserMessagesInCurrentBuffer"] != ui.userMessagesInCurrentBuffer->isChecked()) return true;

  return false;
}




