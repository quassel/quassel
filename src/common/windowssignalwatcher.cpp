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

#include "windowssignalwatcher.h"

#include <signal.h>
#include <windows.h>

#include <QDebug>

// This handler is called by Windows in a different thread when a console event happens
// FIXME: When the console window is closed, the application is supposedly terminated as soon as
//        this handler returns. We may want to block and wait for the main thread so set some
//        condition variable once shutdown is complete...
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:      // Ctrl+C
    case CTRL_CLOSE_EVENT:  // Closing the console window
        WindowsSignalWatcher::signalHandler(SIGTERM);
        return TRUE;
    default:
        return FALSE;
    }
}

WindowsSignalWatcher::WindowsSignalWatcher(QObject* parent)
    : AbstractSignalWatcher{parent}
    , Singleton<WindowsSignalWatcher>{this}
{
    static bool registered = []() {
        qRegisterMetaType<AbstractSignalWatcher::Action>();
        return true;
    }();
    Q_UNUSED(registered)

    // Use POSIX-style API to register standard signals.
    // Not sure if this is safe to use, but it has worked so far...
    signal(SIGTERM, signalHandler);
    signal(SIGINT,  signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGSEGV, signalHandler);

    // React on console window events
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
}

void WindowsSignalWatcher::signalHandler(int signal)
{
    qInfo() << "Caught signal" << signal;

    switch (signal) {
    case SIGINT:
    case SIGTERM:
        emit instance()->handleSignal(Action::Terminate);
        break;
    case SIGABRT:
    case SIGSEGV:
        emit instance()->handleSignal(Action::HandleCrash);
        break;
    default:
        ;
    }
}
