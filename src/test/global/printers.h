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

#include "test-global-export.h"

#include <iostream>

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "testglobal.h"

// GTest printers for commonly used Qt types

TEST_GLOBAL_EXPORT void PrintTo(const QByteArray&, std::ostream*);
TEST_GLOBAL_EXPORT void PrintTo(const QDateTime&, std::ostream*);
TEST_GLOBAL_EXPORT void PrintTo(const QString&, std::ostream*);
TEST_GLOBAL_EXPORT void PrintTo(const QVariant&, std::ostream*);
TEST_GLOBAL_EXPORT void PrintTo(const QVariantList&, std::ostream*);
TEST_GLOBAL_EXPORT void PrintTo(const QVariantMap&, std::ostream*);
