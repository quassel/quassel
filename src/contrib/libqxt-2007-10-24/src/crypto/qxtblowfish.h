/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCrypto module of the Qt eXTension library
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

#ifndef QXT__BLOWFISH_H_sadasd
#define QXT__BLOWFISH_H_sadasd

#include <QByteArray>
#include <QObject>
#include <qxtglobal.h>
struct bf_key_st;
class QXT_NETWORK_EXPORT QxtBlowFish : public QObject
{
    Q_OBJECT

public:
    QxtBlowFish(QObject * parent = 0);
    ~QxtBlowFish();

    void setPassword(QByteArray);
    QByteArray encrypt(QByteArray);
    QByteArray decrypt(QByteArray);

private:
    bf_key_st * key;

};


#endif


