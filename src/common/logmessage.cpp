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

#include "logmessage.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include "quassel.h"

LogMessage::LogMessage(Logger::LogLevel level)
    : _stream(&_buffer, QIODevice::WriteOnly)
    , _logLevel(level)
{
}


LogMessage::~LogMessage()
{
    Quassel::instance()->logger()->handleMessage(_logLevel, _buffer);
}


LogMessage &LogMessage::operator<<(const QStringList &t)
{
    _stream << t.join(" ") << " ";
    return *this;
}


LogMessage &LogMessage::operator<<(bool t) {
    _stream << (t ? "true" : "false") << " ";
    return *this;
}
