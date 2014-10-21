/*
  This file has been derived from Konversation, the KDE IRC client.
  You can redistribute it and/or modify it under the terms of the
  GNU General Public License as published by the Free Software Foundation;
  either version 2 of the License, or (at your option) any later version.
*/

/*
  Copyright (C) 1997 Robey Pointer <robeypointer@gmail.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2009 Travis McHenry <tmchenryaz@cox.net>
  Copyright (C) 2009 Johannes Huber <johu@gmx.de>
*/

#include "cipher.h"
#include "logger.h"

Cipher::Cipher()
{
    m_primeNum = QCA::BigInteger("12745216229761186769575009943944198619149164746831579719941140425076456621824834322853258804883232842877311723249782818608677050956745409379781245497526069657222703636504651898833151008222772087491045206203033063108075098874712912417029101508315117935752962862335062591404043092163187352352197487303798807791605274487594646923");
    setType("blowfish");
}


Cipher::Cipher(QByteArray key, QString cipherType)
{
    m_primeNum = QCA::BigInteger("12745216229761186769575009943944198619149164746831579719941140425076456621824834322853258804883232842877311723249782818608677050956745409379781245497526069657222703636504651898833151008222772087491045206203033063108075098874712912417029101508315117935752962862335062591404043092163187352352197487303798807791605274487594646923");
    setKey(key);
    setType(cipherType);
}


Cipher::~Cipher()
{}

bool Cipher::setKey(QByteArray key)
{
    if (key.isEmpty()) {
        m_key.clear();
        return false;
    }

    if (key.mid(0, 4).toLower() == "ecb:")
    {
        m_cbc = false;
        m_key = key.mid(4);
    }
    //strip cbc: if included
    else if (key.mid(0, 4).toLower() == "cbc:")
    {
        m_cbc = true;
        m_key = key.mid(4);
    }
    else
    {
//    if(Preferences::self()->encryptionType())
//      m_cbc = true;
//    else
        m_cbc = false;
        m_key = key;
    }
    return true;
}


bool Cipher::setType(const QString &type)
{
    //TODO check QCA::isSupported()
    m_type = type;
    return true;
}


QByteArray Cipher::decrypt(QByteArray cipherText)
{
    QByteArray pfx = "";
    bool error = false; // used to flag non cbc, seems like good practice not to parse w/o regard for set encryption type

    //if we get cbc
    if (cipherText.mid(0, 5) == "+OK *")
    {
        //if we have cbc
        if (m_cbc)
            cipherText = cipherText.mid(5);
        //if we don't
        else
        {
            cipherText = cipherText.mid(5);
            pfx = "ERROR_NONECB: ";
            error = true;
        }
    }
    //if we get ecb
    else if (cipherText.mid(0, 4) == "+OK " || cipherText.mid(0, 5) == "mcps ")
    {
        //if we had cbc
        if (m_cbc)
        {
            cipherText = (cipherText.mid(0, 4) == "+OK ") ? cipherText.mid(4) : cipherText.mid(5);
            pfx = "ERROR_NONCBC: ";
            error = true;
        }
        //if we don't
        else
        {
            if (cipherText.mid(0, 4) == "+OK ")
                cipherText = cipherText.mid(4);
            else
                cipherText = cipherText.mid(5);
        }
    }
    //all other cases we fail
    else
        return cipherText;

    QByteArray temp;
    // (if cbc and no error we parse cbc) || (if ecb and error we parse cbc)
    if ((m_cbc && !error) || (!m_cbc && error))
    {
        cipherText = cipherText;
        temp = blowfishCBC(cipherText, false);

        if (temp == cipherText)
        {
            // kDebug("Decryption from CBC Failed");
            return cipherText+' '+'\n';
        }
        else
            cipherText = temp;
    }
    else
    {
        temp = blowfishECB(cipherText, false);

        if (temp == cipherText)
        {
            // kDebug("Decryption from ECB Failed");
            return cipherText+' '+'\n';
        }
        else
            cipherText = temp;
    }
    // TODO FIXME the proper fix for this is to show encryption differently e.g. [nick] instead of <nick>
    // don't hate me for the mircryption reference there.
    if (cipherText.at(0) == 1)
        pfx = "\x0";
    cipherText = pfx+cipherText+' '+'\n'; // FIXME(??) why is there an added space here?
    return cipherText;
}


QByteArray Cipher::initKeyExchange()
{
    QCA::Initializer init;
    m_tempKey = QCA::KeyGenerator().createDH(QCA::DLGroup(m_primeNum, QCA::BigInteger(2))).toDH();

    if (m_tempKey.isNull())
        return QByteArray();

    QByteArray publicKey = m_tempKey.toPublicKey().toDH().y().toArray().toByteArray();

    //remove leading 0
    if (publicKey.length() > 135 && publicKey.at(0) == '\0')
        publicKey = publicKey.mid(1);

    return publicKey.toBase64().append('A');
}


QByteArray Cipher::parseInitKeyX(QByteArray key)
{
    QCA::Initializer init;
    bool isCBC = false;

    if (key.endsWith(" CBC"))
    {
        isCBC = true;
        key.chop(4);
    }

    if (key.length() != 181)
        return QByteArray();

    QCA::SecureArray remoteKey = QByteArray::fromBase64(key.left(180));
    QCA::DLGroup group(m_primeNum, QCA::BigInteger(2));
    QCA::DHPrivateKey privateKey = QCA::KeyGenerator().createDH(group).toDH();

    if (privateKey.isNull())
        return QByteArray();

    QByteArray publicKey = privateKey.y().toArray().toByteArray();

    //remove leading 0
    if (publicKey.length() > 135 && publicKey.at(0) == '\0')
        publicKey = publicKey.mid(1);

    QCA::DHPublicKey remotePub(group, remoteKey);

    if (remotePub.isNull())
        return QByteArray();

    QByteArray sharedKey = privateKey.deriveKey(remotePub).toByteArray();
    sharedKey = QCA::Hash("sha256").hash(sharedKey).toByteArray().toBase64();

    //remove trailing = because mircryption and fish think it's a swell idea.
    while (sharedKey.endsWith('=')) sharedKey.chop(1);

    if (isCBC)
        sharedKey.prepend("cbc:");

    bool success = setKey(sharedKey);

    if (!success)
        return QByteArray();

    return publicKey.toBase64().append('A');
}


bool Cipher::parseFinishKeyX(QByteArray key)
{
    QCA::Initializer init;

    if (key.length() != 181)
        return false;

    QCA::SecureArray remoteKey = QByteArray::fromBase64(key.left(180));
    QCA::DLGroup group(m_primeNum, QCA::BigInteger(2));

    QCA::DHPublicKey remotePub(group, remoteKey);

    if (remotePub.isNull())
        return false;

    if (m_tempKey.isNull())
        return false;

    QByteArray sharedKey = m_tempKey.deriveKey(remotePub).toByteArray();
    sharedKey = QCA::Hash("sha256").hash(sharedKey).toByteArray().toBase64();

    //remove trailng = because mircryption and fish think it's a swell idea.
    while (sharedKey.endsWith('=')) sharedKey.chop(1);

    bool success = setKey(sharedKey);

    return success;
}


QByteArray Cipher::decryptTopic(QByteArray cipherText)
{
    if (cipherText.mid(0, 4) == "+OK ") // FiSH style topic
        cipherText = cipherText.mid(4);
    else if (cipherText.left(5) == "«m«")
        cipherText = cipherText.mid(5, cipherText.length()-10);
    else
        return cipherText;

    QByteArray temp;
    //TODO currently no backwards sanity checks for topic, it seems to use different standards
    //if somebody figures them out they can enable it and add "ERROR_NONECB/CBC" warnings
    if (m_cbc)
        temp = blowfishCBC(cipherText.mid(1), false);
    else
        temp = blowfishECB(cipherText, false);

    if (temp == cipherText)
    {
        return cipherText;
    }
    else
        cipherText = temp;

    if (cipherText.mid(0, 2) == "@@")
        cipherText = cipherText.mid(2);

    return cipherText;
}


bool Cipher::encrypt(QByteArray &cipherText)
{
    if (cipherText.left(3) == "+p ") //don't encode if...?
        cipherText = cipherText.mid(3);
    else
    {
        if (m_cbc) //encode in ecb or cbc decide how to determine later
        {
            QByteArray temp = blowfishCBC(cipherText, true);

            if (temp == cipherText)
            {
                // kDebug("CBC Encoding Failed");
                return false;
            }

            cipherText = "+OK *" + temp;
        }
        else
        {
            QByteArray temp = blowfishECB(cipherText, true);

            if (temp == cipherText)
            {
                // kDebug("ECB Encoding Failed");
                return false;
            }

            cipherText = "+OK " + temp;
        }
    }
    return true;
}


//THE BELOW WORKS AKA DO NOT TOUCH UNLESS YOU KNOW WHAT YOU'RE DOING
QByteArray Cipher::blowfishCBC(QByteArray cipherText, bool direction)
{
    QCA::Initializer init;
    QByteArray temp = cipherText;
    if (direction)
    {
        // make sure cipherText is an interval of 8 bits. We do this before so that we
        // know there's at least 8 bytes to en/decryption this ensures QCA doesn't fail
        while ((temp.length() % 8) != 0) temp.append('\0');

        QCA::InitializationVector iv(8);
        temp.prepend(iv.toByteArray()); // prefix with 8bits of IV for mircryptions *CUSTOM* cbc implementation
    }
    else
    {
        temp = QByteArray::fromBase64(temp);
        //supposedly nescessary if we get a truncated message also allows for decryption of 'crazy'
        //en/decoding clients that use STANDARDIZED PADDING TECHNIQUES
        while ((temp.length() % 8) != 0) temp.append('\0');
    }

    QCA::Direction dir = (direction) ? QCA::Encode : QCA::Decode;
    QCA::Cipher cipher(m_type, QCA::Cipher::CBC, QCA::Cipher::NoPadding, dir, m_key, QCA::InitializationVector(QByteArray("0")));
    QByteArray temp2 = cipher.update(QCA::MemoryRegion(temp)).toByteArray();
    temp2 += cipher.final().toByteArray();

    if (!cipher.ok())
        return cipherText;

    if (direction) //send in base64
        temp2 = temp2.toBase64();
    else //cut off the 8bits of IV
        temp2 = temp2.remove(0, 8);

    return temp2;
}


QByteArray Cipher::blowfishECB(QByteArray cipherText, bool direction)
{
    QCA::Initializer init;
    QByteArray temp = cipherText;

    //do padding ourselves
    if (direction)
    {
        while ((temp.length() % 8) != 0) temp.append('\0');
    }
    else
    {
        // ECB Blowfish encodes in blocks of 12 chars, so anything else is malformed input
        if ((temp.length() % 12) != 0)
            return cipherText;

        temp = b64ToByte(temp);
        while ((temp.length() % 8) != 0) temp.append('\0');
    }

    QCA::Direction dir = (direction) ? QCA::Encode : QCA::Decode;
    QCA::Cipher cipher(m_type, QCA::Cipher::ECB, QCA::Cipher::NoPadding, dir, m_key);
    QByteArray temp2 = cipher.update(QCA::MemoryRegion(temp)).toByteArray();
    temp2 += cipher.final().toByteArray();

    if (!cipher.ok())
        return cipherText;

    if (direction) {
        // Sanity check
        if ((temp2.length() % 8) != 0)
            return cipherText;

        temp2 = byteToB64(temp2);
    }

    return temp2;
}


//Custom non RFC 2045 compliant Base64 enc/dec code for mircryption / FiSH compatibility
QByteArray Cipher::byteToB64(QByteArray text)
{
    int left = 0;
    int right = 0;
    int k = -1;
    int v;
    QString base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray encoded;
    while (k < (text.length() - 1)) {
        k++;
        v = text.at(k); if (v < 0) v += 256;
        left = v << 24;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        left += v << 16;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        left += v << 8;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        left += v;

        k++;
        v = text.at(k); if (v < 0) v += 256;
        right = v << 24;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        right += v << 16;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        right += v << 8;
        k++;
        v = text.at(k); if (v < 0) v += 256;
        right += v;

        for (int i = 0; i < 6; i++) {
            encoded.append(base64.at(right & 0x3F).toAscii());
            right = right >> 6;
        }

        //TODO make sure the .toascii doesn't break anything
        for (int i = 0; i < 6; i++) {
            encoded.append(base64.at(left & 0x3F).toAscii());
            left = left >> 6;
        }
    }
    return encoded;
}


QByteArray Cipher::b64ToByte(QByteArray text)
{
    QString base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray decoded;
    int k = -1;
    while (k < (text.length() - 1)) {
        int right = 0;
        int left = 0;
        int v = 0;
        int w = 0;
        int z = 0;

        for (int i = 0; i < 6; i++) {
            k++;
            v = base64.indexOf(text.at(k));
            right |= v << (i * 6);
        }

        for (int i = 0; i < 6; i++) {
            k++;
            v = base64.indexOf(text.at(k));
            left |= v << (i * 6);
        }

        for (int i = 0; i < 4; i++) {
            w = ((left & (0xFF << ((3 - i) * 8))));
            z = w >> ((3 - i) * 8);
            if (z < 0) { z = z + 256; }
            decoded.append(z);
        }

        for (int i = 0; i < 4; i++) {
            w = ((right & (0xFF << ((3 - i) * 8))));
            z = w >> ((3 - i) * 8);
            if (z < 0) { z = z + 256; }
            decoded.append(z);
        }
    }
    return decoded;
}


bool Cipher::neededFeaturesAvailable()
{
    QCA::Initializer init;

    if (QCA::isSupported("blowfish-ecb") && QCA::isSupported("blowfish-cbc") && QCA::isSupported("dh"))
        return true;

    return false;
}
