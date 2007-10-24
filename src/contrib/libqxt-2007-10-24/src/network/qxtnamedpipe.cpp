/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtNetwork module of the Qt eXTension library
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
\class QxtNamedPipe QxtNamedPipe

\ingroup QxtNetwork

\brief Provides a QIODevice over a named pipe

\note not part of 0.2.4
*/




#include "qxtnamedpipe.h"
#ifdef Q_OS_UNIX
#    include <fcntl.h>
class QxtNamedPipePrivate : public QxtPrivate<QxtNamedPipe>
{
public:
    QxtNamedPipePrivate()
    {}
    QXT_DECLARE_PUBLIC(QxtNamedPipe);

    QString pipeName;
    int fd;
};

QxtNamedPipe::QxtNamedPipe(const QString& name, QObject* parent) : QAbstractSocket(QAbstractSocket::UnknownSocketType, parent)
{
    QXT_INIT_PRIVATE(QxtNamedPipe);
    qxt_d().pipeName = name;
    qxt_d().fd = 0;
}

bool QxtNamedPipe::open(QIODevice::OpenMode mode)
{
    int m = O_RDWR;

    if (!(mode & QIODevice::ReadOnly))         ///FIXME: what?
        m = O_WRONLY;
    else if (!(mode & QIODevice::WriteOnly))
        m = O_RDONLY;
    qxt_d().fd = ::open(qPrintable(qxt_d().pipeName), m);

    if (qxt_d().fd != 0)
    {
        setSocketDescriptor(qxt_d().fd, QAbstractSocket::ConnectedState, mode);
        setOpenMode ( mode);
        return true;
    }
    else
    {
        return false;
    }
}

bool QxtNamedPipe::open(const QString& name, QIODevice::OpenMode mode)
{
    qxt_d().pipeName = name;
    return QxtNamedPipe::open(mode);
}

void QxtNamedPipe::close()
{
    if (qxt_d().fd) ::close(qxt_d().fd);
    setOpenMode(QIODevice::NotOpen);
}

QByteArray QxtNamedPipe::readAvailableBytes()
{
    char ch;
    QByteArray rv;
    while (getChar(&ch)) rv += ch;
    return rv;
}
#else
#    error "No Windows implementation for QxtNamedPipe"
#endif

