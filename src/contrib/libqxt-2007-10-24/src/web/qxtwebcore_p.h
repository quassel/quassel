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
#include "qxtwebcore.h"
#include <QTextStream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEncoder>

class QxtScgiController;
class QxtWebCorePrivate : public QObject,public QxtPrivate<QxtWebCore>
{
    Q_OBJECT
    QXT_DECLARE_PUBLIC(QxtWebCore);

public:
    QxtWebCorePrivate(QObject *parent = 0);
    void send(QString);
    void sendheader();
    void header(QString,QString);
    void redirect(QString,int );
    void close();


    QxtAbstractWebConnector * connector;
    server_t currentservert;
    bool header_sent;
    server_t answer;
    QTextDecoder *decoder;
    QTextEncoder *encoder;
public slots:
    void incomming(server_t  SERVER);

};
