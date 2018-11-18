/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "coreconnectionsettingspage.h"

CoreConnectionSettingsPage::CoreConnectionSettingsPage(QWidget* parent)
    : SettingsPage(tr("Remote Cores"), tr("Connection"), parent)
{
    ui.setupUi(this);

    initAutoWidgets();

    connect(ui.useQNetworkConfigurationManager, &QAbstractButton::toggled, this, &CoreConnectionSettingsPage::widgetHasChanged);
    connect(ui.usePingTimeout, &QAbstractButton::toggled, this, &CoreConnectionSettingsPage::widgetHasChanged);
    connect(ui.useNoTimeout, &QAbstractButton::toggled, this, &CoreConnectionSettingsPage::widgetHasChanged);
}

void CoreConnectionSettingsPage::widgetHasChanged()
{
    bool hasChanged = false;
    CoreConnectionSettings::NetworkDetectionMode mode = modeFromRadioButtons();
    if (mode != _detectionMode)
        hasChanged = true;

    setChangedState(hasChanged);
}

void CoreConnectionSettingsPage::defaults()
{
    setRadioButtons(CoreConnectionSettings::UseQNetworkConfigurationManager);

    SettingsPage::defaults();
}

void CoreConnectionSettingsPage::load()
{
    CoreConnectionSettings s;
    _detectionMode = s.networkDetectionMode();
    setRadioButtons(_detectionMode);
    SettingsPage::load();
}

void CoreConnectionSettingsPage::save()
{
    _detectionMode = modeFromRadioButtons();
    CoreConnectionSettings s;
    s.setNetworkDetectionMode(_detectionMode);
    SettingsPage::save();
}

void CoreConnectionSettingsPage::setRadioButtons(CoreConnectionSettings::NetworkDetectionMode mode)
{
    switch (mode) {
    case CoreConnectionSettings::UseQNetworkConfigurationManager:
        ui.useQNetworkConfigurationManager->setChecked(true);
        break;
    case CoreConnectionSettings::UsePingTimeout:
        ui.usePingTimeout->setChecked(true);
        break;
    default:
        ui.useNoTimeout->setChecked(true);
    }
}

CoreConnectionSettings::NetworkDetectionMode CoreConnectionSettingsPage::modeFromRadioButtons() const
{
    if (ui.useQNetworkConfigurationManager->isChecked())
        return CoreConnectionSettings::UseQNetworkConfigurationManager;
    if (ui.usePingTimeout->isChecked())
        return CoreConnectionSettings::UsePingTimeout;

    return CoreConnectionSettings::NoActiveDetection;
}
