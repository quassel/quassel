/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
  Copyright (C) 1997 Robey Pointer <robeypointer@gmail.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2009 Travis McHenry <tmchenryaz@cox.net>
  Copyright (C) 2009 Johannes Huber <johu@gmx.de>
*/

#ifndef CIPHER_H
#define CIPHER_H

#include<QtCrypto>

class Cipher
{
  public:
    Cipher();
    explicit Cipher(QByteArray key, QString cipherType=QString("blowfish"));
    ~Cipher();
    QByteArray decrypt(QByteArray cipher);
    QByteArray decryptTopic(QByteArray cipher);
    bool encrypt(QByteArray& cipher);
    QByteArray initKeyExchange();
    QByteArray parseInitKeyX(QByteArray key);
    bool parseFinishKeyX(QByteArray key);
    bool setKey(QByteArray key);
    QByteArray key() { return m_key; }
    bool setType(const QString &type);
    QString type() { return m_type; }
    
  private:
    //direction is true for encrypt, false for decrypt
    QByteArray blowfishCBC(QByteArray cipherText, bool direction);
    QByteArray blowfishECB(QByteArray cipherText, bool direction);
    QByteArray b64ToByte(QByteArray text);
    QByteArray byteToB64(QByteArray text);

    QCA::Initializer init;
    QByteArray m_key;
    QCA::DHPrivateKey m_tempKey;
    QCA::BigInteger m_primeNum;
    QString m_type;
    bool m_cbc;
};
#endif // CIPHER_H