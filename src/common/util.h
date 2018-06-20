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

#pragma once

#include <QList>
#include <QString>
#include <QVariant>

QString nickFromMask(const QString &mask);
QString userFromMask(const QString &mask);
QString hostFromMask(const QString &mask);
bool isChannelName(const QString &str);

//! Strip mIRC format codes
QString stripFormatCodes(QString);

//! Remove accelerator markers (&) from the string
QString stripAcceleratorMarkers(const QString &);

QString secondsToString(int timeInSeconds);

//! Take a string and decode it using the specified text codec, recognizing utf8.
/** This function takes a string and first checks if it is encoded in utf8, in which case it is
 *  decoded appropriately. Otherwise, the specified text codec is used to transform the string.
 *  \param input The input string containing encoded data
 *  \param codec The text codec we use if the input is not utf8
 *  \return The decoded string.
 */
QString decodeString(const QByteArray &input, QTextCodec *codec = 0);

uint editingDistance(const QString &s1, const QString &s2);

template<typename T>
QVariantList toVariantList(const QList<T> &list)
{
    QVariantList variants;
    for (int i = 0; i < list.count(); i++) {
        variants << QVariant::fromValue<T>(list[i]);
    }
    return variants;
}


template<typename T>
QList<T> fromVariantList(const QVariantList &variants)
{
    QList<T> list;
    for (int i = 0; i < variants.count(); i++) {
        list << variants[i].value<T>();
    }
    return list;
}


QByteArray prettyDigest(const QByteArray &digest);

/**
 * Format a string with %%<text>%% to current date/timestamp via QDateTime.
 *
 * @param[in] formatStr String with format codes
 * @return String with current date/time substituted in via formatting codes
 */
QString formatCurrentDateTimeInString(const QString &formatStr);

/** Check if a scope rule matches a string
 *
 * When isRegEx is false:
 * Checks that the string does NOT match ANY inverted rules (prefixed by '!'), then checks that
 * it matches AT LEAST one normal (non-inverted) rule.
 *
 * If only inverted rules are specified, it'll match so long as the string does not match any
 * inverted rules (implicit wildcard).
 *
 * When isRegEx is true:
 * Checks that the string matches the entire scopeRule as a regular expression.  If scopeRule starts
 * with a '!', check that the string does NOT match the regular expression.
 *
 * @param string           String to test, e.g. network/channel name
 * @param scopeRule        ';'-separated list of wildcard expressions, prefix of '!' inverts subrule
 * @param isRegEx          If true, treat entire scope rule as regular expression, not wildcards
 * @param isCaseSensitive  If true, treat as case-sensitive, else case-insensitive
 * @return True if matches, otherwise false
 */
bool scopeMatch(const QString &string, const QString &scopeRule,
                const bool &isRegEx = false, const bool &isCaseSensitive = false);

/**
 * Try to localize a given date/time in seconds from Unix epoch, pass through string if invalid
 *
 * Allows compatibility with date/time fields that may or may not be in Unix epoch format,
 * localizing if possible, leaving alone if not.
 *
 * @param possibleEpochDate Date/time that might be in seconds since Unix epoch format
 * @return Localized date/time if parse succeeded, otherwise the source string
 */
QString tryFormatUnixEpoch(const QString &possibleEpochDate);
