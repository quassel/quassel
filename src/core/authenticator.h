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

#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <QtCore>

#include "types.h"

class Authenticator : public QObject {

    Q_OBJECT

public:
    Authenticator(QObject *parent = 0);
    virtual ~Authenticator() {};

    enum State {
        IsReady,        // ready to go
        NeedsSetup,        // need basic setup (ask the user for input)
        NotAvailable     // remove the authenticator backend from the list of avaliable authenticators.
    };


public slots:
    // General

    //! Check if the authenticator type is available.
    /** An authenticator subclass should return true if it can be successfully used, i.e. if all
     *  prerequisites are in place.
     * \return True if and only if the authenticator class can be successfully used.
     */
    virtual bool isAvailable() const = 0;

    //! Returns the display name of the authenticator backend
    /** \return A string that can be used by the client to name the authenticator backend */
    virtual QString backendId() const = 0;

    //! Returns a description of this authenticator backend
    /** \return A string that can be displayed by the client to describe the authenticator */
    virtual QString description() const = 0;

    //! Returns a list of properties required to use the authenticator backend
    virtual QStringList setupKeys() const = 0;

    //! Checks if the authenticator allows manual password changes from inside quassel.
    virtual bool canChangePassword() const = 0;

    //! Returns a map where the keys are are properties to use the authenticator backend
    /*  the values are QVariants with default values */
    virtual QVariantMap setupDefaults() const = 0;

    //! Setup the authenticator provider.
    /** This prepares the authenticator provider (e.g. create tables, etc.) for use within Quassel.
     *  \param settings   Hostname, port, username, password, ...
     *  \return True if and only if the authenticator provider was initialized successfully.
     */
    virtual bool setup(const QVariantMap &settings = QVariantMap()) = 0;

    //! Initialize the authenticator provider
    /** \param settings   Hostname, port, username, password, ...
     *  \return the State the authenticator backend is now in (see authenticator::State)
     */
    virtual State init(const QVariantMap &settings = QVariantMap()) = 0;

    //! Validate a username with a given password.
    /** \param user     The username to validate
     *  \param password The user's alleged password
     *  \return A valid UserId if the password matches the username; 0 else
     */
    virtual UserId validateUser(const QString &user, const QString &password) = 0;

private:

};

#endif
