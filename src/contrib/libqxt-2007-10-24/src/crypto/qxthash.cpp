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

#include "qxthash.h"
#include "qxthash_p.h"

/**
\class QxtHash QxtHash

\ingroup QxtCrypto

\brief Hashing funcions

\code
qDebug()<<QxtHash(QxtHash::Md5,"foo").hash();
\endcode
*/
/**
\fn QxtHash::QxtHash (Algorithm algo)
constructs a new hash with Algorithm \p algo
*/
QxtHash::QxtHash (Algorithm algo)
{
    QXT_INIT_PRIVATE(QxtHash);
    qxt_d().algo=algo;
    reset();
}
/**
\fn QxtHash::QxtHash (Algorithm algo, const QByteArray & data)
constructs a new hash with Algorithm \p algo and reads the data \p data
*/
QxtHash::QxtHash (Algorithm algo, const QByteArray & data)
{
    QXT_INIT_PRIVATE(QxtHash);
    qxt_d().algo=algo;
    reset();
    append(data);
}
/**
\fn void QxtHash::append ( const QByteArray & dt)
appends the data \p dt to the current data
*/

void QxtHash::append ( const QByteArray & dt)
{
    if (qxt_d().algo==Md5)
    {
        MD5Update(&qxt_d().md5ctx, (const unsigned char *)dt.constData(), dt.length());
    }
    else if (qxt_d().algo==Md4)
    {
        md4_update(&qxt_d().md4ctx, (const unsigned char *)dt.constData(), dt.length());
    }

}
/**
\fn void QxtHash::operator+= ( const QByteArray & dt)
appends the data \p dt to the current data
*/
void  QxtHash::operator+= ( const QByteArray & dt)
{
    append(dt);
}
/**
\fn void QxtHash::reset()
resets teh hash, deletes all data
*/
void QxtHash::reset()
{
    if (qxt_d().algo==Md5)
    {
        MD5Init(&qxt_d().md5ctx);
    }
    else if (qxt_d().algo==Md4)
    {
        md4_init(&qxt_d().md4ctx);
    }
}
/**
\fn QByteArray QxtHash::hash()
returns the resulting hash
*/

QByteArray QxtHash::hash()
{
    QByteArray hs;

    if (qxt_d().algo==Md5)
    {
        hs.resize(16);
        MD5Final(&qxt_d().md5ctx, (unsigned char *)hs.data());
    }
    else if (qxt_d().algo==Md4)
    {
        hs.resize(MD4_RESULTLEN);
        md4_final(&qxt_d().md4ctx, (unsigned char *)hs.data());
    }

    return hs;
}


///uh oh
#include "thirdparty/md5.cpp"
#include "thirdparty/md4.cpp"


