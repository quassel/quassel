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
#include "uisettings.h"
#include "buffersettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("General"), parent) {
  ui.setupUi(this);

#ifdef Q_WS_MAC
  ui.useSystemTrayIcon->hide();
#else
  ui.macOnly->hide();
#endif

#ifdef Q_WS_WIN
  ui.minimizeOnMinimize->hide();
#endif

  connect(ui.useSystemTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.showSystemTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.minimizeOnMinimize, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.minimizeOnClose, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.animateTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.bubbleBox, SIGNAL(toggled(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.desktopBox, SIGNAL(toggled(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.timeout_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.x_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.y_value, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));

  connect(ui.userMessagesInStatusBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userMessagesInQueryBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userMessagesInCurrentBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.displayTopicInTooltip, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.mouseWheelChangesBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.completionSuffix, SIGNAL(textEdited(const QString&)), this, SLOT(widgetHasChanged()));
}

bool GeneralSettingsPage::hasDefaults() const {
  return true;
}

void GeneralSettingsPage::defaults() {
  ui.useSystemTrayIcon->setChecked(true);
  ui.showSystemTrayIcon->setChecked(true);
  ui.minimizeOnMinimize->setChecked(false);
  ui.minimizeOnClose->setChecked(false);

  ui.animateTrayIcon->setChecked(true);
  ui.bubbleBox->setChecked(true);
  ui.desktopBox->setChecked(true);
  ui.timeout_value->setValue(5000);
  ui.x_value->setValue(0);
  ui.y_value->setValue(0);

  ui.userMessagesInStatusBuffer->setChecked(true);
  ui.userMessagesInQueryBuffer->setChecked(false);
  ui.userMessagesInCurrentBuffer->setChecked(false);

  ui.displayTopicInTooltip->setChecked(false);
  ui.mouseWheelChangesBuffers->setChecked(true);

  ui.completionSuffix->setText(": ");

  widgetHasChanged();
}

void GeneralSettingsPage::load() {
  // uiSettings:
  UiSettings uiSettings;
  settings["UseSystemTrayIcon"] = uiSettings.value("UseSystemTrayIcon", QVariant(true));
  ui.useSystemTrayIcon->setChecked(settings["UseSystemTrayIcon"].toBool());
  ui.showSystemTrayIcon->setChecked(settings["UseSystemTrayIcon"].toBool());

  settings["MinimizeOnMinimize"] = uiSettings.value("MinimizeOnMinimize", QVariant(false));
  ui.minimizeOnMinimize->setChecked(settings["MinimizeOnMinimize"].toBool());

  settings["MinimizeOnClose"] = uiSettings.value("MinimizeOnClose", QVariant(false));
  ui.minimizeOnClose->setChecked(settings["MinimizeOnClose"].toBool());

  settings["MouseWheelChangesBuffers"] = uiSettings.value("MouseWheelChangesBuffers", QVariant(true));
  ui.mouseWheelChangesBuffers->setChecked(settings["MouseWheelChangesBuffers"].toBool());

  settings["AnimateTrayIcon"] = uiSettings.value("AnimateTrayIcon", QVariant(true));
  ui.animateTrayIcon->setChecked(settings["AnimateTrayIcon"].toBool());

  settings["NotificationBubble"] = uiSettings.value("NotificationBubble", QVariant(true));
  ui.bubbleBox->setChecked(settings["NotificationBubble"].toBool());

  settings["NotificationDesktop"] = uiSettings.value("NotificationDesktop", QVariant(true));
  ui.desktopBox->setChecked(settings["NotificationDesktop"].toBool());
  settings["NotificationDesktopTimeout"] = uiSettings.value("NotificationDesktopTimeout", QVariant(5000));
  ui.timeout_value->setValue(settings["NotificationDesktopTimeout"].toInt());
  settings["NotificationDesktopHintX"] = uiSettings.value("NotificationDesktopHintX", QVariant(0));
  ui.x_value->setValue(settings["NotificationDesktopHintX"].toInt());
  settings["NotificationDesktopHintY"] = uiSettings.value("NotificationDesktopHintY", QVariant(0));
  ui.y_value->setValue(settings["NotificationDesktopHintY"].toInt());

  // bufferSettings:
  BufferSettings bufferSettings;
  settings["UserMessagesInStatusBuffer"] = bufferSettings.value("UserMessagesInStatusBuffer", QVariant(true));
  ui.userMessagesInStatusBuffer->setChecked(settings["UserMessagesInStatusBuffer"].toBool());

  settings["UserMessagesInQueryBuffer"] = bufferSettings.value("UserMessagesInQueryBuffer", QVariant(false));
  ui.userMessagesInQueryBuffer->setChecked(settings["UserMessagesInQueryBuffer"].toBool());

  settings["UserMessagesInCurrentBuffer"] = bufferSettings.value("UserMessagesInCurrentBuffer", QVariant(false));
  ui.userMessagesInCurrentBuffer->setChecked(settings["UserMessagesInCurrentBuffer"].toBool());

  settings["DisplayTopicInTooltip"] = bufferSettings.value("DisplayTopicInTooltip", QVariant(false));
  ui.displayTopicInTooltip->setChecked(settings["DisplayTopicInTooltip"].toBool());

  // inputline settings
  settings["CompletionSuffix"] = uiSettings.value("CompletionSuffix", QString(": "));
  ui.completionSuffix->setText(settings["CompletionSuffix"].toString());

  setChangedState(false);
}

void GeneralSettingsPage::save() {
  UiSettings uiSettings;
  uiSettings.setValue("UseSystemTrayIcon", ui.useSystemTrayIcon->isChecked());
  uiSettings.setValue("MinimizeOnMinimize",  ui.minimizeOnMinimize->isChecked());
  uiSettings.setValue("MinimizeOnClose", ui.minimizeOnClose->isChecked());
  uiSettings.setValue("MouseWheelChangesBuffers", ui.mouseWheelChangesBuffers->isChecked());

  uiSettings.setValue("AnimateTrayIcon", ui.animateTrayIcon->isChecked());
//<<< HEAD:src/qtui/settingspages/generalsettingspage.cpp
//  uiSettings.setValue("DisplayPopupMessages", ui.displayPopupMessages->isChecked());
//  uiSettings.setValue("CompletionSuffix", ui.completionSuffix->text());
  
//=======
  uiSettings.setValue("NotificationBubble", ui.bubbleBox->isChecked());
  uiSettings.setValue("NotificationDesktop", ui.desktopBox->isChecked());
  uiSettings.setValue("NotificationDesktopTimeout", ui.timeout_value->value());
  uiSettings.setValue("NotificationDesktopHintX", ui.x_value->value());
  uiSettings.setValue("NotificationDesktopHintY", ui.y_value->value());

//>>> Configuration support for desktop notifications.:src/qtui/settingspages/generalsettingspage.cpp
  BufferSettings bufferSettings;
  bufferSettings.setValue("UserMessagesInStatusBuffer", ui.userMessagesInStatusBuffer->isChecked());
  bufferSettings.setValue("UserMessagesInQueryBuffer", ui.userMessagesInQueryBuffer->isChecked());
  bufferSettings.setValue("UserMessagesInCurrentBuffer", ui.userMessagesInCurrentBuffer->isChecked());

  bufferSettings.setValue("DisplayTopicInTooltip", ui.displayTopicInTooltip->isChecked());

  load();
  setChangedState(false);
}

void GeneralSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool GeneralSettingsPage::testHasChanged() {
  if(settings["UseSystemTrayIcon"].toBool() != ui.useSystemTrayIcon->isChecked()) return true;
  if(settings["MinimizeOnMinimize"].toBool() != ui.minimizeOnMinimize->isChecked()) return true;
  if(settings["MinimizeOnClose"].toBool() != ui.minimizeOnClose->isChecked()) return true;

  if(settings["AnimateTrayIcon"].toBool() != ui.animateTrayIcon->isChecked()) return true;
  if(settings["NotificationBubble"].toBool() != ui.bubbleBox->isChecked()) return true;
  if(settings["NotificationDesktop"].toBool() != ui.desktopBox->isChecked()) return true;
  if(settings["NotificationDesktopTimeout"].toInt() != ui.timeout_value->value()) return true;
  if(settings["NotificationDesktopHintX"].toInt() != ui.x_value->value()) return true;
  if(settings["NotificationDesktopHintY"].toInt() != ui.y_value->value()) return true;

  if(settings["UserMessagesInStatusBuffer"].toBool() != ui.userMessagesInStatusBuffer->isChecked()) return true;
  if(settings["UserMessagesInQueryBuffer"].toBool() != ui.userMessagesInQueryBuffer->isChecked()) return true;
  if(settings["UserMessagesInCurrentBuffer"].toBool() != ui.userMessagesInCurrentBuffer->isChecked()) return true;

  if(settings["DisplayTopicInTooltip"].toBool() != ui.displayTopicInTooltip->isChecked()) return true;
  if(settings["MouseWheelChangesBuffers"].toBool() != ui.mouseWheelChangesBuffers->isChecked()) return true;

  if(settings["CompletionSuffix"].toString() != ui.completionSuffix->text()) return true;

  return false;
}




