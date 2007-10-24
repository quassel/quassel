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
#include "qxtstdstreambufdevice.h"

/**
\class QxtStdStreambufDevice QxtStdStreambufDevice

\ingroup QxtCore

\brief QIODevice support for std::streambuf

does NOT include the readyRead() signal

*/

/**
\fn QxtStdStreambufDevice::QxtStdStreambufDevice(std::streambuf * b,QObject * parent)

creates a QxtStdStreambufDevice using a single stream buffer as in and output

 */
QxtStdStreambufDevice::QxtStdStreambufDevice(std::streambuf * b,QObject * parent):QIODevice(parent),buff(b)
{
    setOpenMode (QIODevice::ReadWrite); ///we don't know the real state
    buff_w=0;
}
/**
\fn QxtStdStreambufDevice::QxtStdStreambufDevice(std::streambuf * r,std::streambuf * w,QObject * parent)
creates a QxtStdStreambufDevice using \p r to read and \p w to write
 */

QxtStdStreambufDevice::QxtStdStreambufDevice(std::streambuf * r,std::streambuf * w,QObject * parent):QIODevice(parent),buff(r),buff_w(w)
{
    setOpenMode (QIODevice::ReadWrite);
}
bool QxtStdStreambufDevice::isSequential() const
{
    return true;///for now
}

qint64 QxtStdStreambufDevice::bytesAvailable () const
{
    return buff->in_avail();
}
qint64 QxtStdStreambufDevice::readData ( char * data, qint64 maxSize )
{
    return buff->sgetn(data,maxSize);
}

qint64 QxtStdStreambufDevice::writeData ( const char * data, qint64 maxSize )
{
    if (buff_w)
        return buff_w->sputn(data,maxSize);
    return buff->sputn(data,maxSize);
}
