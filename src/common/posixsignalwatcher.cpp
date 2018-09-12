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

#include "posixsignalwatcher.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>

#include <QDebug>
#include <QSocketNotifier>

#include "logmessage.h"

int PosixSignalWatcher::_sockpair[2];

PosixSignalWatcher::PosixSignalWatcher(QObject *parent)
    : AbstractSignalWatcher{parent}
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, _sockpair)) {
        qWarning() << "Could not setup POSIX signal watcher:" << ::strerror(errno);
        return;
    }

    _notifier = new QSocketNotifier(_sockpair[1], QSocketNotifier::Read, this);
    connect(_notifier, &QSocketNotifier::activated, this, &PosixSignalWatcher::onNotify);
    _notifier->setEnabled(true);

    registerSignal(SIGINT);
    registerSignal(SIGTERM);
    registerSignal(SIGHUP);

#ifdef HAVE_BACKTRACE
    // we only handle crashes ourselves if coredumps are disabled
    struct rlimit *limit = (rlimit *)malloc(sizeof(struct rlimit));
    int rc = getrlimit(RLIMIT_CORE, limit);
    if (rc == -1 || !((long)limit->rlim_cur > 0 || limit->rlim_cur == RLIM_INFINITY)) {
        registerSignal(SIGABRT);
        registerSignal(SIGSEGV);
        registerSignal(SIGBUS);
    }
    free(limit);
#endif
}

void PosixSignalWatcher::registerSignal(int signal)
{
    struct sigaction sigact;
    sigact.sa_handler = PosixSignalWatcher::signalHandler;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags |= SA_RESTART;
    if (sigaction(signal, &sigact, nullptr)) {
        qWarning() << "Could not register handler for POSIX signal:" << ::strerror(errno);
    }
}

void PosixSignalWatcher::signalHandler(int signal)
{
    auto bytes = ::write(_sockpair[0], &signal, sizeof(signal));
    Q_UNUSED(bytes)
}

void PosixSignalWatcher::onNotify(int sockfd)
{
    int signal;
    auto bytes = ::read(sockfd, &signal, sizeof(signal));
    Q_UNUSED(bytes)
    quInfo() << "Caught signal" << signal;

    switch (signal) {
    case SIGHUP:
        emit handleSignal(Action::Reload);
        break;
    case SIGINT:
    case SIGTERM:
        emit handleSignal(Action::Terminate);
        break;
    case SIGABRT:
    case SIGSEGV:
    case SIGBUS:
        emit handleSignal(Action::HandleCrash);
        break;
    default:
        ;
    }
}
