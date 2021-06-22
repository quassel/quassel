/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "ircencoder.h"

QByteArray IrcEncoder::writeMessage(const std::function<QByteArray(const QString&)>& encode, const IrcMessage& message)
{
    QByteArray msg;
    writeTags(msg, encode, message.tags);
    writePrefix(msg, encode, message.prefix);
    writeCommand(msg, encode, message.cmd);
    writeParams(msg, message.params);
    return msg;
}

QString IrcEncoder::writeTagValue(const QString& value)
{
    QString it = value;
    return it.replace("\\", R"(\\)")
             .replace(";", R"(\:)")
             .replace(" ", R"(\s)")
             .replace("\r", R"(\r)")
             .replace("\n", R"(\n)");
}

void IrcEncoder::writeTags(QByteArray& msg, const std::function<QByteArray(const QString&)>& encode, const QHash<IrcTagKey, QString>& tags)
{
    if (!tags.isEmpty()) {
        msg += encode("@");
        bool isFirstTag = true;
        for (auto key = tags.keyBegin(), end = tags.keyEnd(); key != end; ++key) {
            if (!isFirstTag) {
                // We join tags with semicolons
                msg += encode(";");
            }
            if (key->clientTag) {
                msg += encode("+");
            }
            if (!key->vendor.isEmpty()) {
                msg += encode(key->vendor);
                msg += encode("/");
            }
            msg += key->key;
            if (!tags[*key].isEmpty()) {
                msg += encode("=");
                msg += encode(writeTagValue(tags[*key]));
            }

            isFirstTag = false;
        }
        msg += encode(" ");
    }
}

void IrcEncoder::writePrefix(QByteArray& msg, const std::function<QByteArray(const QString&)>& encode, const QString& prefix)
{
    if (!prefix.isEmpty()) {
        msg += ":" + encode(prefix) + " ";
    }
}

void IrcEncoder::writeCommand(QByteArray& msg, const std::function<QByteArray(const QString&)>& encode, const QString& cmd)
{
    msg += encode(cmd.toUpper());
}

void IrcEncoder::writeParams(QByteArray& msg, const QList<QByteArray>& params)
{
    for (int i = 0; i < params.size(); i++) {
        msg += " ";

        bool isLastParam = i == params.size() - 1;
        if (isLastParam && (params[i].isEmpty() || params[i].contains(' ') || params[i][0] == ':'))
            msg += ":";

        msg += params[i];
    }
}
