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

#ifndef _LOGGER_H_
#define _LOGGER_H_

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

    inline Logger(LogLevel level) : stream(new Stream(level)) {}
    ~Logger();

    inline Logger &operator<<(const char* t) { stream->internalStream << QString::fromAscii(t); return *this; }
    inline Logger &operator<<(QChar t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(bool t) { stream->internalStream << (t ? "true" : "false"); return *this; }
    inline Logger &operator<<(char t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed short t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned short t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed int t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned int t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(signed long t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(unsigned long t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(qint64 t) { stream->internalStream << QString::number(t); return *this; }
    inline Logger &operator<<(quint64 t) { stream->internalStream << QString::number(t); return *this; }
    inline Logger &operator<<(float t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(double t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QString & t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QLatin1String &t) { stream->internalStream << t.latin1(); return *this; }
    inline Logger &operator<<(const QByteArray & t) { stream->internalStream << t ; return *this; }
    inline Logger &operator<<(const void * t) { stream->internalStream << t; return *this; }
    inline Logger &operator<<(const QStringList & t) { stream->internalStream << t.join(" "); return *this; }
    inline Logger &operator<<(const BufferId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const NetworkId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const UserId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const MsgId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const IdentityId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }
    inline Logger &operator<<(const AccountId & t) { stream->internalStream << QVariant::fromValue(t).toInt(); return *this; }

    void log();
  private:
    struct Stream {
      Stream(LogLevel level)
      : internalStream(&buffer, QIODevice::WriteOnly),
        logLevel(level) {}
      QTextStream internalStream;
      QString buffer;
      LogLevel logLevel;
    } *stream;
};

inline Logger quDebug() { return Logger(Logger::DebugLevel); }
inline Logger quInfo() { return Logger(Logger::InfoLevel); }
inline Logger quWarning() { return Logger(Logger::WarningLevel); }
inline Logger quError() { return Logger(Logger::ErrorLevel); }

#endif
