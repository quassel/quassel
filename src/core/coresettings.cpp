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

#include "coresettings.h"

#include "quassel.h"

CoreSettings::CoreSettings(QString group)
    : Settings(std::move(group), Quassel::buildInfo().coreApplicationName)
{}

void CoreSettings::setStorageSettings(const QVariant& data)
{
    setLocalValue("StorageSettings", data);
}

QVariant CoreSettings::storageSettings(const QVariant& def) const
{
    return localValue("StorageSettings", def);
}

void CoreSettings::setAuthSettings(const QVariant& data)
{
    setLocalValue("AuthSettings", data);
}

QVariant CoreSettings::authSettings(const QVariant& def) const
{
    return localValue("AuthSettings", def);
}

// FIXME remove
QVariant CoreSettings::oldDbSettings() const
{
    return localValue("DatabaseSettings");
}

void CoreSettings::setCoreState(const QVariant& data)
{
    setLocalValue("CoreState", data);
}

QVariant CoreSettings::coreState(const QVariant& def) const
{
    return localValue("CoreState", def);
}
