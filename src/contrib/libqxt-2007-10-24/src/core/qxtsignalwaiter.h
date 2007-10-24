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


#ifndef QXTSIGNALWAITER_H
#define QXTSIGNALWAITER_H
#include <qxtglobal.h>

#include <QObject>

/**
\class QxtSignalWaiter QxtSignalWaiter

\ingroup kit

\brief Wait until a signal ocured

In many cases, writing code that assumes certain actions are synchronous is considerably simpler than breaking your function into multiple blocks and using signals and slots to connect them all. Using this class, QSignalWaiter::wait will block until a certain signal is emitted and then return. The return value is true if the signal was caught, or false if a user-specified timeout elapses before catching the signal.



\code
void MyObject::myFunction() {
    QxtSignalWaiter waiter(myOtherObject, SIGNAL(longProcessFinished()));
    myOtherObject->longProcess();
    if(waiter.wait(5000))
        doSomething(myOtherObject->information());
    else
        QMessageBox::information(0, "MyObject", "Timed out while waiting on longProcessFinished()", QMessageBox::Ok);
}
\endcode


\bug
QxtSignalWaiter is, sadly, not reentrant. In particular, only one QxSignalWaiter object can be safely waiting at a time. If a second QxSignalWaiter is used while the first is waiting, the first will not return until the second has timed out or successfully caught its signal. A later revision of the class may be able to solve this problem. Until then, be careful not to rely on two QxtSignalWaiter objects at the same time.

*/
class QXT_CORE_EXPORT QxtSignalWaiter : public QObject
{
    Q_OBJECT
public:

    /** default constructor
        Creates a QSignalWaiter and sets the default signal to sender::signal.*/
    QxtSignalWaiter(const QObject* sender, const char* signal);

    /** wait for signal
    Waits for the signal sender::signal to be emitted. If msec is not -1, wait() will return after msec milliseconds.
    If a signal is not specified, the default signal specified in the constructor is used instead. If no default signal was specified in the constructor, wait() will block until the timeout has elapsed. If no timeout was specified, wait() will return immediately.\n
    Returns true if the signal was caught, false if it timed out. \n
    This function is not reentrant.
    */

    bool wait(int msec = -1,bool reset=false);
    static bool wait(const QObject* sender, const char* signal, int msec = -1);
protected:
    void timerEvent(QTimerEvent* event);
private slots:
    void signalCaught();
private:
    bool ready, timeout;
    int timerID;
};

#endif
