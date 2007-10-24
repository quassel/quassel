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
#ifndef QxtWebController_H_sapoidnasoas
#define QxtWebController_H_sapoidnasoas

#include <QObject>
#include <QTextStream>
#include <QVariant>
#include "qxtwebcore.h"

class QxtWebController : public QObject
{
    Q_OBJECT
public:
    QxtWebController(QString name);
    int invoke(server_t &);
    static QString WebRoot();
public slots:
    int index()
    {
        echo()<<"overwrite the index function of this controller("+objectName()+")";
        return 404;
    }

protected:
    QTextStream & echo();
    QString self();
    server_t SERVER;
private:
    QTextStream *stream_m;
};

#endif

