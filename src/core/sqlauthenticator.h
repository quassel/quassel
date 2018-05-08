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

#include "authenticator.h"

class SqlAuthenticator : public Authenticator
{
    Q_OBJECT

public:
    SqlAuthenticator(QObject *parent = 0);
    virtual ~SqlAuthenticator();

public slots:
    /* General */
    bool isAvailable() const;
    QString backendId() const;
    QString displayName() const;
    QString description() const;
    virtual inline QVariantList setupData() const { return {}; }

    virtual inline bool canChangePassword() const { return true; }

    bool setup(const QVariantMap &settings, const QProcessEnvironment &environment,
               bool loadFromEnvironment);
    State init(const QVariantMap &settings, const QProcessEnvironment &environment,
               bool loadFromEnvironment);
    UserId validateUser(const QString &user, const QString &password);

    /* User handling */
    //virtual UserId getUserId(const QString &username);
};
