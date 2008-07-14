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
#include "cliparser.h"

#include <QString>
#include <QFileInfo>

CliParser::CliParser(QStringList arguments)
{
  argsRaw = arguments;
//   remove Qt internal debugging arguments 
  argsRaw.removeOne("-sync");
  argsRaw.removeOne("-nograb");
  argsRaw.removeOne("-dograb");
}

void CliParser::addSwitch(const QString longName, const char shortName, const QString help) {
  CliParserArg arg = CliParserArg(CliParserArg::CliArgSwitch, shortName, help);
  argsHash.insert(longName, arg);
  if(shortName) {
   if(!shortHash.contains(shortName)) shortHash.insert(shortName, argsHash.find(longName));
   else qWarning("Warning: shortName %c defined multiple times.", shortName);
  }
}

void CliParser::addOption(const QString longName, const char shortName, const QString help, const QString def) {
  CliParserArg arg = CliParserArg(CliParserArg::CliArgOption, shortName, help, def);
  argsHash.insert(longName, arg);
  if(shortName) {
   if(!shortHash.contains(shortName)) shortHash.insert(shortName, argsHash.find(longName));
   else qWarning("Warning: shortName %c defined multiple times.", shortName);
  }
}

bool CliParser::parse() {
  QStringList::const_iterator currentArg;
  for (currentArg = argsRaw.constBegin(); currentArg != argsRaw.constEnd(); ++currentArg) {
    if(currentArg->startsWith("--")) {
      QString name;
      // long
      if(currentArg->contains("=")) {
        // option
        QStringList tmp = currentArg->mid(2).split("=");
        name = tmp.at(0);
        QString value = tmp.at(1);
        if(argsHash.contains(name) && !value.isEmpty()){
          argsHash[name].value = value;
        }
        else return false;
      }
      else {
        // switch
        name = currentArg->mid(2);
        if(argsHash.contains(name)) {
            argsHash[name].boolValue = true;
        }
        else return false;
      }
    }
    else if(currentArg->startsWith("-")) {
    char name;
        // short
      bool bla = true;
      if(++currentArg == argsRaw.constEnd()) {
        --currentArg;
        bla = false;
      }
      // if next arg is a short/long option/switch the current arg is one too
      if(currentArg->startsWith("-")) {
        // switch
        if(bla) --currentArg;
        for (int i = 0; i < currentArg->mid(1).toAscii().size(); i++) {
          name = currentArg->mid(1).toAscii().at(i);
          if(shortHash.contains(name) && shortHash.value(name).value().type == CliParserArg::CliArgSwitch) {
            shortHash[name].value().boolValue = true;
          }
          else return false;
        }
      }
      // if next arg is is no option/switch it's an argument to a shortoption
      else {
        // option
        QString value = currentArg->toLocal8Bit();
        if(bla) --currentArg;
        name = currentArg->mid(1).toAscii().at(0);
        if(bla) currentArg++;
        if(shortHash.contains(name) && shortHash.value(name).value().type == CliParserArg::CliArgOption) {
          shortHash[name].value().value = value;
        }
        else return false;
      }
    }
    else {
      // we don't support plain arguments without -/--
      if(currentArg->toLatin1() != argsRaw.at(0)) {
        return false;
      }
    }
  }
  return true;
}

void CliParser::usage() {
  qWarning("Usage: %s [arguments]",QFileInfo(argsRaw.at(0)).completeBaseName().toLatin1().constData());
  
  // get size of longName field
  QStringList keys = argsHash.keys();
  uint lnameFieldSize = 0;
  foreach (QString key, keys) {
    uint size = 0;
    if(argsHash.value(key).type == CliParserArg::CliArgOption)
      size += key.size()*2;
    else
      size += key.size();
    // this is for " --...=[....] "
    size += 8;
    if(size > lnameFieldSize) lnameFieldSize = size;
  }
  
  QHash<QString, CliParserArg>::const_iterator arg;
  for(arg = argsHash.constBegin(); arg != argsHash.constEnd(); ++arg) {
    QString output;
    QString lnameField;
    
    if(arg.value().shortName) {
      output.append(" -").append(arg.value().shortName).append(",");
    }
    else output.append("    ");
    lnameField.append(" --").append(arg.key());
    if(arg.value().type == CliParserArg::CliArgOption) {
      lnameField.append("=[").append(arg.key().toUpper()).append("]");
    }
    output.append(lnameField.leftJustified(lnameFieldSize));
    if(!arg.value().help.isEmpty()) {
      output.append(arg.value().help);
    }
    if(arg.value().type == CliParserArg::CliArgOption) {
      output.append(". Default is: ").append(arg.value().def);
    }
    qWarning(output.toLatin1());
  }
}

QString CliParser::value(const QString &longName) {
  if(argsHash.contains(longName) && argsHash.value(longName).type == CliParserArg::CliArgOption) {
    if(!argsHash.value(longName).value.isEmpty())
      return argsHash.value(longName).value;
    else
      return argsHash.value(longName).def;
  }
  else {
    qWarning("Warning: Requested value of not defined argument '%s' or argument is a switch",longName.toLatin1().constData());
    return QString();
  }
}

bool CliParser::isSet(const QString &longName) {
  if(argsHash.contains(longName)) {
    if(argsHash.value(longName).type == CliParserArg::CliArgOption) return !argsHash.value(longName).value.isEmpty();
    else return argsHash.value(longName).boolValue;
  }
  else {
    qWarning("Warning: Requested isSet of not defined argument '%s'",longName.toLatin1().constData());
    return false;
  }
}

CliParserArg::CliParserArg(const CliArgType _type, const char _shortName, const QString _help, const QString _def)
  : type(_type),
    shortName(_shortName),
    help(_help),
    def(_def),
    value(QString()),
    boolValue(false)
{
}

CliParserArg::CliParserArg(const CliParserArg &other) {
  type = other.type;
  shortName = other.shortName;
  help = other.help;
  def = other.def;
  value = other.value;
  boolValue = other.boolValue;
}

CliParserArg &CliParserArg::operator=(const CliParserArg &other) {
  type = other.type;
  shortName = other.shortName;
  help = other.help;
  def = other.def;
  value = other.value;
  boolValue = other.boolValue;
  return *this;
}
