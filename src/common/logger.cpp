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
#include "global.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

Logger::~Logger() {
  QDateTime date = QDateTime::currentDateTime();
  if(_logLevel == DebugLevel) _buffer.prepend("Debug: ");
  else if (_logLevel == InfoLevel) _buffer.prepend("Info: ");
  else if (_logLevel == WarningLevel) _buffer.prepend("Warning: ");
  else if (_logLevel == ErrorLevel) _buffer.prepend("Error: ");
  _buffer.prepend(date.toString("yyyy-MM-dd hh:mm:ss "));
  log();
}

void Logger::log() {
  LogLevel lvl;
  if (Global::parser.value("loglevel") == "Debug") lvl = DebugLevel;
  else if (Global::parser.value("loglevel") == "Info") lvl = InfoLevel;
  else if (Global::parser.value("loglevel") == "Warning") lvl = WarningLevel;
  else if (Global::parser.value("loglevel") == "Error") lvl = ErrorLevel;
  else lvl = InfoLevel;

  if(_logLevel < lvl) return;

  // if we can't open logfile we log to stdout
  QTextStream out(stdout);
  QFile file;
  if(!Global::parser.value("logfile").isEmpty()) {
    file.setFileName(Global::parser.value("logfile"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
      out.setDevice(&file);
      _buffer.remove(QChar('\n'));
    }
  }
  out << _buffer << endl;
  if(file.isOpen()) file.close();
}
