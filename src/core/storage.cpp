/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "storage.h"

#include <QCryptographicHash>

Storage::Storage(QObject *parent)
    : QObject(parent)
{
}

QString Storage::hashPassword(const QString &password)
{
#if QT_VERSION >= 0x050000
    return hashPasswordSha2_512(password);
#else
    return hashPasswordSha1(password);
#endif
}

bool Storage::checkHashedPassword(const UserId user, const QString &password, const QString &hashedPassword, const Storage::HashVersion version)
{
    bool passwordCorrect = false;
    
    switch (version) {
    case Storage::HashVersion::sha1:
        passwordCorrect = checkHashedPasswordSha1(password, hashedPassword);
        break;

#if QT_VERSION >= 0x050000
    case Storage::HashVersion::sha2_512:
        passwordCorrect = checkHashedPasswordSha2_512(password, hashedPassword);
        break;
#endif

    default:
        qWarning() << "Password hash version" << QString(version) << "is not supported, please reset password";
    }
    
    if (passwordCorrect && version < Storage::HashVersion::latest) {
        updateUser(user, password);
    }
    
    return passwordCorrect;
}

QString Storage::hashPasswordSha1(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1).toHex());
}

bool Storage::checkHashedPasswordSha1(const QString &password, const QString &hashedPassword)
{
    return hashPasswordSha1(password) == hashedPassword;
}

#if QT_VERSION >= 0x050000
QString Storage::hashPasswordSha2_512(const QString &password)
{
    // Generate a salt of 512 bits (64 bytes) using the Mersenne Twister
    std::random_device seed;
    std::mt19937 generator(seed());
    std::uniform_int_distribution<int> distribution(0, 255);
    QByteArray saltBytes;
    saltBytes.resize(64);
    for (int i = 0; i < 64; i++) {
        saltBytes[i] = (unsigned char) distribution(generator);
    }
    QString salt(saltBytes.toHex());

    // Append the salt to the password and hash it
    QString passwordAndSalt(password + salt);
    QString hash(QCryptographicHash::hash(passwordAndSalt.toUtf8(), QCryptographicHash::Sha512).toHex());

    return hash + ":" + salt;
}

bool Storage::checkHashedPasswordSha2_512(const QString &password, const QString &hashedPassword)
{
    QRegExp colonSplitter("\\:");
    QStringList hashedPasswordAndSalt = hashedPassword.split(colonSplitter);

    if (hashedPasswordAndSalt.size() == 2){
        QString passwordAndSalt(password + hashedPasswordAndSalt[1]);
        return QString(QCryptographicHash::hash(passwordAndSalt.toUtf8(), QCryptographicHash::Sha512).toHex()) == hashedPasswordAndSalt[0];
    }
    else {
        qWarning() << "Password hash and salt were not in the correct format";
        return false;
    }
}
#endif
