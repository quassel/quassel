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

#include <QMetaType>
#include <QObject>

/**
 * Interface for watching external, asynchronous events like POSIX signals.
 *
 * Abstracts the platform-specific details away, since we're only interested
 * in the action to take.
 */
class AbstractSignalWatcher : public QObject
{
    Q_OBJECT

public:
    enum class Action
    {
        Reload,      ///< Configuration should be reloaded (e.g. SIGHUP)
        Terminate,   ///< Application should be terminated (e.g. SIGTERM)
        HandleCrash  ///< Application is crashing (e.g. SIGSEGV)
    };

    using QObject::QObject;

signals:
    /// An event/signal was received and the given action should be taken
    void handleSignal(AbstractSignalWatcher::Action action);
};

Q_DECLARE_METATYPE(AbstractSignalWatcher::Action)
