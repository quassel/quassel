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

#include "common-export.h"

#include <QStringList>
#include <QTextStream>

#include "logger.h"
#include "types.h"

/**
 * Class encapsulating a single log message.
 *
 * Very similar in concept to qDebug() and friends.
 */
class COMMON_EXPORT LogMessage
{
public:
    LogMessage(Logger::LogLevel level);
    ~LogMessage();

    template<typename T>
    LogMessage& operator<<(const T& value)
    {
        _stream << value << " ";
        return *this;
    }

    LogMessage& operator<<(const QStringList& t);
    LogMessage& operator<<(bool t);

private:
    QTextStream _stream;
    QString _buffer;
    Logger::LogLevel _logLevel;
};

// The only reason for LogMessage and the helpers below to exist is the fact that Qt versions
// prior to 5.5 did not support the Info level.
// Once we can rely on Qt 5.5, they will be removed and replaced by the native Qt functions.

/**
 * Creates an info-level log message.
 *
 * @sa qInfo
 */
class quInfo : public LogMessage
{
public:
    quInfo()
        : LogMessage(Logger::LogLevel::Info)
    {}
};

/**
 * Creates a warning-level log message.
 *
 * @sa qWarning
 */
class quWarning : public LogMessage
{
public:
    quWarning()
        : LogMessage(Logger::LogLevel::Warning)
    {}
};

/**
 * Creates an error-level log message.
 *
 * @sa qCritical
 */
class quError : public LogMessage
{
public:
    quError()
        : LogMessage(Logger::LogLevel::Error)
    {}
};
