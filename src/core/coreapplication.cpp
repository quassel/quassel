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

#include "coreapplication.h"

CoreApplication::CoreApplication(int& argc, char** argv)
    : QCoreApplication(argc, argv)
{
    Quassel::registerQuitHandler([this]() {
        connect(_core.get(), &Core::shutdownComplete, this, &CoreApplication::onShutdownComplete);
        _core->shutdown();
    });
}

void CoreApplication::init()
{
    _core = std::make_unique<Core>();
    _core->init();
}

void CoreApplication::onShutdownComplete()
{
    connect(_core.get(), &QObject::destroyed, QCoreApplication::instance(), &QCoreApplication::quit);
    _core.release()->deleteLater();
}
