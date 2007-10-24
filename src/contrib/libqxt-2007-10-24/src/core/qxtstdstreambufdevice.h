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
#ifndef QxtStdStreambufDevice_HEADER_GUARD
#define QxtStdStreambufDevice_HEADER_GUARD

#include <QIODevice>
#include <QObject>
#include <iostream>
#include "qxtglobal.h"

class QXT_CORE_EXPORT QxtStdStreambufDevice : public QIODevice
{
    Q_OBJECT
public:
    QxtStdStreambufDevice(std::streambuf *,QObject * parent=0); /// one rw device
    QxtStdStreambufDevice(std::streambuf * r,std::streambuf * w,QObject * parent=0); ///one for read and one for write

    virtual bool isSequential () const;
    virtual qint64 bytesAvailable () const;
protected:
    virtual qint64 readData ( char * data, qint64 maxSize );
    virtual qint64 writeData ( const char * data, qint64 maxSize );


private:
    std::streambuf * buff;
    std::streambuf * buff_w;

};

#endif
