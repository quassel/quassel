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
#include "qxtstdio.h"
#include <cstdio>
#include <QSocketNotifier>

/**
\class QxtStdio QxtStdio

\ingroup QxtCore

\brief QIODevice support for stdin and stdout

including readyRead() signal
note that when using this class, the buffers for stdin/stdout will be disabled, and NOT reenabled on destruction

perfect as a counter part for QProcess
*/

QxtStdio::QxtStdio(QObject * parent):QIODevice(parent)
{
    setvbuf ( stdin , NULL , _IONBF , 0 );
    setvbuf ( stdout , NULL , _IONBF , 0 );

    setOpenMode (QIODevice::ReadWrite);
    notify = new QSocketNotifier (

#ifdef Q_CC_MSVC
                 _fileno(stdin)
#else
                 fileno(stdin)
#endif

                 ,QSocketNotifier::Read,this );
    connect(notify, SIGNAL(activated(int)),this,SLOT(activated(int)));
}

qint64 QxtStdio::readData ( char * data, qint64 maxSize )
{
    qint64 i=0;
    for (;i<maxSize;i++)
    {
        if (inbuffer.isEmpty())
            break;
        (*data++)=inbuffer.dequeue();
    }
    return i;
}
qint64 QxtStdio::writeData ( const char * data, qint64 maxSize )
{
    qint64 i=0;
    for (;i<maxSize;i++)
    {
        char c=*data++;
        putchar(c);
    }
// 	emit(bytesWritten (i)); ///FIXME: acording to the docs this may not be recoursive. how do i prevent that?
    return i;
}


bool QxtStdio::isSequential () const
{
    return true;
}


qint64 QxtStdio::bytesAvailable () const
{
    return inbuffer.count();
}

void QxtStdio::activated(int )
{
    inbuffer.enqueue(getchar());
    emit(readyRead ());
}


