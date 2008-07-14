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
  CliParserArg(CliArgType type, QString longName, char shortName = 0, QVariant _def = QVariant());
  CliParserArg &operator=(const CliParserArg &other);

  CliArgType type;
  QString lname;
  char sname;
  QString shortHelp;
  QVariant def;
  QVariant value;
};
Q_DECLARE_METATYPE(CliParserArg);

class CliParser{
public:
  inline CliParser() {};
  CliParser(QStringList arguments);
  bool parse();
  QVariant value(QString key);
  void addSwitch(QString longName, char shortName = 0, QVariant def = false);
  void addOption(QString longName, char shortName = 0, QVariant def = QVariant());
  void addHelp(QString key, QString txt);
  void usage();
private:
  void addArgument(CliParserArg::CliArgType type, QString longName, char shortName, QVariant def);
  QStringList argsRaw;
  QHash<QString, CliParserArg> argsHash;
  QHash<const char, QHash<QString, CliParserArg>::iterator> shortHash;
};


#endif
