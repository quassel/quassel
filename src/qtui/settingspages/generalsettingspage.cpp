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
#include "backlogsettings.h"
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

  connect(ui.userNoticesInDefaultBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userNoticesInStatusBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.userNoticesInCurrentBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.serverNoticesInDefaultBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.serverNoticesInStatusBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.serverNoticesInCurrentBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.errorMsgsInDefaultBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.errorMsgsInStatusBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.errorMsgsInCurrentBuffer, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));

  connect(ui.displayTopicInTooltip, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.mouseWheelChangesBuffers, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.completionSuffix, SIGNAL(textEdited(const QString&)), this, SLOT(widgetHasChanged()));
  connect(ui.fixedBacklogAmount, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.dynamicBacklogAmount, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
}

bool GeneralSettingsPage::hasDefaults() const {
  return true;
}

void GeneralSettingsPage::defaults() {
  ui.useSystemTrayIcon->setChecked(true);
  ui.showSystemTrayIcon->setChecked(true);
  ui.minimizeOnMinimize->setChecked(false);
  ui.minimizeOnClose->setChecked(false);

  ui.userNoticesInDefaultBuffer->setChecked(true);
  ui.userNoticesInStatusBuffer->setChecked(false);
  ui.userNoticesInCurrentBuffer->setChecked(false);

  ui.serverNoticesInDefaultBuffer->setChecked(false);
  ui.serverNoticesInStatusBuffer->setChecked(true);
  ui.serverNoticesInCurrentBuffer->setChecked(false);

  ui.errorMsgsInDefaultBuffer->setChecked(true);
  ui.errorMsgsInStatusBuffer->setChecked(false);
  ui.errorMsgsInCurrentBuffer->setChecked(false);

  ui.displayTopicInTooltip->setChecked(false);
  ui.mouseWheelChangesBuffers->setChecked(false);

  ui.completionSuffix->setText(": ");

  widgetHasChanged();
}

void GeneralSettingsPage::load() {
  // uiSettings:
  QtUiSettings qtuiSettings;
  UiSettings uiSettings;
  settings["UseSystemTrayIcon"] = qtuiSettings.value("UseSystemTrayIcon", QVariant(true));
  ui.useSystemTrayIcon->setChecked(settings["UseSystemTrayIcon"].toBool());
  ui.showSystemTrayIcon->setChecked(settings["UseSystemTrayIcon"].toBool());

  settings["MinimizeOnMinimize"] = qtuiSettings.value("MinimizeOnMinimize", QVariant(false));
  ui.minimizeOnMinimize->setChecked(settings["MinimizeOnMinimize"].toBool());

  settings["MinimizeOnClose"] = qtuiSettings.value("MinimizeOnClose", QVariant(false));
  ui.minimizeOnClose->setChecked(settings["MinimizeOnClose"].toBool());

  settings["MouseWheelChangesBuffers"] = uiSettings.value("MouseWheelChangesBuffers", QVariant(false));
  ui.mouseWheelChangesBuffers->setChecked(settings["MouseWheelChangesBuffers"].toBool());

  // bufferSettings:
  BufferSettings bufferSettings;
  SettingsPage::load(ui.userNoticesInDefaultBuffer, bufferSettings.value("UserNoticesInDefaultBuffer", QVariant(true)).toBool());
  SettingsPage::load(ui.userNoticesInStatusBuffer, bufferSettings.value("UserNoticesInStatusBuffer", QVariant(false)).toBool());
  SettingsPage::load(ui.userNoticesInCurrentBuffer, bufferSettings.value("UserNoticesInCurrentBuffer", QVariant(false)).toBool());

  SettingsPage::load(ui.serverNoticesInDefaultBuffer, bufferSettings.value("ServerNoticesInDefaultBuffer", QVariant(false)).toBool());
  SettingsPage::load(ui.serverNoticesInStatusBuffer, bufferSettings.value("ServerNoticesInStatusBuffer", QVariant(true)).toBool());
  SettingsPage::load(ui.serverNoticesInCurrentBuffer, bufferSettings.value("ServerNoticesInCurrentBuffer", QVariant(false)).toBool());

  SettingsPage::load(ui.errorMsgsInDefaultBuffer, bufferSettings.value("ErrorMsgsInDefaultBuffer", QVariant(true)).toBool());
  SettingsPage::load(ui.errorMsgsInStatusBuffer, bufferSettings.value("ErrorMsgsInStatusBuffer", QVariant(false)).toBool());
  SettingsPage::load(ui.errorMsgsInCurrentBuffer, bufferSettings.value("ErrorMsgsInCurrentBuffer", QVariant(false)).toBool());


  settings["DisplayTopicInTooltip"] = bufferSettings.value("DisplayTopicInTooltip", QVariant(false));
  ui.displayTopicInTooltip->setChecked(settings["DisplayTopicInTooltip"].toBool());

  // inputline settings
  settings["CompletionSuffix"] = uiSettings.value("CompletionSuffix", QString(": "));
  ui.completionSuffix->setText(settings["CompletionSuffix"].toString());

  // backlogSettings:
  BacklogSettings backlogSettings;
  settings["FixedBacklogAmount"] = backlogSettings.fixedBacklogAmount();
  ui.fixedBacklogAmount->setValue(backlogSettings.fixedBacklogAmount());

  settings["DynamicBacklogAmount"] = backlogSettings.dynamicBacklogAmount();
  ui.dynamicBacklogAmount->setValue(backlogSettings.dynamicBacklogAmount());

  setChangedState(false);
}

void GeneralSettingsPage::save() {
  QtUiSettings qtuiSettings;
  qtuiSettings.setValue("UseSystemTrayIcon", ui.useSystemTrayIcon->isChecked());
  qtuiSettings.setValue("MinimizeOnMinimize",  ui.minimizeOnMinimize->isChecked());
  qtuiSettings.setValue("MinimizeOnClose", ui.minimizeOnClose->isChecked());

  UiSettings uiSettings;
  uiSettings.setValue("MouseWheelChangesBuffers", ui.mouseWheelChangesBuffers->isChecked());

  BufferSettings bufferSettings;
  bufferSettings.setValue("UserNoticesInDefaultBuffer", ui.userNoticesInDefaultBuffer->isChecked());
  bufferSettings.setValue("UserNoticesInStatusBuffer", ui.userNoticesInStatusBuffer->isChecked());
  bufferSettings.setValue("UserNoticesInCurrentBuffer", ui.userNoticesInCurrentBuffer->isChecked());

  bufferSettings.setValue("ServerNoticesInDefaultBuffer", ui.serverNoticesInDefaultBuffer->isChecked());
  bufferSettings.setValue("ServerNoticesInStatusBuffer", ui.serverNoticesInStatusBuffer->isChecked());
  bufferSettings.setValue("ServerNoticesInCurrentBuffer", ui.serverNoticesInCurrentBuffer->isChecked());

  bufferSettings.setValue("ErrorMsgsInDefaultBuffer", ui.errorMsgsInDefaultBuffer->isChecked());
  bufferSettings.setValue("ErrorMsgsInStatusBuffer", ui.errorMsgsInStatusBuffer->isChecked());
  bufferSettings.setValue("ErrorMsgsInCurrentBuffer", ui.errorMsgsInCurrentBuffer->isChecked());

  bufferSettings.setValue("DisplayTopicInTooltip", ui.displayTopicInTooltip->isChecked());

  uiSettings.setValue("CompletionSuffix", ui.completionSuffix->text());


  BacklogSettings backlogSettings;
  backlogSettings.setFixedBacklogAmount(ui.fixedBacklogAmount->value());
  backlogSettings.setDynamicBacklogAmount(ui.dynamicBacklogAmount->value());

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

  if(SettingsPage::hasChanged(ui.userNoticesInStatusBuffer)) return true;
  if(SettingsPage::hasChanged(ui.userNoticesInDefaultBuffer)) return true;
  if(SettingsPage::hasChanged(ui.userNoticesInCurrentBuffer)) return true;

  if(SettingsPage::hasChanged(ui.serverNoticesInStatusBuffer)) return true;
  if(SettingsPage::hasChanged(ui.serverNoticesInDefaultBuffer)) return true;
  if(SettingsPage::hasChanged(ui.serverNoticesInCurrentBuffer)) return true;

  if(SettingsPage::hasChanged(ui.errorMsgsInStatusBuffer)) return true;
  if(SettingsPage::hasChanged(ui.errorMsgsInDefaultBuffer)) return true;
  if(SettingsPage::hasChanged(ui.errorMsgsInCurrentBuffer)) return true;

  if(settings["DisplayTopicInTooltip"].toBool() != ui.displayTopicInTooltip->isChecked()) return true;
  if(settings["MouseWheelChangesBuffers"].toBool() != ui.mouseWheelChangesBuffers->isChecked()) return true;

  if(settings["CompletionSuffix"].toString() != ui.completionSuffix->text()) return true;

  if(settings["FixedBacklogAmount"].toInt() != ui.fixedBacklogAmount->value()) return true;
  if(settings["DynamicBacklogAmount"].toInt() != ui.dynamicBacklogAmount->value()) return true;

  return false;
}
