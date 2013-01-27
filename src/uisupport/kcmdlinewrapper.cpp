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

#include "kcmdlinewrapper.h"

#include <KCmdLineArgs>

KCmdLineWrapper::KCmdLineWrapper()
{
}


void KCmdLineWrapper::addArgument(const QString &longName, const CliParserArg &arg)
{
    if (arg.shortName != 0) {
        _cmdLineOptions.add(QByteArray().append(arg.shortName));
    }
    _cmdLineOptions.add(longName.toUtf8(), ki18n(arg.help.toUtf8()), arg.def.toUtf8());
}


bool KCmdLineWrapper::init(const QStringList &)
{
    KCmdLineArgs::addCmdLineOptions(_cmdLineOptions);
    return true;
}


QString KCmdLineWrapper::value(const QString &longName)
{
    return KCmdLineArgs::parsedArgs()->getOption(longName.toUtf8());
}


bool KCmdLineWrapper::isSet(const QString &longName)
{
    // KCmdLineArgs handles --nooption like NOT --option
    if (longName.startsWith("no"))
        return !KCmdLineArgs::parsedArgs()->isSet(longName.mid(2).toUtf8());
    return KCmdLineArgs::parsedArgs()->isSet(longName.toUtf8());
}


void KCmdLineWrapper::usage()
{
    KCmdLineArgs::usage();
}
