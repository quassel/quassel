/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

QByteArray IrcEncoder::writeMessage(const QHash<IrcTagKey, QString>& tags,
                                    const QByteArray& prefix,
                                    const QString& cmd,
                                    const QList<QByteArray>& params)
{
    QByteArray msg;
    writeTags(msg, tags);
    writePrefix(msg, prefix);
    writeCommand(msg, cmd);
    writeParams(msg, params);
    return msg;
}

void IrcEncoder::writeTagValue(QByteArray& msg, const QString& value)
{
    QString it = value;
    msg += it.replace("\\", R"(\\)").replace(";", R"(\:)").replace(" ", R"(\s)").replace("\r", R"(\r)").replace("\n", R"(\n)").toUtf8();
}

void IrcEncoder::writeTags(QByteArray& msg, const QHash<IrcTagKey, QString>& tags)
{
    if (!tags.isEmpty()) {
        msg += "@";
        bool isFirstTag = true;
        for (const IrcTagKey& key : tags.keys()) {
            if (!isFirstTag) {
                // We join tags with semicolons
                msg += ";";
            }
            if (key.clientTag) {
                msg += "+";
            }
            if (!key.vendor.isEmpty()) {
                msg += key.vendor.toUtf8();
                msg += "/";
            }
            msg += key.key.toUtf8();
            if (!tags[key].isEmpty()) {
                msg += "=";
                writeTagValue(msg, tags[key]);
            }

            isFirstTag = false;
        }
        msg += " ";
    }
}

void IrcEncoder::writePrefix(QByteArray& msg, const QByteArray& prefix)
{
    if (!prefix.isEmpty()) {
        msg += ":" + prefix + " ";
    }
}

void IrcEncoder::writeCommand(QByteArray& msg, const QString& cmd)
{
    msg += cmd.toUpper().toLatin1();
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
