/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
#include <QTextCodec>
#include <QVector>

#include "quassel.h"

// MIBenum values from http://www.iana.org/assignments/character-sets/character-sets.xml#table-character-sets-1
static QList<int> utf8DetectionBlacklist = QList<int>()
    << 39 /* ISO-2022-JP */;

QString nickFromMask(const QString &mask)
{
    return mask.left(mask.indexOf('!'));
}


QString userFromMask(const QString &mask)
{
    const int offset = mask.indexOf('!') + 1;
    if (offset <= 0)
        return {};
    const int length = mask.indexOf('@', offset) - offset;
    return mask.mid(offset, length >= 0 ? length : -1);
}


QString hostFromMask(const QString &mask)
{
    const int excl = mask.indexOf('!');
    if (excl < 0)
        return {};
    const int offset = mask.indexOf('@', excl + 1) + 1;
    return offset > 0 && offset < mask.size() ? mask.mid(offset) : QString{};
}


bool isChannelName(const QString &str)
{
    static constexpr std::array<quint8, 4> prefixes{{'#', '&', '!', '+'}};
    return std::any_of(prefixes.cbegin(), prefixes.cend(), [&str](quint8 c) { return c == str[0]; });
}


QString stripFormatCodes(QString message)
{
    static QRegExp regEx{"\x03(\\d\\d?(,\\d\\d?)?)?|[\x02\x0f\x12\x16\x1d\x1f]"};
    return message.remove(regEx);
}


QString stripAcceleratorMarkers(const QString &label_)
{
    QString label = label_;
    int p = 0;
    forever {
        p = label.indexOf('&', p);
        if (p < 0 || p + 1 >= label.length())
            break;

        if (label.at(p + 1).isLetterOrNumber() || label.at(p + 1) == '&')
            label.remove(p, 1);

        ++p;
    }
    return label;
}


QString decodeString(const QByteArray &input, QTextCodec *codec)
{
    if (codec && utf8DetectionBlacklist.contains(codec->mibEnum()))
        return codec->toUnicode(input);

    // First, we check if it's utf8. It is very improbable to encounter a string that looks like
    // valid utf8, but in fact is not. This means that if the input string passes as valid utf8, it
    // is safe to assume that it is.
    // Q_ASSERT(sizeof(const char) == sizeof(quint8));  // In God we trust...
    bool isUtf8 = true;
    int cnt = 0;
    for (int i = 0; i < input.size(); i++) {
        if (cnt) {
            // We check a part of a multibyte char. These need to be of the form 10yyyyyy.
            if ((input[i] & 0xc0) != 0x80) { isUtf8 = false; break; }
            cnt--;
            continue;
        }
        if ((input[i] & 0x80) == 0x00) continue;  // 7 bit is always ok
        if ((input[i] & 0xf8) == 0xf0) { cnt = 3; continue; } // 4-byte char 11110xxx 10yyyyyy 10zzzzzz 10vvvvvv
        if ((input[i] & 0xf0) == 0xe0) { cnt = 2; continue; } // 3-byte char 1110xxxx 10yyyyyy 10zzzzzz
        if ((input[i] & 0xe0) == 0xc0) { cnt = 1; continue; } // 2-byte char 110xxxxx 10yyyyyy
        isUtf8 = false; break; // 8 bit char, but not utf8!
    }
    if (isUtf8 && cnt == 0) {
        QString s = QString::fromUtf8(input);
        //qDebug() << "Detected utf8:" << s;
        return s;
    }
    //QTextCodec *codec = QTextCodec::codecForName(encoding.toLatin1());
    if (!codec) return QString::fromLatin1(input);
    return codec->toUnicode(input);
}


uint editingDistance(const QString &s1, const QString &s2)
{
    uint n = s1.size()+1;
    uint m = s2.size()+1;
    QVector<QVector<uint> > matrix(n, QVector<uint>(m, 0));

    for (uint i = 0; i < n; i++)
        matrix[i][0] = i;

    for (uint i = 0; i < m; i++)
        matrix[0][i] = i;

    uint min;
    for (uint i = 1; i < n; i++) {
        for (uint j = 1; j < m; j++) {
            uint deleteChar = matrix[i-1][j] + 1;
            uint insertChar = matrix[i][j-1] + 1;

            if (deleteChar < insertChar)
                min = deleteChar;
            else
                min = insertChar;

            if (s1[i-1] == s2[j-1]) {
                uint inheritChar = matrix[i-1][j-1];
                if (inheritChar < min)
                    min = inheritChar;
            }

            matrix[i][j] = min;
        }
    }
    return matrix[n-1][m-1];
}


QString secondsToString(int timeInSeconds)
{
    static QVector<std::pair<int, QString>> timeUnit {
        std::make_pair(365*24*60*60, QCoreApplication::translate("Quassel::secondsToString()", "year")),
        std::make_pair(24*60*60, QCoreApplication::translate("Quassel::secondsToString()", "day")),
        std::make_pair(60*60, QCoreApplication::translate("Quassel::secondsToString()", "h")),
        std::make_pair(60, QCoreApplication::translate("Quassel::secondsToString()", "min")),
        std::make_pair(1, QCoreApplication::translate("Quassel::secondsToString()", "sec"))
    };

    if (timeInSeconds != 0) {
        QStringList returnString;
        for (int i = 0; i < timeUnit.size(); i++) {
            int n = timeInSeconds / timeUnit[i].first;
            if (n > 0) {
                returnString += QString("%1 %2").arg(QString::number(n), timeUnit[i].second);
            }
            timeInSeconds = timeInSeconds % timeUnit[i].first;
        }
        return returnString.join(", ");
    }
    else {
        return QString("%1 %2").arg(QString::number(timeInSeconds), timeUnit.last().second);
    }
}


QByteArray prettyDigest(const QByteArray &digest)
{
    QByteArray hexDigest = digest.toHex().toUpper();
    QByteArray prettyDigest;
    prettyDigest.fill(':', hexDigest.count() + (hexDigest.count() / 2) - 1);

    for (int i = 0; i * 2 < hexDigest.count(); i++) {
        prettyDigest.replace(i * 3, 2, hexDigest.mid(i * 2, 2));
    }
    return prettyDigest;
}


QString formatCurrentDateTimeInString(const QString &formatStr)
{
    // Work on a copy of the string to avoid modifying the input string
    QString formattedStr = QString(formatStr);

    // Exit early if there's nothing to format
    if (formattedStr.isEmpty())
        return formattedStr;

    // Find %%<text>%% in string. Replace inside text formatted to QDateTime with the current
    // timestamp, using %%%% as an escape for multiple %% signs.
    // For example:
    // Simple:   "All Quassel clients vanished from the face of the earth... %%hh:mm:ss%%"
    // > Result:  "All Quassel clients vanished from the face of the earth... 23:20:34"
    // Complex:  "Away since %%hh:mm%% on %%dd.MM%% - %%%% not here %%%%"
    // > Result:  "Away since 23:20 on 21.05 - %% not here %%"
    //
    // Match groups of double % signs - Some text %%inside here%%, and even %%%%:
    //   %%(.*)%%
    //   (...)    marks a capturing group
    //   .*       matches zero or more characters, not including newlines
    // Note that '\' must be escaped as '\\'
    // Helpful interactive website for debugging and explaining:  https://regex101.com/
    QRegExp regExpMatchTime("%%(.*)%%");

    // Preserve the smallest groups possible to allow for multiple %%blocks%%
    regExpMatchTime.setMinimal(true);

    // NOTE: Move regExpMatchTime to a static regular expression if used anywhere that performance
    // matters.

    // Don't allow a runaway regular expression to loop for too long.  This might not happen.. but
    // when dealing with user input, better to be safe..?
    int numIterations = 0;

    // Find each group of %%text here%% starting from the beginning
    int index = regExpMatchTime.indexIn(formattedStr);
    int matchLength;
    QString matchedFormat;
    while (index >= 0 && numIterations < 512) {
        // Get the total length of the matched expression
        matchLength = regExpMatchTime.cap(0).length();
        // Get the format string, e.g. "this text here" from "%%this text here%%"
        matchedFormat = regExpMatchTime.cap(1);
        // Check that there's actual characters inside.  A quadruple % (%%%%) represents two %%
        // signs.
        if (matchedFormat.length() > 0) {
            // Format the string according to the current date and time.  Invalid time format
            // strings are ignored.
            formattedStr.replace(index, matchLength,
                                 QDateTime::currentDateTime().toString(matchedFormat));
            // Subtract the length of the removed % signs
            // E.g. "%%h:mm ap%%" turns into "h:mm ap", removing four % signs, thus -4.  This is
            // used below to determine how far to advance when looking for the next formatting code.
            matchLength -= 4;
        } else if (matchLength == 4) {
            // Remove two of the four percent signs, so '%%%%' escapes to '%%'
            formattedStr.remove(index, 2);
            // Subtract the length of the removed % signs, this time removing two % signs, thus -2.
            matchLength -= 2;
        } else {
            // If neither of these match, something went wrong.  Don't modify it to be safe.
            qDebug() << "Unexpected time format when parsing string, no matchedFormat, matchLength "
                        "should be 4, actually is" << matchLength;
        }

        // Find the next group of %%text here%% starting from where the last group ended
        index = regExpMatchTime.indexIn(formattedStr, index + matchLength);
        numIterations++;
    }

    return formattedStr;
}
