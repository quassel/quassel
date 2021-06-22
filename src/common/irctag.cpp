/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "irctag.h"

uint qHash(const IrcTagKey& key)
{
    QString clientTag;
    if (key.clientTag) {
        clientTag = "+";
    }
    return qHash(QString(clientTag + key.vendor + "/" + key.key));
}

bool operator==(const IrcTagKey& a, const IrcTagKey& b)
{
    return a.vendor == b.vendor && a.key == b.key && a.clientTag == b.clientTag;
}

bool operator<(const IrcTagKey& a, const IrcTagKey& b)
{
    if (a.vendor == b.vendor) {
        if (a.key == b.key) {
            return a.clientTag < b.clientTag;
        } else {
            return a.key < b.key;
        }
    } else {
        return a.vendor < b.vendor;
    }
}

QDebug operator<<(QDebug dbg, const IrcTagKey& i) {
    if (i.vendor.isEmpty()) {
        dbg.noquote() << QString("%1%2")
            .arg(i.clientTag ? "+" : "", i.key);
        return dbg;
    } else {
        dbg.noquote() << QString("%1%2/%3")
            .arg(i.clientTag ? "+" : "", i.vendor, i.key);
        return dbg;
    }
}

std::ostream& operator<<(std::ostream& o, const IrcTagKey& i) {
    std::string result;
    if (i.clientTag)
        result += "+";
    if (!i.vendor.isEmpty()) {
        result += i.vendor.toStdString();
        result += "/";
    }
    result += i.key.toStdString();
    return o << result;
}
