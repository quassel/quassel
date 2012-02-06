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

#ifndef OIDENTDCONFIGGENERATOR_H
#define OIDENTDCONFIGGENERATOR_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QDateTime>

#include <QDebug>

class OidentdConfigGenerator : public QObject
{
    Q_OBJECT
public:
  explicit OidentdConfigGenerator(QObject *parent = 0);

  QDir configDir;
  QString configFileName;
    
signals:
    
public slots:
  bool update();

private:
  bool init();
  bool writeConfig();
  bool parseConfig();
  bool checkLine(const QByteArray &line);

  bool _initialized;
  QDateTime _lastSync;
  QFile *_configFile;
  QByteArray _config;

  QString configTag;

  QRegExp quasselStanza;
};

#endif // OIDENTDCONFIGGENERATOR_H
