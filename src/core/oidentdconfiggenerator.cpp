/***************************************************************************
 *   Copyright (C) 2012 by the Quassel Project                             *
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

#include "oidentdconfiggenerator.h"

OidentdConfigGenerator::OidentdConfigGenerator(QObject *parent) :
  QObject(parent),
  _initialized(false)
{
  qDebug() << "OidentdConfigGenerator() checking for being initialized";
  if (!_initialized)
    init();
}

bool OidentdConfigGenerator::init() {
  configDir = QDir::homePath();
  configFileName = ".oidentd.conf";
  configTag = " stanza created by Quassel";

  _configFile = new QFile(configDir.absoluteFilePath(configFileName));
  qDebug() << "1: _configFile" << _configFile->fileName();

  quasselStanza = QRegExp(QString("^lport .* { .* } #%1$").arg(configTag));

  if (update())
    _initialized = true;

  qDebug() << "konichi wa °-°";

  return _initialized;
}

bool OidentdConfigGenerator::update() {
  if (parseConfig())
    qDebug() << "oidentd config parsed successfully";
  else
    qDebug() << QString("parsing oidentd config failed (%1 [%2])").arg(_configFile->errorString()).arg(_configFile->error());

  return writeConfig();
}

bool OidentdConfigGenerator::parseConfig() {
  qDebug() << "_configFile name" << _configFile->fileName();
  qDebug() << "open?" << _configFile->isOpen();
  if (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadWrite))
    return false;

  QByteArray parsedConfig;
  while (!_configFile->atEnd()) {
    QByteArray line = _configFile->readLine();

    if (checkLine(line))
      parsedConfig.append(line);
  }

  _config = parsedConfig;

  return true;
}

bool OidentdConfigGenerator::writeConfig() {
  return true;
}

bool OidentdConfigGenerator::checkLine(const QByteArray &line) {
  return !quasselStanza.exactMatch(line);
}
