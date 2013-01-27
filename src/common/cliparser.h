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

#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <QMap>

#include "abstractcliparser.h"

//! Quassel's own parser for command line arguments
class CliParser : public AbstractCliParser
{
public:
    CliParser();

    bool init(const QStringList &arguments = QStringList());

    QString value(const QString &longName);
    bool isSet(const QString &longName);
    void usage();

private:
    void addArgument(const QString &longName, const CliParserArg &arg);
    bool addLongArg(const CliParserArg::CliArgType type, const QString &name, const QString &value = QString());
    bool addShortArg(const CliParserArg::CliArgType type, const char shortName, const QString &value = QString());
    QString escapedValue(const QString &value);
    QString lnameOfShortArg(const char arg);

    QStringList argsRaw;
    QMap<QString, CliParserArg> argsMap;
};


#endif
