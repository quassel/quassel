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

  if(Quassel::isOptionSet("oidentd-conffile"))
    configPath = Quassel::optionValue("oidentd-conffile");
  else
    configPath = configDir.absoluteFilePath(configFileName);

  configTag = " stanza created by Quassel";

  _configFile = new QFile(configPath);
  qDebug() << "1: _configFile" << _configFile->fileName();

  quasselStanza = QRegExp(QString("^lport .* { .* } #%1$").arg(configTag));

  if (parseConfig(false) && writeConfig())
    _initialized = true;

  qDebug() << "OidentdConfigGenerator" << (!_initialized ? "not" : "") << "initialized";

  return _initialized;
}

bool OidentdConfigGenerator::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort) {
  qDebug() << "localAddress" << localAddress;
  qDebug() << "localPort" << localPort;
  qDebug() << "peerAddress" << peerAddress;
  qDebug() << "peerPort" << peerPort;
  qDebug() << "ident" << identity->ident();

  QString ident = identity->ident();

  _config.append(QString("lport %1 { reply \"%2\" } #%3\n").arg(localPort).arg(ident).arg(configTag));

  bool ret = writeConfig();
  qDebug() << "config written" << ret;

  return ret;
}

// not yet implemented
bool OidentdConfigGenerator::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort) {
  Q_UNUSED(identity) Q_UNUSED(localAddress) Q_UNUSED(localPort) Q_UNUSED(peerAddress) Q_UNUSED(peerPort)
  return true;
}

bool OidentdConfigGenerator::parseConfig(bool keepQuasselStanzas) {
  qDebug() << "_configFile name" << _configFile->fileName();
  qDebug() << "open?" << _configFile->isOpen();
  qDebug() << "keeping our stanzas?" << keepQuasselStanzas;
  if (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadWrite))
    return false;

  QByteArray parsedConfig;
  while (!_configFile->atEnd()) {
    QByteArray line = _configFile->readLine();

    qDebug() << "line" << line;
    qDebug() << "line by us?" << lineByUs(line);
    if (keepQuasselStanzas || !lineByUs(line))
      parsedConfig.append(line);
  }

  _config = parsedConfig;

  return true;
}

bool OidentdConfigGenerator::writeConfig() {
  if (!_configFile->isOpen() && !_configFile->open(QFile::WriteOnly | QFile::Truncate))
    return false;

  //FIXME: thread safety
  QTextStream out(_configFile);
  out << _config;

  return _configFile->flush();
}

bool OidentdConfigGenerator::lineByUs(const QByteArray &line) {
  return !quasselStanza.exactMatch(line);
}
