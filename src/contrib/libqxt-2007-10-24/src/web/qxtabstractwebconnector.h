/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtWeb  module of the Qt eXTension library
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
** <http://libqxt.org>  <foundation@libqxt.org>
**
****************************************************************************/
#ifndef QxtAbstractWebConnector_header_guards_oaksndoapsid
#define QxtAbstractWebConnector_header_guards_oaksndoapsid


#include <QByteArray>
#include <QHostAddress>

#include "qxtwebcore.h"


class QxtAbstractWebConnector : public QObject
{
    Q_OBJECT
public:
    virtual int  start (quint16 port,const QHostAddress & address)=0;


    virtual QIODevice * socket()=0;
    virtual void sendHeader(server_t &)=0;

    virtual void close()=0;

    virtual QByteArray content(quint64 maxsize)=0;

signals:
    void incomming(server_t );
    void aboutToClose();
};


#endif
