/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "sonnetsettingspage.h"

#include <QVBoxLayout>

#include "qtui.h"

SonnetSettingsPage::SonnetSettingsPage(QWidget *parent)
    : SettingsPage(tr("Interface"), tr("Spell Checking"), parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    _configWidget = new Sonnet::ConfigWidget(this);
    layout->addWidget(_configWidget);
    connect(_configWidget, SIGNAL(configChanged()), SLOT(widgetHasChanged()));
}


bool SonnetSettingsPage::hasDefaults() const
{
    return true;
}


void SonnetSettingsPage::defaults()
{
    _configWidget->slotDefault();
    widgetHasChanged();
}


void SonnetSettingsPage::load()
{
    SettingsPage::load();
}


void SonnetSettingsPage::save()
{
    _configWidget->save();
    SettingsPage::save();
}


void SonnetSettingsPage::widgetHasChanged()
{
    if (!hasChanged())
        setChangedState(true);
}
