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

#include "backlogsettingspage.h"

#include "qtui.h"
#include "backlogsettings.h"

BacklogSettingsPage::BacklogSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("Backlog"), parent)
{
  ui.setupUi(this);
  connect(ui.requesterType, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetHasChanged()));

  connect(ui.fixedBacklogAmount, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.globalUnreadLimit, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.globalUnreadAdditional, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.perBufferUnreadLimit, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
  connect(ui.perBufferUnreadAdditional, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));

  connect(ui.dynamicBacklogAmount, SIGNAL(valueChanged(int)), this, SLOT(widgetHasChanged()));
}

bool BacklogSettingsPage::hasDefaults() const {
  return true;
}

void BacklogSettingsPage::defaults() {
  //  ui.completionSuffix->setText(": ");

  widgetHasChanged();
}

void BacklogSettingsPage::load() {
  BacklogSettings backlogSettings;
  SettingsPage::load(ui.requesterType, backlogSettings.requesterType() - 1);
  
  SettingsPage::load(ui.fixedBacklogAmount, backlogSettings.fixedBacklogAmount());
  SettingsPage::load(ui.globalUnreadLimit, backlogSettings.globalUnreadBacklogLimit());
  SettingsPage::load(ui.globalUnreadAdditional, backlogSettings.globalUnreadBacklogAdditional());
  SettingsPage::load(ui.perBufferUnreadLimit, backlogSettings.perBufferUnreadBacklogLimit());
  SettingsPage::load(ui.perBufferUnreadAdditional, backlogSettings.perBufferUnreadBacklogAdditional());
  
  SettingsPage::load(ui.dynamicBacklogAmount, backlogSettings.dynamicBacklogAmount());

  setChangedState(false);
}

void BacklogSettingsPage::save() {
  BacklogSettings backlogSettings;
  backlogSettings.setRequesterType(ui.requesterType->currentIndex() + 1);
  backlogSettings.setFixedBacklogAmount(ui.fixedBacklogAmount->value());
  backlogSettings.setGlobalUnreadBacklogLimit(ui.globalUnreadLimit->value());
  backlogSettings.setGlobalUnreadBacklogAdditional(ui.globalUnreadAdditional->value());
  backlogSettings.setPerBufferUnreadBacklogLimit(ui.perBufferUnreadLimit->value());
  backlogSettings.setPerBufferUnreadBacklogAdditional(ui.perBufferUnreadAdditional->value());
  
  backlogSettings.setDynamicBacklogAmount(ui.dynamicBacklogAmount->value());

  load();
  setChangedState(false);
}

void BacklogSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool BacklogSettingsPage::testHasChanged() {
  if(SettingsPage::hasChanged(ui.fixedBacklogAmount)) return true;
  if(SettingsPage::hasChanged(ui.dynamicBacklogAmount)) return true;

  return false;
}
