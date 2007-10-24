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
\class QxtFifo QxtFifo

\ingroup QxtCore

\brief Simple Loopback QIODevice

read and write to the same object \n
emits a readyRead Signal. \n
usefull for loopback tests where QBuffer does not work.

\code
QxtFifo fifo;
 QTextStream (&fifo)<<QString("foo");
 QString a;
 QTextStream(&fifo)>>a;
 qDebug()<<a;
\endcode

*/
#include "qxtfifo.h"
#include <QDebug>

QxtFifo::QxtFifo(QObject *parent):QIODevice(parent)
{
    setOpenMode (QIODevice::ReadWrite);
}

qint64 QxtFifo::readData ( char * data, qint64 maxSize )
{
    qint64 i=0;
    for (;i<maxSize;i++)
    {
        if (q.isEmpty())
            break;
        (*data++)=q.dequeue();
    }
    return i;
}
qint64 QxtFifo::writeData ( const char * data, qint64 maxSize )
{
    qint64 i=0;
    for (;i<maxSize;i++)
        q.enqueue(*data++);

    if (i>0)
        emit(readyRead ());
    return maxSize;
}


bool QxtFifo::isSequential () const
{
    return true;
}


qint64 QxtFifo::bytesAvailable () const
{
    return q.count();
}







