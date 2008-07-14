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
}

void CliParser::addSwitch(QString longName, char shortName, QVariant def) {
  addArgument(CliParserArg::CliArgSwitch, longName, shortName, def);
}

void CliParser::addOption(QString longName, char shortName, QVariant def) {
  addArgument(CliParserArg::CliArgOption, longName, shortName, def);
}

void CliParser::addArgument(CliParserArg::CliArgType type, QString longName, char shortName, QVariant def) {
  CliParserArg arg = CliParserArg(type, longName, shortName, def);
  argsHash.insert(longName, arg);
  if(shortName && !shortHash.contains(shortName)) shortHash.insert(shortName, argsHash.find(longName));
}

void CliParser::addHelp(QString key, QString txt) {
  if(argsHash.contains(key)) argsHash[key].shortHelp = txt;
  else qWarning("Warning: Helptext for unknown argument '%s' given",key.toLatin1().constData());
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
            argsHash[name].value = true;
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
            shortHash[name].value().value = true;
          }
          else return false;
        }
      }
      // if next arg is is no option/switch it's an argument to a shortoption
      else {
        // option
        QString value = currentArg->toUtf8();
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
    
    if(arg.value().sname) {
      output.append(" -").append(arg.value().sname).append(",");
    }
    else output.append("    ");
    lnameField.append(" --").append(arg.key().toLatin1().constData());
    if(arg.value().type == CliParserArg::CliArgOption) {
      lnameField.append("=[").append(arg.value().lname.toUpper()).append("]");
    }
    output.append(lnameField.leftJustified(lnameFieldSize));
    if(!arg.value().shortHelp.isEmpty()) {
      output.append(arg.value().shortHelp);
    }
    if(arg.value().type == CliParserArg::CliArgOption) {
      output.append(". Default is: ").append(arg.value().def.toString());
    }
    qWarning(output.toLatin1());
  }
}

QVariant CliParser::value(QString key) {
  if(argsHash.contains(key)) {
    if(argsHash.value(key).value.isValid())
      return argsHash.value(key).value;
    else
      return argsHash.value(key).def;
  }
  else {
    qWarning("Warning: Requested value of not defined argument '%s'",key.toLatin1().constData());
    return QVariant();
  }
}

CliParserArg::CliParserArg(CliArgType _type, QString longName, char shortName, QVariant _def)
  : type(_type),
    lname(longName),
    sname(shortName),
    shortHelp(QString()),
    def(_def),
    value(QVariant()) {
}

CliParserArg::CliParserArg(const CliParserArg &other) {
  type = other.type;
  lname = other.lname;
  sname = other.sname;
  shortHelp = other.shortHelp;
  def = other.def;
  value = other.value;
}

CliParserArg &CliParserArg::operator=(const CliParserArg &other) {
  type = other.type;
  lname = other.lname;
  sname = other.sname;
  shortHelp = other.shortHelp;
  def = other.def;
  value = other.value;
  return *this;
}
