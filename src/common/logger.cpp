/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include <iostream>



Logger::~Logger() {
  //qInstallMsgHandler(0);
}

void messageHandler(QtMsgType type, const char *msg) {
  switch (type) {
    case QtDebugMsg:
      std::cerr << "[DEBUG] " << msg << "\n";
      break;
    case QtWarningMsg:
      std::cerr << "[WARNING] " << msg << "\n";
      break;
    case QtCriticalMsg:
      std::cerr << "[CRITICAL] " << msg << "\n";
      break;
    case QtFatalMsg:
      std::cerr << "[FATAL] " << msg << "\n";
      abort(); // deliberately core dump
  }
}

Logger::Logger() {
  //qInstallMsgHandler(messageHandler);
}
