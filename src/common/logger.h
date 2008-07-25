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

    inline Logger(LogLevel level) : _stream(new Stream(level)) {}
    ~Logger();

    inline Logger &operator<<(const char* t) { _stream->internalStream << QString::fromAscii(t); return *this; }
    inline Logger &operator<<(QChar t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(bool t) { _stream->internalStream << (t ? "true" : "false"); return *this; }
    inline Logger &operator<<(char t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed short t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned short t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed int t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned int t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed long t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned long t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(qint64 t) { _stream->internalStream << QString::number(t); return *this; }
    inline Logger &operator<<(quint64 t) { _stream->internalStream << QString::number(t); return *this; }
    inline Logger &operator<<(float t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(double t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QString & t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QLatin1String &t) { _stream->internalStream << t.latin1(); return *this; }
    inline Logger &operator<<(const QByteArray & t) { _stream->internalStream << t ; return *this; }
    inline Logger &operator<<(const void * t) { _stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QStringList & t) { _stream->internalStream << t.join(" "); return *this; }
    inline Logger &operator<<(const BufferId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const NetworkId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const UserId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const MsgId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const IdentityId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const AccountId & t) { _stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }

    void log();
  private:
    struct Stream {
      Stream(LogLevel level)
      : internalStream(&buffer, QIODevice::WriteOnly),
        logLevel(level) {}
      QTextStream internalStream;
      QString buffer;
      LogLevel logLevel;
    } *_stream;
};

inline Logger quDebug() { return Logger(Logger::DebugLevel); }
inline Logger quInfo() { return Logger(Logger::InfoLevel); }
inline Logger quWarning() { return Logger(Logger::WarningLevel); }
inline Logger quError() { return Logger(Logger::ErrorLevel); }

#endif
