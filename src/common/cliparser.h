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

#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVariant>

class CliParserArg {
public:
  enum CliArgType {
    CliArgInvalid,
    CliArgSwitch,
    CliArgOption
  };
  typedef CliArgType CliArgTypes;
  
  inline CliParserArg() {};
  CliParserArg(const CliParserArg &other);
  CliParserArg(const CliArgType type, const char _shortName = 0, const QString _help = QString(), const QString _def = QString());
  CliParserArg &operator=(const CliParserArg &other);

  CliArgType type;
  char shortName;
  QString help;
  QString def;
  QString value;
  bool boolValue;
};
Q_DECLARE_METATYPE(CliParserArg);

class CliParser{
public:
  inline CliParser() {};
  CliParser(QStringList arguments);
  bool parse();
  QString value(const QString &longName);
  bool isSet(const QString &longName);
  void addSwitch(const QString longName, const char shortName = 0, const QString help = QString());
  void addOption(const QString longName, const char shortName = 0, const QString help = QString(), const QString def = QString());
  void usage();
private:
  QStringList argsRaw;
  QHash<QString, CliParserArg> argsHash;
  QHash<const char, QHash<QString, CliParserArg>::iterator> shortHash;
};

#endif
