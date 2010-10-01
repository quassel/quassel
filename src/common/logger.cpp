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

#include "logger.h"
#include "quassel.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#endif

Logger::~Logger() {
  QDateTime date = QDateTime::currentDateTime();

  switch (_logLevel) {
    case DebugLevel:
      _buffer.prepend("Debug: ");
      break;
    case InfoLevel:
      _buffer.prepend("Info: ");
      break;
    case WarningLevel:
      _buffer.prepend("Warning: ");
      break;
    case ErrorLevel:
      _buffer.prepend("Error: ");
      break;
    default:
      Q_ASSERT(_logLevel); // There should be no unknown log level
      break;
  }
#ifndef HAVE_SYSLOG_H
  _buffer.prepend(date.toString("yyyy-MM-dd hh:mm:ss "));
#endif
  log();
}

void Logger::log() {
  LogLevel lvl;
  if (Quassel::optionValue("loglevel") == "Debug") lvl = DebugLevel;
  else if (Quassel::optionValue("loglevel") == "Info") lvl = InfoLevel;
  else if (Quassel::optionValue("loglevel") == "Warning") lvl = WarningLevel;
  else if (Quassel::optionValue("loglevel") == "Error") lvl = ErrorLevel;
  else lvl = InfoLevel;

  if(_logLevel < lvl) return;

#ifdef HAVE_SYSLOG_H
  if (Quassel::isOptionSet("syslog")) {
    int prio;
    switch (lvl) {
      case DebugLevel:
        prio = LOG_DEBUG;
        break;
      case InfoLevel:
        prio = LOG_INFO;
        break;
      case WarningLevel:
        prio = LOG_WARNING;
        break;
      case ErrorLevel:
        prio = LOG_ERR;
        break;
      default:
        prio = LOG_INFO;
        break;
    }
    syslog(LOG_USER & prio, "%s", qPrintable(_buffer));
  }
#else
  // if we can't open logfile we log to stdout
  QTextStream out(stdout);
  QFile file;
  if(!Quassel::optionValue("logfile").isEmpty()) {
    file.setFileName(Quassel::optionValue("logfile"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
      out.setDevice(&file);
      _buffer.remove(QChar('\n'));
    }
  }
  out << _buffer << endl;
  if(file.isOpen()) file.close();
#endif
}


void Logger::logMessage(QtMsgType type, const char *msg) {
  switch (type) {
  case QtDebugMsg:
    Logger(Logger::DebugLevel) << msg;
    break;
  case QtWarningMsg:
    Logger(Logger::WarningLevel) << msg;
    break;
  case QtCriticalMsg:
    Logger(Logger::ErrorLevel) << msg;
    break;
  case QtFatalMsg:
    Logger(Logger::ErrorLevel) << msg;
    Quassel::logFatalMessage(msg);
    return;
  }
}
