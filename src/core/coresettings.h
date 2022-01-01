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

#pragma once

#include "settings.h"

class CoreSettings : public Settings
{
public:
    CoreSettings(QString group = "Core");

    void setStorageSettings(const QVariant& data);
    QVariant storageSettings(const QVariant& def = {}) const;

    void setAuthSettings(const QVariant& data);
    QVariant authSettings(const QVariant& def = {}) const;

    QVariant oldDbSettings() const;  // FIXME remove

    void setCoreState(const QVariant& data);
    QVariant coreState(const QVariant& def = {}) const;
};
