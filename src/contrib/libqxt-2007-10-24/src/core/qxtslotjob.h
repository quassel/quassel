
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
#ifndef QXTJOB_H_GUARD
#define QXTJOB_H_GUARD


#include <QVariant>

#include <qxtpimpl.h>
#include <qxtglobal.h>
#include <qxtjob.h>


class QxtSignalWaiter;
class QxtSlotJob;
class QXT_CORE_EXPORT QxtFuture: public QObject
{
Q_OBJECT
public:
    QVariant delayedResult(int msec=-1);
    QVariant joinedResult();

    QxtFuture(const QxtFuture& other);
    ~QxtFuture();
private:
    friend class QxtSlotJob;
    QxtFuture(QxtSlotJob* j);
    QxtSlotJob * job;
    QxtSignalWaiter * waiter;
signals:
    void done();
    void done(QVariant);
};


class QxtSlotJobPrivate;
class QThread;
class QXT_CORE_EXPORT QxtSlotJob : public QxtJob
{
Q_OBJECT
public:
     static QxtFuture detach(QThread * o,QObject* recv, const char* slot,
        QGenericArgument p1=QGenericArgument(),
        QGenericArgument p2=QGenericArgument(),
        QGenericArgument p3=QGenericArgument(),
        QGenericArgument p4=QGenericArgument(),
        QGenericArgument p5=QGenericArgument(),
        QGenericArgument p6=QGenericArgument(),
        QGenericArgument p7=QGenericArgument(),
        QGenericArgument p8=QGenericArgument(),
        QGenericArgument p9=QGenericArgument(),
        QGenericArgument p10=QGenericArgument()); 

    QxtSlotJob(QObject* recv, const char* slot,
        QGenericArgument p1=QGenericArgument(),
        QGenericArgument p2=QGenericArgument(),
        QGenericArgument p3=QGenericArgument(),
        QGenericArgument p4=QGenericArgument(),
        QGenericArgument p5=QGenericArgument(),
        QGenericArgument p6=QGenericArgument(),
        QGenericArgument p7=QGenericArgument(),
        QGenericArgument p8=QGenericArgument(),
        QGenericArgument p9=QGenericArgument(),
        QGenericArgument p10=QGenericArgument());

    QVariant result();
    QxtFuture exec(QThread *o);

protected:
    virtual void run();
signals:
    void done(QVariant);
private:
    QXT_DECLARE_PRIVATE(QxtSlotJob);
///must not be in pimpl. that's heavy doom when they are both Qobject and one moves to another thread
private slots:  
    void pdone();

};




#endif


