/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "qt5cliparser.h"

#include <QCoreApplication>
#include <QDebug>

bool Qt5CliParser::init(const QStringList &arguments)
{
    _qCliParser.addHelpOption();
    _qCliParser.addVersionOption();
    _qCliParser.setApplicationDescription(QCoreApplication::translate("CliParser", "Quassel IRC is a modern, distributed IRC client."));

    _qCliParser.process(arguments);
    return true; // process() does error handling by itself
}


bool Qt5CliParser::isSet(const QString &longName)
{

    return _qCliParser.isSet(longName);
}


QString Qt5CliParser::value(const QString &longName)
{
    return _qCliParser.value(longName);
}


void Qt5CliParser::usage()
{
    _qCliParser.showHelp();
}


void Qt5CliParser::addArgument(const QString &longName, const AbstractCliParser::CliParserArg &arg)
{
    QStringList names(longName);
    if (arg.shortName != 0)
        names << QString(arg.shortName);

    switch(arg.type) {
    case CliParserArg::CliArgSwitch:
        _qCliParser.addOption(QCommandLineOption(names, arg.help));
        break;
    case CliParserArg::CliArgOption:
        _qCliParser.addOption(QCommandLineOption(names, arg.help, arg.valueName, arg.def));
        break;
    default:
        qWarning() << "Warning: Unrecognized argument:" << longName;
    }
}
