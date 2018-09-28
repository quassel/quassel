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

#include <QtGlobal>

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
    Singleton(T *instance)
    {
        if (_destroyed) {
            qFatal("Trying to reinstantiate a destroyed singleton, this must not happen!");
            abort();  // This produces a backtrace, which is highly useful for finding the culprit
        }
        if (_instance) {
            qFatal("Trying to reinstantiate a singleton that is already instantiated, this must not happen!");
            abort();
        }
        _instance = instance;
    }

    // Satisfy Rule of Five
    Singleton(const Singleton &) = delete;
    Singleton(Singleton &&) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton &operator=(Singleton &&) = delete;

    /**
     * Destructor.
     *
     * Sets the instance pointer to null and flags the destruction, so a subsequent reinstantiation will fail.
     */
    ~Singleton()
    {
        _instance = nullptr;
        _destroyed = true;
    }

    /**
     * Accesses the instance pointer.
     *
     * If the singleton hasn't been instantiated yet, the program is aborted. No lazy instantiation takes place,
     * because the singleton's lifetime shall be explicitly controlled.
     *
     * @returns A pointer to the instance
     */
    static T *instance()
    {
        if (_instance) {
            return _instance;
        }
        qFatal("Trying to access a singleton that has not been instantiated yet");
        abort();
    }

private:
    static T *_instance;
    static bool _destroyed;

};

template<typename T>
T *Singleton<T>::_instance{nullptr};

template<typename T>
bool Singleton<T>::_destroyed{false};
