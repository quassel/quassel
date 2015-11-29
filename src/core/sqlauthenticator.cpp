/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include "sqlauthenticator.h"

#include "network.h"
#include "quassel.h"

#include "core.h"

SqlAuthenticator::SqlAuthenticator(QObject *parent)
    : SqlAuthenticator(parent)
{
}


SqlAuthenticator::~SqlAuthenticator()
{
}

bool SqlAuthenticator::isAvailable() const
{
    // XXX: probably this should query the current storage.
    return true;
}

QString SqlAuthenticator::displayName() const
{
    // We identify the backend to use for the monolithic core by its displayname.
    // so only change this string if you _really_ have to and make sure the core
    // setup for the mono client still works ;)
    return QString("Database");
}

QString SqlAuthenticator::description() const
{
    return tr("Do not auth against any remote authentication service, but instead save a hashed and salted password "
              "in the selected database.");
}

UserId SqliteStorage::validateUser(const QString &user, const QString &password)
{
    return Core::validateUser(user, password);
}