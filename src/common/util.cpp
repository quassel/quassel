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

#include "util.h"

#include <algorithm>
#include <array>
#include <utility>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTimeZone>
#include <QVector>

#include "quassel.h"

// Encoding values from http://www.iana.org/assignments/character-sets/character-sets.xml#table-character-sets-1
static QList<QStringConverter::Encoding> utf8DetectionBlacklist = {QStringConverter::Utf16, QStringConverter::Utf32};

QString nickFromMask(const QString& mask)
{
    return mask.left(mask.indexOf('!'));
}

QString userFromMask(const QString& mask)
{
    const int offset = mask.indexOf('!') + 1;
    if (offset <= 0)
        return {};
    const int length = mask.indexOf('@', offset) - offset;
    return mask.mid(offset, length >= 0 ? length : -1);
}

QString hostFromMask(const QString& mask)
{
    const int excl = mask.indexOf('!');
    if (excl < 0)
        return {};
    const int offset = mask.indexOf('@', excl + 1) + 1;
    return offset > 0 && offset < mask.size() ? mask.mid(offset) : QString{};
}

bool isChannelName(const QString& str)
{
    if (str.isEmpty())
        return false;
    static constexpr std::array<quint8, 4> prefixes{{'#', '&', '!', '+'}};
    return std::any_of(prefixes.cbegin(), prefixes.cend(), [&str](quint8 c) { return QChar(c) == str[0]; });
}

QString stripFormatCodes(QString message)
{
    static QRegularExpression regEx("\x03(\\d\\d?(,\\d\\d?)?)?|\x04([\\da-fA-F]{6}(,[\\da-fA-F]{6})?)?|[\x02\x0f\x11\x12\x16\x1d\x1e\x1f]");
    return message.remove(regEx);
}

QString stripAcceleratorMarkers(const QString& label_)
{
    QString label = label_;
    int p = 0;
    while (true) {
        p = label.indexOf('&', p);
        if (p < 0 || p + 1 >= label.length())
            break;

        if (label.at(p + 1).isLetterOrNumber() || label.at(p + 1) == '&')
            label.remove(p, 1);

        ++p;
    }
    return label;
}

QString decodeString(const QByteArray& input, QStringConverter::Encoding encoding)
{
    if (encoding == QStringConverter::Utf16 || encoding == QStringConverter::Utf32) {
        QStringDecoder decoder(encoding);
        return decoder.decode(input);
    }

    // First, we check if it's utf8. It is very improbable to encounter a string that looks like
    // valid utf8, but in fact is not. This means that if the input string passes as valid utf8, it
    // is safe to assume that it is.
    bool isUtf8 = true;
    int cnt = 0;
    for (uchar c : input) {
        if (cnt) {
            // We check a part of a multibyte char. These need to be of the form 10yyyyyy.
            if ((c & 0xc0) != 0x80) {
                isUtf8 = false;
                break;
            }
            cnt--;
            continue;
        }
        if ((c & 0x80) == 0x00)
            continue;  // 7 bit is always ok
        if ((c & 0xf8) == 0xf0) {
            cnt = 3;
            continue;
        }  // 4-byte char 11110xxx 10yyyyyy 10zzzzzz 10vvvvvv
        if ((c & 0xf0) == 0xe0) {
            cnt = 2;
            continue;
        }  // 3-byte char 1110xxxx 10yyyyyy 10zzzzzz
        if ((c & 0xe0) == 0xc0) {
            cnt = 1;
            continue;
        }  // 2-byte char 110xxxxx 10yyyyyy
        isUtf8 = false;
        break;  // 8 bit char, but not utf8!
    }
    if (isUtf8 && cnt == 0) {
        QStringDecoder decoder(QStringConverter::Utf8);
        return decoder.decode(input);
    }
    QStringDecoder decoder(encoding);
    return decoder.decode(input);
}

uint editingDistance(const QString& s1, const QString& s2)
{
    uint n = s1.size() + 1;
    uint m = s2.size() + 1;
    QVector<QVector<uint>> matrix(n, QVector<uint>(m, 0));

    for (uint i = 0; i < n; i++)
        matrix[i][0] = i;

    for (uint i = 0; m > 0 && i < m; i++)
        matrix[0][i] = i;

    uint min;
    for (uint i = 1; i < n; i++) {
        for (uint j = 1; j < m; j++) {
            uint deleteChar = matrix[i - 1][j] + 1;
            uint insertChar = matrix[i][j - 1] + 1;

            if (deleteChar < insertChar)
                min = deleteChar;
            else
                min = insertChar;

            if (s1[i - 1] == s2[j - 1]) {
                uint inheritChar = matrix[i - 1][j - 1];
                if (inheritChar < min)
                    min = inheritChar;
            }

            matrix[i][j] = min;
        }
    }
    return matrix[n - 1][m - 1];
}

QString secondsToString(int timeInSeconds)
{
    static QVector<std::pair<int, QString>> timeUnit{{365 * 24 * 60 * 60, QCoreApplication::translate("Quassel::secondsToString()", "year")},
                                                     {24 * 60 * 60, QCoreApplication::translate("Quassel::secondsToString()", "day")},
                                                     {60 * 60, QCoreApplication::translate("Quassel::secondsToString()", "h")},
                                                     {60, QCoreApplication::translate("Quassel::secondsToString()", "min")},
                                                     {1, QCoreApplication::translate("Quassel::secondsToString()", "sec")}};

    if (timeInSeconds != 0) {
        QStringList returnString;
        for (const auto& tu : timeUnit) {
            int n = timeInSeconds / tu.first;
            if (n > 0) {
                returnString += QString("%1 %2").arg(QString::number(n), tu.second);
            }
            timeInSeconds = timeInSeconds % tu.first;
        }
        return returnString.join(", ");
    }

    return QString("%1 %2").arg(QString::number(timeInSeconds), timeUnit.back().second);
}

QByteArray prettyDigest(const QByteArray& digest)
{
    QByteArray hexDigest = digest.toHex().toUpper();
    QByteArray prettyDigest;
    prettyDigest.fill(':', hexDigest.size() + (hexDigest.size() / 2) - 1);

    for (int i = 0; i * 2 < hexDigest.size(); i++) {
        prettyDigest.replace(i * 3, 2, hexDigest.mid(i * 2, 2));
    }
    return prettyDigest;
}

QString formatCurrentDateTimeInString(const QString& formatStr)
{
    // Work on a copy of the string to avoid modifying the input string
    QString formattedStr = formatStr;

    // Exit early if there's nothing to format
    if (formattedStr.isEmpty())
        return formattedStr;

    // Find %%<text>%% in string. Replace inside text formatted to QDateTime with the current
    // timestamp, using %%%% as an escape for multiple %% signs.
    static QRegularExpression regExpMatchTime("%%(.*?)%%", QRegularExpression::UseUnicodePropertiesOption);

    // Don't allow a runaway regular expression to loop for too long.
    int numIterations = 0;

    // Find each group of %%text here%% starting from the beginning
    QRegularExpressionMatchIterator it = regExpMatchTime.globalMatch(formattedStr);
    while (it.hasNext() && numIterations < 512) {
        QRegularExpressionMatch match = it.next();
        int index = match.capturedStart();
        // Get the total length of the matched expression
        int matchLength = match.captured(0).length();
        // Get the format string, e.g. "this text here" from "%%this text here%%"
        QString matchedFormat = match.captured(1);
        // Check that there's actual characters inside
        if (matchedFormat.length() > 0) {
            // Format the string according to the current date and time. Invalid time format
            // strings are ignored.
            formattedStr.replace(index, matchLength, QDateTime::currentDateTime().toString(matchedFormat));
            // Subtract the length of the removed % signs
            matchLength -= 4;
        }
        else if (matchLength == 4) {
            // Remove two of the four percent signs, so '%%%%' escapes to '%%'
            formattedStr.remove(index, 2);
            // Subtract the length of the removed % signs
            matchLength -= 2;
        }
        else {
            // If neither of these match, something went wrong. Don't modify it to be safe.
            qDebug() << "Unexpected time format when parsing string, no matchedFormat, matchLength "
                        "should be 4, actually is"
                     << matchLength;
        }
        numIterations++;
    }

    return formattedStr;
}

QString tryFormatUnixEpoch(const QString& possibleEpochDate, Qt::DateFormat dateFormat, bool useUTC)
{
    // Does the string resemble a Unix epoch? Parse as 64-bit time
    qint64 secsSinceEpoch = possibleEpochDate.toLongLong();
    if (secsSinceEpoch == 0) {
        // Parsing either failed, or '0' was sent. No need to distinguish; either way, it's not
        // useful as epoch.
        return possibleEpochDate;
    }

    // Time checks out, parse it
    QDateTime date;
    date.setSecsSinceEpoch(secsSinceEpoch);

    // Return the localized date/time
    if (useUTC) {
        // Return UTC time
        if (dateFormat == Qt::DateFormat::ISODate) {
            // Replace the "T" date/time separator with " " for readability.
            return date.toUTC().toString(dateFormat).replace(10, 1, " ");
        }
        else {
            return date.toUTC().toString(dateFormat);
        }
    }
    else if (dateFormat == Qt::DateFormat::ISODate) {
        // Add in ISO local timezone information
        return formatDateTimeToOffsetISO(date);
    }
    else {
        // Return local time
        return date.toString(dateFormat);
    }
}

QString formatDateTimeToOffsetISO(const QDateTime& dateTime)
{
    if (!dateTime.isValid()) {
        // Don't try to do anything with invalid date/time
        return "formatDateTimeToISO() invalid date/time";
    }

    // Replace the "T" date/time separator with " " for readability.
    return dateTime.toOffsetFromUtc(dateTime.offsetFromUtc()).toString(Qt::ISODate).replace(10, 1, " ");
}
