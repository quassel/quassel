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

#pragma once

#include "common-export.h"

#include <functional>

#include "irctag.h"

class COMMON_EXPORT IrcDecoder
{
public:
    /**
     * Parses an IRC message
     * @param decode Decoder to be used for decoding the message
     * @param rawMsg Raw Message
     * @param tags[out] Parsed map of IRCv3 message tags
     * @param prefix[out] Parsed prefix
     * @param command[out] Parsed command
     * @param parameters[out] Parsed list of parameters
     */
    static void parseMessage(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, QHash<IrcTagKey, QString>& tags, QString& prefix, QString& command, QList<QByteArray>& parameters);

    /**
     * Extracts a space-delimited fragment from an IRC message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced automatically
     * @param end End of fragment, if already known. Default is -1, in which case it will be set to the next whitespace
     * character or the end of the string
     * @param prefix Required prefix. Default is 0. If set, this only parses a fragment if it starts with the given prefix.
     * @return Fragment
     */
    static QByteArray extractFragment(const QByteArray& raw, int& start, int end = -1, char prefix = 0);

    /**
     * Skips empty parts in the message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced  automatically
     */
    static void skipEmptyParts(const QByteArray& raw, int& start);
private:
    /**
     * Parses an encoded IRCv3 message tag value
     * @param value encoded IRCv3 message tag value
     * @return decoded string
     */
    static QString parseTagValue(const QString& value);
    /**
     * Parses IRCv3 message tags given a message
     * @param net Decoder to be used for decoding the message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced automatically
     * @return Parsed message tags
     */
    static QHash<IrcTagKey, QString> parseTags(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start);
    /**
     * Parses an IRC prefix, if available
     * @param net Decoder to be used for decoding the message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced automatically
     * @return Parsed prefix or empty string
     */
    static QString parsePrefix(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start);
    /**
     * Parses an IRC named command or numeric RPL
     * @param net Decoder to be used for decoding the message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced automatically
     * @return Parsed command
     */
    static QString parseCommand(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start);
    /**
     * Parses an IRC parameter
     * @param net Decoder to be used for decoding the message
     * @param raw Raw Message
     * @param start Current index into the message, will be advanced automatically
     * @return Parsed parameter
     */
    static QByteArray parseParameter(const QByteArray& raw, int& start);
};
