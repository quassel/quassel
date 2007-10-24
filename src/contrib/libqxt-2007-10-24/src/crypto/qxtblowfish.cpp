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


#include "qxtblowfish.h"
#include <openssl/blowfish.h>



/**
\class QxtBlowFish QxtBlowFish

\ingroup QxtCrypto

\brief  Blowfish Encryption Class


useage:
\code
QxtBlowFish() fish;
fish.setPassword("foobar").

QByteArray a("barblah");

a= fish.encrypt(a);
a= fish.decrypt(a);
\endcode
*/





QxtBlowFish::QxtBlowFish(QObject * parent) :QObject(parent)
{
    key=new BF_KEY;
}

QxtBlowFish::~QxtBlowFish()
{
    delete(key);
}


void QxtBlowFish::setPassword(QByteArray k )
{
    BF_set_key(key, k.count() , (unsigned char *)k.constData ());
}



QByteArray  QxtBlowFish::encrypt(QByteArray in)
{
    QByteArray out(in);
    int num =0;
    unsigned char  ivec [9];
    ivec[0]=(unsigned char )3887;
    ivec[1]=(unsigned char )3432;
    ivec[2]=(unsigned char )3887;
    ivec[3]=(unsigned char )2344;
    ivec[4]=(unsigned char )678;
    ivec[5]=(unsigned char )3887;
    ivec[6]=(unsigned char )575;
    ivec[7]=(unsigned char )2344;
    ivec[8]=(unsigned char )2222;


    BF_cfb64_encrypt(
        (unsigned char *)in.constData (),
        (unsigned char *)out.data(),
        in.size(),
        key,
        ivec,
        &num,
        BF_ENCRYPT
    );


    out=out.toBase64();
    return out;

}


QByteArray  QxtBlowFish::decrypt(QByteArray in)

{
    in=QByteArray::fromBase64(in);

    QByteArray out(in);

    int num =0;
    unsigned char  ivec [9];
    ivec[0]=(unsigned char )3887;
    ivec[1]=(unsigned char )3432;
    ivec[2]=(unsigned char )3887;
    ivec[3]=(unsigned char )2344;
    ivec[4]=(unsigned char )678;
    ivec[5]=(unsigned char )3887;
    ivec[6]=(unsigned char )575;
    ivec[7]=(unsigned char )2344;
    ivec[8]=(unsigned char )2222;


    BF_cfb64_encrypt(
        (unsigned char *)in.constData (),
        (unsigned char *)out.data(),
        in.size(),
        key,
        ivec,
        &num,
        BF_DECRYPT
    );


    return out;
}
