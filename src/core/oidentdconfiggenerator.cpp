/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "oidentdconfiggenerator.h"

OidentdConfigGenerator::OidentdConfigGenerator(QObject *parent) :
    QObject(parent),
    _initialized(false)
{
    if (!_initialized)
        init();
}


OidentdConfigGenerator::~OidentdConfigGenerator()
{
    _quasselConfig.clear();
    writeConfig();
    _configFile->deleteLater();
}


bool OidentdConfigGenerator::init()
{
    _configDir = QDir::homePath();
    _configFileName = ".oidentd.conf";

    if (Quassel::isOptionSet("oidentd-conffile"))
        _configPath = Quassel::optionValue("oidentd-conffile");
    else
        _configPath = _configDir.absoluteFilePath(_configFileName);

    _configTag = " stanza created by Quassel";

    _configFile = new QFile(_configPath);

    // Rx has to match Template in order for cleanup to work.
    // Template should be enhanced with the "from" parameter as soon as Quassel gains
    // the ability to bind to an IP on client sockets.

    _quasselStanzaTemplate = QString("lport %1 { reply \"%2\" } #%3\n");
    _quasselStanzaRx = QRegExp(QString("^lport .* \\{ .* \\} #%1\\r?\\n").arg(_configTag));

    // initially remove all Quassel stanzas that might be present
    if (parseConfig(false) && writeConfig())
        _initialized = true;

    return _initialized;
}


bool OidentdConfigGenerator::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort)
{
    Q_UNUSED(localAddress) Q_UNUSED(peerAddress) Q_UNUSED(peerPort)
    QString ident = identity->ident();

    _quasselConfig.append(_quasselStanzaTemplate.arg(localPort).arg(ident).arg(_configTag).toAscii());

    bool ret = writeConfig();

    return ret;
}


//! not yet implemented
bool OidentdConfigGenerator::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort)
{
    Q_UNUSED(identity) Q_UNUSED(localAddress) Q_UNUSED(localPort) Q_UNUSED(peerAddress) Q_UNUSED(peerPort)
    return true;
}


bool OidentdConfigGenerator::parseConfig(bool readQuasselStanzas)
{
    if (!_configFile->exists())
        return true;

    if (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadOnly))
        return false;
    _mutex.lock();

    _parsedConfig.clear();
    _configFile->seek(0);
    while (!_configFile->atEnd()) {
        QByteArray line = _configFile->readLine();

        if (!lineByUs(line))
            _parsedConfig.append(line);
        else if (readQuasselStanzas)
            _quasselConfig.append(line);
    }

    _configFile->close();
    _mutex.unlock();
    return true;
}


bool OidentdConfigGenerator::writeConfig()
{
#ifdef HAVE_UMASK
    mode_t prev_umask = umask(S_IXUSR | S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH); // == 0133, rw-r--r--
#endif
    bool not_open = (!_configFile->isOpen() && !_configFile->open(QIODevice::ReadWrite | QIODevice::Text));
#ifdef HAVE_UMASK
    umask(prev_umask);
#endif

    if (not_open)
        return false;

    _mutex.lock();

    _configFile->seek(0);
    _configFile->resize(0);
    _configFile->write(_parsedConfig);
    _configFile->write(_quasselConfig);

    _configFile->close();
    _mutex.unlock();
    return true;
}


bool OidentdConfigGenerator::lineByUs(const QByteArray &line)
{
    return _quasselStanzaRx.exactMatch(line);
}
