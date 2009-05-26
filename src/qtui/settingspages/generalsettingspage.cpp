/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include "buffersettings.h"

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent)
  : SettingsPage(tr("Misc"), QString(), parent) {
  ui.setupUi(this);

#ifdef Q_WS_MAC
  ui.useSystemTrayIcon->hide();
#else
  ui.macOnly->hide();
#endif

  connect(ui.useSystemTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
  connect(ui.showSystemTrayIcon, SIGNAL(clicked(bool)), this, SLOT(widgetHasChanged()));
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
}

bool GeneralSettingsPage::hasDefaults() const {
  return true;
}

void GeneralSettingsPage::defaults() {
  ui.useSystemTrayIcon->setChecked(true);
  ui.showSystemTrayIcon->setChecked(true);
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

  settings["MinimizeOnClose"] = qtuiSettings.value("MinimizeOnClose", QVariant(false));
  ui.minimizeOnClose->setChecked(settings["MinimizeOnClose"].toBool());

  settings["MouseWheelChangesBuffers"] = uiSettings.value("MouseWheelChangesBuffers", QVariant(false));
  ui.mouseWheelChangesBuffers->setChecked(settings["MouseWheelChangesBuffers"].toBool());

  // bufferSettings:
  BufferSettings bufferSettings;
  int redirectTarget = bufferSettings.userNoticesTarget();
  SettingsPage::load(ui.userNoticesInDefaultBuffer, redirectTarget & BufferSettings::DefaultBuffer);
  SettingsPage::load(ui.userNoticesInStatusBuffer, redirectTarget & BufferSettings::StatusBuffer);
  SettingsPage::load(ui.userNoticesInCurrentBuffer, redirectTarget & BufferSettings::CurrentBuffer);

  redirectTarget = bufferSettings.serverNoticesTarget();
  SettingsPage::load(ui.serverNoticesInDefaultBuffer, redirectTarget & BufferSettings::DefaultBuffer);
  SettingsPage::load(ui.serverNoticesInStatusBuffer, redirectTarget & BufferSettings::StatusBuffer);
  SettingsPage::load(ui.serverNoticesInCurrentBuffer, redirectTarget & BufferSettings::CurrentBuffer);

  redirectTarget = bufferSettings.errorMsgsTarget();
  SettingsPage::load(ui.errorMsgsInDefaultBuffer, redirectTarget & BufferSettings::DefaultBuffer);
  SettingsPage::load(ui.errorMsgsInStatusBuffer, redirectTarget & BufferSettings::StatusBuffer);
  SettingsPage::load(ui.errorMsgsInCurrentBuffer, redirectTarget & BufferSettings::CurrentBuffer);


  settings["DisplayTopicInTooltip"] = bufferSettings.value("DisplayTopicInTooltip", QVariant(false));
  ui.displayTopicInTooltip->setChecked(settings["DisplayTopicInTooltip"].toBool());

  // completion settings
  TabCompletionSettings completionSettings;
  settings["CompletionSuffix"] = completionSettings.completionSuffix();
  ui.completionSuffix->setText(settings["CompletionSuffix"].toString());

  setChangedState(false);
}

void GeneralSettingsPage::save() {
  QtUiSettings qtuiSettings;
#ifdef Q_WS_MAC
  qtuiSettings.setValue("UseSystemTrayIcon", ui.showSystemTrayIcon->isChecked());
#else
  qtuiSettings.setValue("UseSystemTrayIcon", ui.useSystemTrayIcon->isChecked());
#endif
  qtuiSettings.setValue("MinimizeOnClose", ui.minimizeOnClose->isChecked());

  UiSettings uiSettings;
  uiSettings.setValue("MouseWheelChangesBuffers", ui.mouseWheelChangesBuffers->isChecked());

  BufferSettings bufferSettings;
  int redirectTarget = 0;
  if(ui.userNoticesInDefaultBuffer->isChecked())
    redirectTarget |= BufferSettings::DefaultBuffer;
  if(ui.userNoticesInStatusBuffer->isChecked())
    redirectTarget |= BufferSettings::StatusBuffer;
  if(ui.userNoticesInCurrentBuffer->isChecked())
    redirectTarget |= BufferSettings::CurrentBuffer;
  bufferSettings.setUserNoticesTarget(redirectTarget);

  redirectTarget = 0;
  if(ui.serverNoticesInDefaultBuffer->isChecked())
    redirectTarget |= BufferSettings::DefaultBuffer;
  if(ui.serverNoticesInStatusBuffer->isChecked())
    redirectTarget |= BufferSettings::StatusBuffer;
  if(ui.serverNoticesInCurrentBuffer->isChecked())
    redirectTarget |= BufferSettings::CurrentBuffer;
  bufferSettings.setServerNoticesTarget(redirectTarget);

  redirectTarget = 0;
  if(ui.errorMsgsInDefaultBuffer->isChecked())
    redirectTarget |= BufferSettings::DefaultBuffer;
  if(ui.errorMsgsInStatusBuffer->isChecked())
    redirectTarget |= BufferSettings::StatusBuffer;
  if(ui.errorMsgsInCurrentBuffer->isChecked())
    redirectTarget |= BufferSettings::CurrentBuffer;
  bufferSettings.setErrorMsgsTarget(redirectTarget);

  bufferSettings.setValue("DisplayTopicInTooltip", ui.displayTopicInTooltip->isChecked());

  TabCompletionSettings completionSettings;
  completionSettings.setCompletionSuffix(ui.completionSuffix->text());


  load();
  setChangedState(false);
}

void GeneralSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool GeneralSettingsPage::testHasChanged() {
#ifdef Q_WS_MAC
  if(settings["UseSystemTrayIcon"].toBool() != ui.showSystemTrayIcon->isChecked()) return true;
#else
  if(settings["UseSystemTrayIcon"].toBool() != ui.useSystemTrayIcon->isChecked()) return true;
#endif
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

  return false;
}
