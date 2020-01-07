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

#pragma once

#include "common-export.h"

#include <QList>
#include <QString>
#include <QVariant>

COMMON_EXPORT QString nickFromMask(const QString& mask);
COMMON_EXPORT QString userFromMask(const QString& mask);
COMMON_EXPORT QString hostFromMask(const QString& mask);
COMMON_EXPORT bool isChannelName(const QString& str);

//! Strip mIRC format codes
COMMON_EXPORT QString stripFormatCodes(QString);

//! Remove accelerator markers (&) from the string
COMMON_EXPORT QString stripAcceleratorMarkers(const QString&);

COMMON_EXPORT QString secondsToString(int timeInSeconds);

//! Take a string and decode it using the specified text codec, recognizing utf8.
/** This function takes a string and first checks if it is encoded in utf8, in which case it is
 *  decoded appropriately. Otherwise, the specified text codec is used to transform the string.
 *  \param input The input string containing encoded data
 *  \param codec The text codec we use if the input is not utf8
 *  \return The decoded string.
 */
COMMON_EXPORT QString decodeString(const QByteArray& input, QTextCodec* codec = nullptr);

COMMON_EXPORT uint editingDistance(const QString& s1, const QString& s2);

template<typename T>
QVariantList toVariantList(const QList<T>& list)
{
    QVariantList variants;
    for (int i = 0; i < list.count(); i++) {
        variants << QVariant::fromValue(list[i]);
    }
    return variants;
}

template<typename T>
QList<T> fromVariantList(const QVariantList& variants)
{
    QList<T> list;
    for (int i = 0; i < variants.count(); i++) {
        list << variants[i].value<T>();
    }
    return list;
}

COMMON_EXPORT QByteArray prettyDigest(const QByteArray& digest);

/**
 * Format a string with %%<text>%% to current date/timestamp via QDateTime.
 *
 * @param[in] formatStr String with format codes
 * @return String with current date/time substituted in via formatting codes
 */
COMMON_EXPORT QString formatCurrentDateTimeInString(const QString& formatStr);

/**
 * Try to localize a given date/time in seconds from Unix epoch, pass through string if invalid
 *
 * Allows compatibility with date/time fields that may or may not be in Unix epoch format,
 * localizing if possible, leaving alone if not.
 *
 * @param possibleEpochDate Date/time that might be in seconds since Unix epoch format
 * @param dateFormat        Desired format of the date/time string
 * @param useUTC            If true, use UTC timezone, otherwise use local time
 * @return Localized date/time if parse succeeded, otherwise the source string
 */
COMMON_EXPORT QString tryFormatUnixEpoch(const QString& possibleEpochDate,
                                         Qt::DateFormat dateFormat = Qt::DateFormat::TextDate,
                                         bool useUTC = false);

/**
 * Format the given date/time in ISO 8601 format with timezone offset
 *
 * @param dateTime Date/time of interest
 * @return Date/time in ISO 8601 format with timezone offset
 */
COMMON_EXPORT QString formatDateTimeToOffsetISO(const QDateTime& dateTime);

namespace detail {

template<typename... Args>
struct SelectOverloadHelper
{
    template<typename R, typename C>
    constexpr auto operator()(R (C::*func)(Args...)) const noexcept -> decltype(func)
    {
        return func;
    }
};

}  // namespace detail

/**
 * Helper for resolving ambiguous overloads when using the member function-based connect syntax.
 *
 * Example usage:
 * @code
 * connect(this, selectOverload<int, QString>(&MyClass::mySignal), other, &Other::mySlot);
 * @endcode
 *
 * @tparam Args Argument types of the desired signature
 */
template<typename... Args>
constexpr Q_DECL_UNUSED detail::SelectOverloadHelper<Args...> selectOverload = {};
