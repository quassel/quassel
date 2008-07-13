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
#include <QDebug>

CliParser::CliParser(int argc, char *argv[])
{
  if(argc) {
    for (int i=0; i < argc; ++i)
      argsRaw.append(QString::fromUtf8(argv[i]));
  }
}


CliParser::~CliParser()
{
}

void CliParser::addArgument(QString longName, char shortName = 0, QVariant def = QVariant()) {
  CliParserArg arg = CliParserArg(longName, shortName, def);
  argsHash.insert(longName, arg);
  qDebug() << "Added Argument: " << longName << " with arg-addr: " << &arg;
}

bool CliParser::parse() {
  qDebug() << "Parse results: ";
  qDebug() << argsHash.value("logfile").value;
  argsHash[QString("logfile")].value = "BOOYA";
}

QVariant CliParser::value(QString key) {
  return argsHash.value(key).value;
}

CliParserArg::CliParserArg(QString longName, char shortName = 0, QVariant _def = QVariant() )
  : lname(longName),
    sname(shortName),
    def(_def),
    value(0) {
    
}

CliParserArg::CliParserArg(const CliParserArg &other) {
  lname = other.lname;
  sname = other.sname;
  def = other.def;
  value = other.value;
}

CliParserArg &CliParserArg::operator=(const CliParserArg &other) {
  lname = other.lname;
  sname = other.sname;
  def = other.def;
  value = other.value;
  return *this;
}

