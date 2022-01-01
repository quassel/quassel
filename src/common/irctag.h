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

#include "common-export.h"

#include <ostream>
#include <utility>

#include <QDebug>
#include <QString>

struct COMMON_EXPORT IrcTagKey
{
    QString vendor;
    QString key;
    bool clientTag;

    explicit IrcTagKey(QString vendor, QString key, bool clientTag = false) :
        vendor(std::move(vendor)), key(std::move(key)), clientTag(clientTag)
    {}

    explicit IrcTagKey(QString key = {}) :
        vendor(QString{}), key(std::move(key)), clientTag(false)
    {}

    friend COMMON_EXPORT uint qHash(const IrcTagKey& key);
    friend COMMON_EXPORT bool operator==(const IrcTagKey& a, const IrcTagKey& b);
    friend COMMON_EXPORT bool operator<(const IrcTagKey& a, const IrcTagKey& b);
    friend COMMON_EXPORT QDebug operator<<(QDebug dbg, const IrcTagKey& i);
    friend COMMON_EXPORT std::ostream& operator<<(std::ostream& o, const IrcTagKey& i);
};
