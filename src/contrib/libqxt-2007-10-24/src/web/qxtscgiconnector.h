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
#ifndef QxtScgiConnector_header_guards_oaksndoapsid
#define QxtScgiConnector_header_guards_oaksndoapsid


#include <QByteArray>
#include <QHostAddress>
#include <qxtpimpl.h>
#include "qxtabstractwebconnector.h"

class QxtScgiConnectorPrivate;
class QxtScgiConnector : public QxtAbstractWebConnector
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtScgiConnector);

public:
    QxtScgiConnector();
    virtual int  start (quint16 port,const QHostAddress & address);

    virtual QIODevice * socket();
    virtual void sendHeader(server_t &);

    virtual void close();

    virtual QByteArray content(quint64 maxsize);
};


#endif
