/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "backlogsettings.h"

BacklogSettings::BacklogSettings()
    : ClientSettings("Backlog")
{}

int BacklogSettings::requesterType() const
{
    int _requesterType = localValue("RequesterType", BacklogRequester::AsNeeded).toInt();
    if (_requesterType == BacklogRequester::GlobalUnread) {
        // GlobalUnread is currently disabled; don't allow it to be used.  Reset to default instead.
        _requesterType = BacklogRequester::AsNeeded;
    }
    return _requesterType;
}

void BacklogSettings::setRequesterType(int requesterType)
{
    // This settings key is also used within ChatMonitorSettingsPage::ChatMonitorSettingsPage()
    setLocalValue("RequesterType", requesterType);
}

int BacklogSettings::dynamicBacklogAmount() const
{
    return localValue("DynamicBacklogAmount", 200).toInt();
}

void BacklogSettings::setDynamicBacklogAmount(int amount)
{
    return setLocalValue("DynamicBacklogAmount", amount);
}

bool BacklogSettings::ensureBacklogOnBufferShow() const
{
    // This settings key is also used within BufferWidget::BufferWidget()
    return localValue("EnsureBacklogOnBufferShow", true).toBool();
}

void BacklogSettings::setEnsureBacklogOnBufferShow(bool enabled)
{
    return setLocalValue("EnsureBacklogOnBufferShow", enabled);
}

int BacklogSettings::fixedBacklogAmount() const
{
    return localValue("FixedBacklogAmount", 500).toInt();
}

void BacklogSettings::setFixedBacklogAmount(int amount)
{
    return setLocalValue("FixedBacklogAmount", amount);
}

int BacklogSettings::globalUnreadBacklogLimit() const
{
    return localValue("GlobalUnreadBacklogLimit", 5000).toInt();
}

void BacklogSettings::setGlobalUnreadBacklogLimit(int limit)
{
    return setLocalValue("GlobalUnreadBacklogLimit", limit);
}

int BacklogSettings::globalUnreadBacklogAdditional() const
{
    return localValue("GlobalUnreadBacklogAdditional", 100).toInt();
}

void BacklogSettings::setGlobalUnreadBacklogAdditional(int additional)
{
    return setLocalValue("GlobalUnreadBacklogAdditional", additional);
}

int BacklogSettings::perBufferUnreadBacklogLimit() const
{
    return localValue("PerBufferUnreadBacklogLimit", 200).toInt();
}

void BacklogSettings::setPerBufferUnreadBacklogLimit(int limit)
{
    return setLocalValue("PerBufferUnreadBacklogLimit", limit);
}

int BacklogSettings::perBufferUnreadBacklogAdditional() const
{
    return localValue("PerBufferUnreadBacklogAdditional", 50).toInt();
}

void BacklogSettings::setPerBufferUnreadBacklogAdditional(int additional)
{
    return setLocalValue("PerBufferUnreadBacklogAdditional", additional);
}

int BacklogSettings::asNeededLegacyBacklogAmount() const
{
    // Mimic FixedBacklogAmount defaults.  This is only used on cores lacking
    // Feature::BufferActivitySync.
    return localValue("AsNeededLegacyBacklogAmount", 500).toInt();
}

void BacklogSettings::setAsNeededLegacyBacklogAmount(int amount)
{
    return setLocalValue("AsNeededLegacyBacklogAmount", amount);
}
