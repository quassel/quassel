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

#include "test-util-export.h"

#include <chrono>

#include <QObject>
#include <QSignalSpy>

#include <boost/optional.hpp>

namespace test {

/**
 * Waits while spinning the event loop until notified, or timed out.
 *
 * Based on QSignalSpy (hence the name), but provides an API that is much more useful
 * for writing asynchronous test cases.
 */
class TEST_UTIL_EXPORT InvocationSpy : public QObject
{
    Q_OBJECT

public:
    InvocationSpy(QObject* parent = nullptr);

    /**
     * Notifies the spy, which will cause it to return from wait().
     */
    void notify();

    /**
     * Waits for the spy to be notified within the given timeout.
     *
     * @param timeout Timeout for waiting
     * @returns true if the spy was notified, and false if it timed out.
     */
    bool wait(std::chrono::milliseconds timeout = std::chrono::seconds{60});

signals:
    /// Internally used signal
    void notified();

private:
    QSignalSpy _internalSpy;
};

/**
 * Spy that allows to be notified with a value.
 *
 * Works like @a InvocationSpy, but takes a value when notified. After successful notification, the value
 * can be accessed and used for test case expectations.
 */
template<typename T>
class ValueSpy : public InvocationSpy
{
public:
    using InvocationSpy::InvocationSpy;

    /**
     * Notifies the spy with the given value.
     *
     * @param value The notification value
     */
    void notify(const T& value)
    {
        _value = value;
        InvocationSpy::notify();
    }

    /**
     * Provides the value the spy was last notified with.
     *
     * @note The value is only valid if wait() returned with true.
     * @returns The value given to notify(), or boost::none if the spy wasn't notified
     */
    T value() const { return *_value; }

private:
    boost::optional<T> _value;
};

}  // namespace test
