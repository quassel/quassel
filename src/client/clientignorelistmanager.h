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

#pragma once

#include "client-export.h"

#include "ignorelistmanager.h"
#include <QMap>

class CLIENT_EXPORT ClientIgnoreListManager : public IgnoreListManager
{
    Q_OBJECT

public:
    explicit ClientIgnoreListManager(QObject *parent = nullptr);

    //! Fetch all matching ignore rules for a given hostmask
    /** \param hostmask The hostmask of the user
      * \param network The network name
      * \param channel The channel name
      * \return Returns a QMap with the rule as key and a bool, representing if the rule is enabled or not, as value
      */
    QMap<QString, bool> matchingRulesForHostmask(const QString &hostmask, const QString &network, const QString &channel) const;

signals:
    void ignoreListChanged();

private:
    // matches an ignore rule against a given string
    bool pureMatch(const IgnoreListItem &item, const QString &string) const;
};
