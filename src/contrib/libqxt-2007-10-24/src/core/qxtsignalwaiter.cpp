/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/


#include <qxtsignalwaiter.h>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

QxtSignalWaiter::QxtSignalWaiter(const QObject* sender, const char* signal) : QObject(0)
{
    Q_ASSERT(sender && signal);
    ready=false;
    connect(sender, signal, this, SLOT(signalCaught()));

}

// Returns true if the signal was caught, returns false if the wait timed out
bool QxtSignalWaiter::wait(const QObject* sender, const char* signal, int msec)
{
    QxtSignalWaiter w(sender, signal);
    return w.wait(msec);
}

// Returns true if the signal was caught, returns false if the wait timed out
bool QxtSignalWaiter::wait(int msec,bool reset)
{
    // Check input parameters
    if (msec < -1) return false;

    // activate the timeout
    if (msec != -1) timerID = startTimer(msec);

    if(reset)
            ready=false;
    // Begin waiting
    timeout = false;
    while (!ready && !timeout)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

    // Clean up and return status
    if (msec != -1) killTimer(timerID);
    return ready || !timeout;
}

void QxtSignalWaiter::signalCaught()
{
    ready = true;
}

void QxtSignalWaiter::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event);
    killTimer(timerID);
    timeout = true;
}
