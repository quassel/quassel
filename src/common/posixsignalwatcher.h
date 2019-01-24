/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include <QObject>

#include "abstractsignalwatcher.h"

class QSocketNotifier;

/**
 * Signal watcher/handler for POSIX systems.
 *
 * Uses a local socket to notify the main thread in a safe way.
 */
class PosixSignalWatcher : public AbstractSignalWatcher
{
    Q_OBJECT

public:
    PosixSignalWatcher(QObject* parent = nullptr);

private:
    static void signalHandler(int signal);

    void registerSignal(int signal);

private slots:
    void onNotify(int sockfd);

private:
    static int _sockpair[2];
    QSocketNotifier* _notifier{nullptr};
};
