/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <iostream>

// For Windows DLLs, we must not export the template function.
// For other systems, we must, otherwise the local static variables won't work across library boundaries...
#ifndef Q_OS_WIN
#    include "common-export.h"
#    define QUASSEL_SINGLETON_EXPORT COMMON_EXPORT
#else
#    define QUASSEL_SINGLETON_EXPORT
#endif

namespace detail {

// This needs to be a free function instead of a member function of Singleton, because
// - MSVC can't deal with static attributes in an exported template class
// - Clang produces weird and unreliable results for local static variables in static member functions
//
// We need to export the function on anything not using Windows DLLs, otherwise local static members don't
// work across library boundaries.
template<typename T>
QUASSEL_SINGLETON_EXPORT T* getOrSetInstance(T* instance = nullptr, bool destroyed = false)
{
    static T* _instance = instance;
    static bool _destroyed = destroyed;

    if (destroyed) {
        _destroyed = true;
        return _instance = nullptr;
    }
    if (instance) {
        if (_destroyed) {
            std::cerr << "Trying to reinstantiate a destroyed singleton, this must not happen!\n";
            abort();  // This produces a backtrace, which is highly useful for finding the culprit
        }
        if (_instance != instance) {
            std::cerr << "Trying to reinstantiate a singleton that is already instantiated, this must not happen!\n";
            abort();
        }
    }
    if (!_instance) {
        std::cerr << "Trying to access a singleton that has not been instantiated yet!\n";
        abort();
    }
    return _instance;
}

}  // detail

/**
 * Mixin class for "pseudo" singletons.
 *
 * Classes inheriting from this mixin become "pseudo" singletons that can still be constructed
 * and destroyed in a controlled manner, but also gain an instance() method for static access.
 * This is very similar to the behavior of e.g. QCoreApplication.
 *
 * The mixin protects against multiple instantiation, use-before-instantiation and use-after-destruction
 * by aborting the program. This is intended to find lifetime issues during development; abort()
 * produces a backtrace that makes it easy to find the culprit.
 *
 * The Curiously Recurring Template Pattern (CRTP) is used for the mixin to be able to provide a
 * correctly typed instance pointer.
 */
template<typename T>
class Singleton
{
public:
    /**
     * Constructs the mixin.
     *
     * The constructor can only be called once; subsequent invocations abort the program.
     *
     * @param instance Pointer to the instance being created, i.e. the 'this' pointer of the parent class
     */
    Singleton(T* instance)
    {
        detail::getOrSetInstance<T>(instance);
    }

    // Satisfy Rule of Five
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    /**
     * Destructor.
     *
     * Sets the instance pointer to null and flags the destruction, so a subsequent reinstantiation will fail.
     */
    ~Singleton()
    {
        detail::getOrSetInstance<T>(nullptr, true);
    }

    /**
     * Accesses the instance pointer.
     *
     * If the singleton hasn't been instantiated yet, the program is aborted. No lazy instantiation takes place,
     * because the singleton's lifetime shall be explicitly controlled.
     *
     * @returns A pointer to the instance
     */
    static T* instance()
    {
        return detail::getOrSetInstance<T>();
    }
};
