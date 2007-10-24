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
#include <QMutex>
#include <qxtjob.h>
#include <QWaitCondition>




class QxtJobPrivate : public QObject, public QxtPrivate<QxtJob>
{
Q_OBJECT
public:
    class RunningState
    {
    public:
        void set(bool a)
        {
            mutex.lock();
            r=a;
            mutex.unlock();
        }
        bool get()
        {
            mutex.lock();
            bool a=r;
            mutex.unlock();
            return a;
        }

        QMutex mutex;
        bool r;

    } running;

    QXT_DECLARE_PUBLIC(QxtJob);
    QMutex mutexa;
    QWaitCondition synca;


public slots:
    void inwrap_d();
signals:
    void done();

};



