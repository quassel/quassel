/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "coreapplication.h"

#include "core.h"

CoreApplicationInternal::CoreApplicationInternal() {
  // put core-only arguments here
  CliParser *parser = Quassel::cliParser();
  parser->addOption("port",'p', tr("The port quasselcore will listen at"), QString("4242"));
  parser->addSwitch("norestore", 'n', tr("Don't restore last core's state"));
  parser->addOption("logfile", 'l', tr("Path to logfile"));
  parser->addOption("loglevel", 'L', tr("Loglevel Debug|Info|Warning|Error"), "Info");
  parser->addOption("datadir", 0, tr("Specify the directory holding datafiles like the Sqlite DB and the SSL Cert"));
}

CoreApplicationInternal::~CoreApplicationInternal() {
  Core::saveState();
  Core::destroy();
}

bool CoreApplicationInternal::init() {
  /* FIXME
  This is an initial check if logfile is writable since the warning would spam stdout if done
  in current Logger implementation. Can be dropped whenever the logfile is only opened once.
  */
  QFile logFile;
  if(!Quassel::optionValue("logfile").isEmpty()) {
    logFile.setFileName(Quassel::optionValue("logfile"));
    if(!logFile.open(QIODevice::Append | QIODevice::Text))
      qWarning("Warning: Couldn't open logfile '%s' - will log to stdout instead",qPrintable(logFile.fileName()));
    else logFile.close();
  }

  Core::instance();  // create and init the core

  if(!Quassel::isOptionSet("norestore")) {
    Core::restoreState();
  }
  return true;
}

/*****************************************************************************/

CoreApplication::CoreApplication(int &argc, char **argv) : QCoreApplication(argc, argv), Quassel() {
  setRunMode(Quassel::CoreOnly);
  _internal = new CoreApplicationInternal();
}

CoreApplication::~CoreApplication() {
  delete _internal;
}

bool CoreApplication::init() {
  if(Quassel::init())
    return _internal->init();
  return false;
}
