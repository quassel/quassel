/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef LOGGER_H
#define LOGGER_H

#include <QStringList>
#include <QTextStream>

#include "quassel.h"
#include "types.h"

class Logger
{
public:
    inline Logger(Quassel::LogLevel level) : _stream(&_buffer, QIODevice::WriteOnly), _logLevel(level) {}
    ~Logger();

    static void logMessage(QtMsgType type, const char *msg);

    template<typename T>
    inline Logger &operator<<(const T &value) { _stream << value << " "; return *this; }
    inline Logger &operator<<(const QStringList &t) { _stream << t.join(" ") << " "; return *this; }
    inline Logger &operator<<(bool t) { _stream << (t ? "true" : "false") << " "; return *this; }

private:
    void log();
    QTextStream _stream;
    QString _buffer;
    Quassel::LogLevel _logLevel;
};


class quInfo : public Logger
{
public:
    inline quInfo() : Logger(Quassel::InfoLevel) {}
};


class quWarning : public Logger
{
public:
    inline quWarning() : Logger(Quassel::WarningLevel) {}
};


class quError : public Logger
{
public:
    inline quError() : Logger(Quassel::ErrorLevel) {}
};


#endif
