/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "ircdecoder.h"

#include <QDebug>

#include "irctag.h"

QString IrcDecoder::parseTagValue(const QString& value)
{
    QString result;
    bool escaped = false;
    for (auto it = value.begin(); it < value.end(); it++) {
        // Check if it's on the list of special wildcard characters, converting to Unicode for use
        // in the switch statement
        //
        // See https://doc.qt.io/qt-5/qchar.html#unicode
        if (escaped) {
            switch (it->unicode()) {
            case '\\':
                result.append('\\');
                break;
            case 's':
                result.append(' ');
                break;
            case ':':
                result.append(';');
                break;
            case 'r':
                result.append('\r');
                break;
            case 'n':
                result.append('\n');
                break;
            default:
                result.append(*it);
            }
            escaped = false;
        } else if (it->unicode() == '\\') {
            escaped = true;
        } else {
            result.append(*it);
        }
    }
    return result;
}

/**
 * Extracts a space-delimited fragment from an IRC message
 * @param raw Raw Message
 * @param start Current index into the message, will be advanced automatically
 * @param end End of fragment, if already known. Default is -1, in which case it will be set to the next whitespace
 * character or the end of the string
 * @param prefix Required prefix. Default is 0. If set, this only parses a fragment if it starts with the given prefix.
 * @return Fragment
 */
QByteArray extractFragment(const QByteArray& raw, int& start, int end = -1, char prefix = 0)
{
    // Try to set find the end of the space-delimited fragment
    if (end == -1) {
        end = raw.indexOf(' ', start);
    }
    // If no space comes after this point, use the remainder of the string
    if (end == -1) {
        end = raw.length();
    }
    QByteArray fragment;
    // If a prefix is set
    if (prefix != 0) {
        // And the fragment starts with the prefix
        if (start < raw.length() && raw[start] == prefix) {
            // return the fragment without the prefix, advancing the string
            fragment = raw.mid(start + 1, end - start - 1);
            start = end;
        }
    }
    else {
        // otherwise return the entire fragment
        fragment = raw.mid(start, end - start);
        start = end;
    }
    return fragment;
}

/**
 * Skips empty parts in the message
 * @param raw Raw Message
 * @param start Current index into the message, will be advanced  automatically
 */
void skipEmptyParts(const QByteArray& raw, int& start)
{
    while (start < raw.length() && raw[start] == ' ') {
        start++;
    }
}

QHash<IrcTagKey, QString> IrcDecoder::parseTags(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start)
{
    QHash<IrcTagKey, QString> tags = {};
    QString rawTagStr = decode(extractFragment(raw, start, -1, '@'));
    // Tags are delimited with ; according to spec
    QList<QString> rawTags = rawTagStr.split(';');
    for (const QString& rawTag : rawTags) {
        if (rawTag.isEmpty()) {
            continue;
        }

        QString rawKey;
        QString rawValue;
        int index = rawTag.indexOf('=');
        if (index == -1 || index == rawTag.length()) {
            rawKey = rawTag;
        }
        else {
            rawKey = rawTag.left(index);
            rawValue = rawTag.mid(index + 1);
        }

        IrcTagKey key{};
        key.clientTag = rawKey.startsWith('+');
        if (key.clientTag) {
            rawKey.remove(0, 1);
        }
        QList<QString> splitByVendorAndKey = rawKey.split('/');
        if (!splitByVendorAndKey.isEmpty()) key.key = splitByVendorAndKey.takeLast();
        if (!splitByVendorAndKey.isEmpty()) key.vendor = splitByVendorAndKey.takeLast();
        tags[key] = parseTagValue(rawValue);
    }
    return tags;
}

QString IrcDecoder::parsePrefix(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start)
{
    return decode(extractFragment(raw, start, -1, ':'));
}

QString IrcDecoder::parseCommand(const std::function<QString(const QByteArray&)>& decode, const QByteArray& raw, int& start)
{
    return decode(extractFragment(raw, start, -1));
}

QByteArray IrcDecoder::parseParameter(const QByteArray& raw, int& start)
{
    if (start < raw.length() && raw[start] == ':') {
        // Skip the prefix
        start++;
        return extractFragment(raw, start, raw.size());
    }
    else {
        return extractFragment(raw, start);
    }
}

void IrcDecoder::parseMessage(const std::function<QString(const QByteArray&)>& decode, const QByteArray& rawMsg, QHash<IrcTagKey, QString>& tags, QString& prefix, QString& command, QList<QByteArray>& parameters)
{
    int start = 0;
    skipEmptyParts(rawMsg, start);
    tags = parseTags(decode, rawMsg, start);
    skipEmptyParts(rawMsg, start);
    prefix = parsePrefix(decode, rawMsg, start);
    skipEmptyParts(rawMsg, start);
    command = parseCommand(decode, rawMsg, start);
    skipEmptyParts(rawMsg, start);
    QList<QByteArray> params;
    while (start != rawMsg.length()) {
        QByteArray param = parseParameter(rawMsg, start);
        skipEmptyParts(rawMsg, start);
        params.append(param);

    }
    parameters = params;
}
