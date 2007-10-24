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
#ifndef QxtStdin_H_GUARD
#define QxtStdin_H_GUARD
#include <qxtglobal.h>
#include <QIODevice>
#include <QQueue>

class QSocketNotifier;
class QXT_CORE_EXPORT QxtStdio : public QIODevice
{
    Q_OBJECT
public:
    QxtStdio(QObject * parent=0);
    virtual bool isSequential () const;
    virtual qint64 bytesAvailable () const;
protected:
    virtual qint64 readData ( char * data, qint64 maxSize );
    virtual qint64 writeData ( const char * data, qint64 maxSize );
private:
    QSocketNotifier * notify;
    QQueue<char> inbuffer;
private slots:
    void activated(int );

};
#endif
