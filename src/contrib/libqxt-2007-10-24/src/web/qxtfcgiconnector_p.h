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
#include "qxtfcgiconnector.h"
#include <QTcpSocket>
#include <QTcpServer>


#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif
#include "fcgio.h"
#include "fcgi_config.h"
#include <QMetaType>
#include <QThread>
#include "qxtstdstreambufdevice.h"

Q_DECLARE_METATYPE(FCGX_Request)

// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;


class QxtFcgiConnectorPrivate : public QThread,public QxtPrivate<QxtFcgiConnector>
{
    QXT_DECLARE_PUBLIC(QxtFcgiConnector);
    Q_OBJECT
public:
    QxtFcgiConnectorPrivate();
    void run();

    QxtStdStreambufDevice * io;

    bool open;


    FCGX_Request request;

signals:
    void close_ss();



};


