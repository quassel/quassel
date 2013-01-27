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

#ifndef OIDENTDCONFIGGENERATOR_H
#define OIDENTDCONFIGGENERATOR_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QHostAddress>
#include <QMutex>
#include <QByteArray>

#ifdef HAVE_UMASK
#  include <sys/types.h>
#  include <sys/stat.h>
#endif /* HAVE_UMASK */

#include "quassel.h"
#include "coreidentity.h"

//!  Produces oidentd configuration files
/*!
  Upon IRC connect this class puts the clients' ident data into an oidentd configuration file.

  The default path is <~/.oidentd.conf>.

  For oidentd to incorporate this file, the global oidentd.conf has to state something like this:

  user "quassel" {
    default {
      allow spoof
      allow spoof_all
    }
  }

*/

class OidentdConfigGenerator : public QObject
{
    Q_OBJECT
public:
    explicit OidentdConfigGenerator(QObject *parent = 0);
    ~OidentdConfigGenerator();

public slots:
    bool addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);
    bool removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort);

private:
    bool init();
    bool writeConfig();
    bool parseConfig(bool readQuasselStanzas = false);
    bool lineByUs(const QByteArray &line);

    bool _initialized;
    QDateTime _lastSync;
    QFile *_configFile;
    QByteArray _parsedConfig;
    QByteArray _quasselConfig;
    // Mutex isn't strictly necessary at the moment, since with the current invocation in Core only one instance at a time exists
    QMutex _mutex;

    QDir _configDir;
    QString _configFileName;
    QString _configPath;
    QString _configTag;
    QRegExp _quasselStanzaRx;
    QString _quasselStanzaTemplate;
};


#endif // OIDENTDCONFIGGENERATOR_H
