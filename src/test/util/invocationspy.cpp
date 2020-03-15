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

#include "invocationspy.h"

namespace test {

InvocationSpy::InvocationSpy(QObject* parent)
    : QObject(parent)
    , _internalSpy{this, &InvocationSpy::notified}
{}

void InvocationSpy::notify()
{
    emit notified();
}

bool InvocationSpy::wait(std::chrono::milliseconds timeout)
{
    if (_internalSpy.count() > 0) {
        _internalSpy.clear();
        return true;
    }
    bool result = _internalSpy.wait(timeout.count());
    _internalSpy.clear();
    return result;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

bool SignalSpy::wait(std::chrono::milliseconds timeout)
{
    bool result = InvocationSpy::wait(timeout);
    for (auto&& connection : _connections) {
        QObject::disconnect(connection);
    }
    _connections.clear();
    return result;
}

}  // namespace test
