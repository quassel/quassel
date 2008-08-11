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

#include "notificationssettingspage.h"

#include "qtui.h"
#include "uisettings.h"
#include "buffersettings.h"

NotificationsSettingsPage::NotificationsSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("Notifications"), parent) {
  ui.setupUi(this);

  connect(ui.animateTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.showBubble, SIGNAL(toggled(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.desktopBox, SIGNAL(toggled(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.timeout_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.x_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.y_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));

#ifndef HAVE_DBUS
  ui.desktopBox->setVisible(false);
#endif
}

bool NotificationsSettingsPage::hasDefaults() const {
  return true;
}

void NotificationsSettingsPage::defaults() {
  ui.animateTrayIcon->setChecked(true);
  ui.showBubble->setChecked(true);
  ui.desktopBox->setChecked(false);
  ui.timeout_value->setValue(5000);
  ui.x_value->setValue(0);
  ui.y_value->setValue(0);

  widgetHasChanged();
}

void NotificationsSettingsPage::load() {
  // uiSettings:
  UiSettings uiSettings;

  settings["AnimateTrayIcon"] = uiSettings.value("AnimateTrayIcon", QVariant(true));
  ui.animateTrayIcon->setChecked(settings["AnimateTrayIcon"].toBool());

  settings["NotificationBubble"] = uiSettings.value("NotificationBubble", QVariant(true));
  ui.showBubble->setChecked(settings["NotificationBubble"].toBool());

  settings["NotificationDesktop"] = uiSettings.value("NotificationDesktop", QVariant(false));
  ui.desktopBox->setChecked(settings["NotificationDesktop"].toBool());
  settings["NotificationDesktopTimeout"] = uiSettings.value("NotificationDesktopTimeout", QVariant(5000));
  ui.timeout_value->setValue(settings["NotificationDesktopTimeout"].toInt());
  settings["NotificationDesktopHintX"] = uiSettings.value("NotificationDesktopHintX", QVariant(0));
  ui.x_value->setValue(settings["NotificationDesktopHintX"].toInt());
  settings["NotificationDesktopHintY"] = uiSettings.value("NotificationDesktopHintY", QVariant(0));
  ui.y_value->setValue(settings["NotificationDesktopHintY"].toInt());

  setChangedState(false);
}

void NotificationsSettingsPage::save() {
  UiSettings uiSettings;

  uiSettings.setValue("AnimateTrayIcon", ui.animateTrayIcon->isChecked());

  uiSettings.setValue("NotificationBubble", ui.showBubble->isChecked());
  uiSettings.setValue("NotificationDesktop", ui.desktopBox->isChecked());
  uiSettings.setValue("NotificationDesktopTimeout", ui.timeout_value->value());
  uiSettings.setValue("NotificationDesktopHintX", ui.x_value->value());
  uiSettings.setValue("NotificationDesktopHintY", ui.y_value->value());

  load();
  setChangedState(false);
}

void NotificationsSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool NotificationsSettingsPage::testHasChanged() {
  if(settings["AnimateTrayIcon"].toBool() != ui.animateTrayIcon->isChecked()) return true;
  if(settings["NotificationBubble"].toBool() != ui.showBubble->isChecked()) return true;
  if(settings["NotificationDesktop"].toBool() != ui.desktopBox->isChecked()) return true;
  if(settings["NotificationDesktopTimeout"].toInt() != ui.timeout_value->value()) return true;
  if(settings["NotificationDesktopHintX"].toInt() != ui.x_value->value()) return true;
  if(settings["NotificationDesktopHintY"].toInt() != ui.y_value->value()) return true;

  return false;
}
