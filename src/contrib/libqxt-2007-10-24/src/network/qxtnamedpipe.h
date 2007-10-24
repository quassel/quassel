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

#ifndef QXTNAMEDPIPE_H
#define QXTNAMEDPIPE_H

#include <QAbstractSocket>
#include <QString>
#include <QByteArray>
#include <qxtpimpl.h>
#include <qxtglobal.h>

class QxtNamedPipePrivate;
class QXT_NETWORK_EXPORT QxtNamedPipe : public QAbstractSocket
{
    Q_OBJECT
public:
    QxtNamedPipe(const QString& name = QString(), QObject* parent = 0);

    bool open(QIODevice::OpenMode mode);
    bool open(const QString& name, QIODevice::OpenMode mode);
    void close();

    QByteArray readAvailableBytes();

private:
    QXT_DECLARE_PRIVATE(QxtNamedPipe);
};

#endif
