/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <random>

#include <QCryptographicHash>

Storage::Storage(QObject *parent)
    : QObject(parent)
{
}

QString Storage::hashPassword(const QString &password)
{
    return hashPasswordSha2_512(password);
}

bool Storage::checkHashedPassword(const UserId user, const QString &password, const QString &hashedPassword, const Storage::HashVersion version)
{
    bool passwordCorrect = false;

    switch (version) {
    case Storage::HashVersion::Sha1:
        passwordCorrect = checkHashedPasswordSha1(password, hashedPassword);
        break;

    case Storage::HashVersion::Sha2_512:
        passwordCorrect = checkHashedPasswordSha2_512(password, hashedPassword);
        break;

    default:
        qWarning() << "Password hash version" << QString(version) << "is not supported, please reset password";
    }

    if (passwordCorrect && version < Storage::HashVersion::Latest) {
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

    // Append the salt to the password, hash the result, and append the salt value
    return sha2_512(password + salt) + ":" + salt;
}

bool Storage::checkHashedPasswordSha2_512(const QString &password, const QString &hashedPassword)
{
    QRegExp colonSplitter("\\:");
    QStringList hashedPasswordAndSalt = hashedPassword.split(colonSplitter);

    if (hashedPasswordAndSalt.size() == 2){
        return sha2_512(password + hashedPasswordAndSalt[1]) == hashedPasswordAndSalt[0];
    }
    else {
        qWarning() << "Password hash and salt were not in the correct format";
        return false;
    }
}

QString Storage::sha2_512(const QString &input)
{
    return QString(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha512).toHex());
}
