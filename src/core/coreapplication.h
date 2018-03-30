/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include <QCoreApplication>

#include "quassel.h"

/// Encapsulates CoreApplication's logic.
/** This allows it to be reused within MonolithicApplication as well.
 */
class CoreApplicationInternal
{
    Q_DECLARE_TR_FUNCTIONS(CoreApplicationInternal)

public:
    CoreApplicationInternal();
    ~CoreApplicationInternal();

    bool init();

private:
    bool _coreCreated;
};


class CoreApplication : public QCoreApplication
{
    Q_OBJECT
public:
    CoreApplication(int &argc, char **argv);
    ~CoreApplication();

    bool init();

private:
    CoreApplicationInternal *_internal;
};
