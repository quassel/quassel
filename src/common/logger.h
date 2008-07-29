/***************************************************************************
 *   Copyright (C) 2005 by the Quassel Project                             *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include "types.h"

#include <QString>
#include <QStringList>
#include <QTextStream>

class Logger {
  public:
    enum LogLevel {
      DebugLevel,
      InfoLevel,
      WarningLevel,
      ErrorLevel
    };

    inline Logger(LogLevel level) : _stream(&_buffer, QIODevice::WriteOnly), _logLevel(level) {}
    ~Logger();

    template<typename T>
    inline Logger &operator<<(const T &value) { _stream << value << " "; return *this; }
    inline Logger &operator<<(const QStringList & t) { _stream << t.join(" ") << " "; return *this; }
    inline Logger &operator<<(bool t) { _stream << (t ? "true" : "false") << " "; return *this; }

  private:
    void log();
    QTextStream _stream;
    QString _buffer;
    LogLevel _logLevel;
};

class quDebug : public Logger {
  public:
    inline quDebug() : Logger(Logger::DebugLevel) {}
};

class quInfo : public Logger {
  public:
    inline quInfo() : Logger(Logger::InfoLevel) {}
};

class quWarning : public Logger {
  public:
    inline quWarning() : Logger(Logger::WarningLevel) {}
};

class quError : public Logger {
  public:
    inline quError() : Logger(Logger::ErrorLevel) {}
};
#endif
