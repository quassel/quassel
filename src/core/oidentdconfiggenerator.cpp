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

  // Rx has to match Template in order for cleanup to work.
  // Template should be enhanced with the "from" parameter as soon as Quassel gains
  // the ability to bind to an IP on client sockets.

  quasselStanzaTemplate = QString("lport %1 { reply \"%2\" } #%3\n");
  quasselStanzaRx = QRegExp(QString("^lport .* \\{ .* \\} #%1\\r?\\n").arg(configTag));

  _mutex.lock();
  // initially remove all Quassel stanzas that might be present
  if (parseConfig(false) && writeConfig())
    _initialized = true;
  _mutex.unlock();

  return _initialized;
}

bool OidentdConfigGenerator::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort) {
  Q_UNUSED(localAddress) Q_UNUSED(peerAddress) Q_UNUSED(peerPort)
  QString ident = identity->ident();

  _config.append(quasselStanzaTemplate.arg(localPort).arg(ident).arg(configTag));

  _mutex.lock();
  bool ret = writeConfig();
  _mutex.unlock();

  return ret;
}

// not yet implemented
bool OidentdConfigGenerator::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort) {
  Q_UNUSED(identity) Q_UNUSED(localAddress) Q_UNUSED(localPort) Q_UNUSED(peerAddress) Q_UNUSED(peerPort)
  return true;
}

bool OidentdConfigGenerator::parseConfig(bool keepQuasselStanzas) {
  if (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadWrite))
    return false;

  QByteArray parsedConfig;
  while (!_configFile->atEnd()) {
    QByteArray line = _configFile->readLine();

    if (keepQuasselStanzas || !lineByUs(line))
      parsedConfig.append(line);
  }

  _config = parsedConfig;

  return true;
}

bool OidentdConfigGenerator::writeConfig() {
  if (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadWrite | QIODevice::Text))
    return false;

  _configFile->seek(0);
  _configFile->resize(0);
  _configFile->write(_config);

  return _configFile->flush();
}

bool OidentdConfigGenerator::lineByUs(const QByteArray &line) {
  return quasselStanzaRx.exactMatch(line);
}
