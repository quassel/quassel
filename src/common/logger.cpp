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

#include <QFile>
#include <QTextStream>
#include <QDateTime>

#ifdef HAVE_SYSLOG
#  include <syslog.h>
#endif

#include "logger.h"
#include "quassel.h"

Logger::~Logger()
{
    log();
}


void Logger::log()
{
    if (_logLevel < Quassel::logLevel())
        return;

    switch (_logLevel) {
    case Quassel::DebugLevel:
        _buffer.prepend("Debug: ");
        break;
    case Quassel::InfoLevel:
        _buffer.prepend("Info: ");
        break;
    case Quassel::WarningLevel:
        _buffer.prepend("Warning: ");
        break;
    case Quassel::ErrorLevel:
        _buffer.prepend("Error: ");
        break;
    default:
        break;
    }

#ifdef HAVE_SYSLOG
    if (Quassel::logToSyslog()) {
        int prio;
        switch (_logLevel) {
        case Quassel::DebugLevel:
            prio = LOG_DEBUG;
            break;
        case Quassel::InfoLevel:
            prio = LOG_INFO;
            break;
        case Quassel::WarningLevel:
            prio = LOG_WARNING;
            break;
        case Quassel::ErrorLevel:
            prio = LOG_ERR;
            break;
        default:
            prio = LOG_INFO;
            break;
        }
        syslog(prio|LOG_USER, "%s", qPrintable(_buffer));
    }
#endif

    // if we neither use syslog nor have a logfile we log to stdout

    if (Quassel::logFile() || !Quassel::logToSyslog()) {
        _buffer.prepend(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss "));

        QTextStream out(stdout);
        if (Quassel::logFile() && Quassel::logFile()->isOpen()) {
            _buffer.remove(QChar('\n'));
            out.setDevice(Quassel::logFile());
        }

        out << _buffer << endl;
    }
}


#if QT_VERSION < 0x050000
void Logger::logMessage(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        Logger(Quassel::DebugLevel) << msg;
        break;
    case QtWarningMsg:
        Logger(Quassel::WarningLevel) << msg;
        break;
    case QtCriticalMsg:
        Logger(Quassel::ErrorLevel) << msg;
        break;
    case QtFatalMsg:
        Logger(Quassel::ErrorLevel) << msg;
        Quassel::logFatalMessage(msg);
        return;
    }
}
#else
void Logger::logMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    switch (type) {
    case QtDebugMsg:
        Logger(Quassel::DebugLevel) << msg.toLocal8Bit().constData();
        break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
        Logger(Quassel::InfoLevel) << msg.toLocal8Bit().constData();
        break;
#endif
    case QtWarningMsg:
        Logger(Quassel::WarningLevel) << msg.toLocal8Bit().constData();
        break;
    case QtCriticalMsg:
        Logger(Quassel::ErrorLevel) << msg.toLocal8Bit().constData();
        break;
    case QtFatalMsg:
        Logger(Quassel::ErrorLevel) << msg.toLocal8Bit().constData();
        Quassel::logFatalMessage(msg.toLocal8Bit().constData());
        return;
    }
}
#endif
