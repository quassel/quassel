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

/**
\class QxtJob QxtJob

\ingroup QxtCore

\brief Execute a Job on a QThread. once or multiple times.

QxtJob allows easily starting jobs on different threads.\n
exec() will ask for the QThread to run the job on.
The Qthread needs an event loop. 4.3 and later versions of Qt have 
a non virtual QThread with a default event loop, allowing easy deployment of jobs.

\code
QThread thread;
thread.start();
LockJob().exec(&thread);
\endcode
*/

#include "qxtjob_p.h"
#include <cassert>
#include <QThread>

class Thread : public QThread
{
public:
    static void usleep(unsigned long usecs)
    {
        QThread::usleep(usecs);
    }
};

QxtJob::QxtJob()
{
    QXT_INIT_PRIVATE(QxtJob);
    qxt_d().running.set(false);
    connect(&qxt_d(),SIGNAL(done()),this,SIGNAL(done()));
}
/*!
execute the Job on \p onthread \n
*/
void QxtJob::exec(QThread * onthread)
{
    qxt_d().moveToThread(onthread);
    connect(this,SIGNAL(subseed()),&qxt_d(),SLOT(inwrap_d()),Qt::QueuedConnection);

    qxt_d().running.set(true);
    emit(subseed());
}
/*!
The dtor joins.
Means it blocks until the job is finished
*/
QxtJob::~QxtJob()
{
    join();
}
/*!
block until the Job finished \n
Note that the current thread will be blocked. \n
If you use this, you better be damn sure you actually want a thread.\n
Maybe you actualy want to use QxtSignalWaiter.
*/
void QxtJob::join()
{
    while(qxt_d().running.get()==true)
    {
        /**
        oh yeah that sucks ass, 
        but since any kind of waitcondition will just fail due to undeterminnism,
        we have no chance then polling.
        And no, a mutex won't work either.
        using join for anything else then testcases sounds kindof retarded anyway.
        */
        Thread::usleep(1000);
    }

}
void QxtJobPrivate::inwrap_d()
{
    synca.wakeAll();
    qxt_p().run();
    running.set(false);
    emit(done());
}









