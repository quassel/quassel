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

#pragma once

#include "uisupport-export.h"

#include <vector>

#include <QIcon>
#include <QString>

namespace icon {

/**
 * Gets an icon from the current icon theme.
 *
 * If the theme does not provide the icon, tries to load the icon from the
 * fallback path, if given.
 *
 * If no icon can be found, a warning is displayed and a null icon returned.
 *
 * @param iconName     Icon name
 * @param fallbackPath Full path to a fallback icon
 * @returns The requested icon, if available
 */
UISUPPORT_EXPORT QIcon get(const QString& iconName, const QString& fallbackPath = {});

/**
 * Gets an icon from the current icon theme.
 *
 * If the theme does not provide any of the given icon names, tries to load the
 * icon from the fallback path, if given.
 *
 * If no icon can be found, a warning is displayed and a null icon returned.
 *
 * @param iconNames    List of icon names (first match wins)
 * @param fallbackPath Full path to a fallback icon
 * @returns The requested icon, if available
 */
UISUPPORT_EXPORT QIcon get(const std::vector<QString>& iconNames, const QString& fallbackPath = {});

}  // namespace icon
