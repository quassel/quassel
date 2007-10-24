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
#ifndef QxtWebCore_HEADER_GIAURX_H
#define QxtWebCore_HEADER_GIAURX_H

#include <QObject>
#include <QMap>
#include <QMetaType>

#include <qxterror.h>
#include <qxtpimpl.h>

#include <qxtglobal.h>
#include <QHostAddress>

typedef  QMap<QByteArray, QByteArray> server_t;
typedef  QMap<QString, QVariant> post_t;


Q_DECLARE_METATYPE(server_t)
class QIODevice;
class QxtAbstractWebConnector;
class QTextCodec;
class QxtWebCorePrivate;
class QXT_WEB_EXPORT QxtWebCore: public QObject
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtWebCore);
public:
    QxtWebCore (QxtAbstractWebConnector *);
    ~QxtWebCore ();

    int start (quint16 port = 8000,const QHostAddress & address = QHostAddress::LocalHost);

    static void setCodec ( QTextCodec * codec );

    static void send(QString);
    static void close();
    static void header(QString,QString);
    static void sendHeader();

    static server_t & SERVER();
    static QIODevice * socket();

    static void redirect(QString location,int code=303);




    static QxtWebCore * instance();

    /*helper*/
    static QxtError parseString(QByteArray str, post_t & POST);
    static QByteArray content(int maxsize=5000);

signals:
    void request();
    void aboutToClose();
};



#endif


