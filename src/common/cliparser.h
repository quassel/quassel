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

#ifdef HAVE_KDE
#  include <KCmdLineOptions>
#endif

class CliParser {
public:
  CliParser();

  bool init(const QStringList &arguments = QStringList());

  QString value(const QString &longName);
  bool isSet(const QString &longName);
  inline void addSwitch(const QString &longName, const char shortName = 0, const QString &help = QString()) {
    addArgument(longName, CliParserArg(CliParserArg::CliArgSwitch, shortName, help));
  }
  inline void addOption(const QString &longName, const char shortName = 0, const QString &help = QString(), const QString &def = QString()) {
    addArgument(longName, CliParserArg(CliParserArg::CliArgOption, shortName, help, def));
  }
  void usage();

private:
  struct CliParserArg {
    enum CliArgType {
      CliArgInvalid,
      CliArgSwitch,
      CliArgOption
    };
    CliParserArg(const CliArgType _type = CliArgInvalid, const char _shortName = 0, const QString _help = QString(), const QString _def = QString())
    : type(_type),
    shortName(_shortName),
    help(_help),
    def(_def),
    value(QString()),
    boolValue(false) {};

    CliArgType type;
    char shortName;
    QString help;
    QString def;
    QString value;
    bool boolValue;
  };

  void addArgument(const QString &longName, const CliParserArg &arg);

#ifndef HAVE_KDE
  bool addLongArg(const CliParserArg::CliArgType type, const QString &name, const QString &value = QString());
  bool addShortArg(const CliParserArg::CliArgType type, const char shortName, const QString &value = QString());
  QString escapedValue(const QString &value);
  QString lnameOfShortArg(const char arg);

  QStringList argsRaw;
  QHash<QString, CliParserArg> argsHash;

#else
  KCmdLineOptions _cmdLineOptions;
#endif
};

#endif
