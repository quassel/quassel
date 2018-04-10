/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#ifndef ABSTRACTCLIPARSER_H
#define ABSTRACTCLIPARSER_H

#include <QStringList>

class AbstractCliParser
{
public:
    virtual bool init(const QStringList &arguments = QStringList()) = 0;

    virtual QString value(const QString &longName) = 0;
    virtual bool isSet(const QString &longName) = 0;
    inline void addSwitch(const QString &longName, const char shortName = 0, const QString &help = QString())
    {
        addArgument(longName, CliParserArg(CliParserArg::CliArgSwitch, shortName, help));
    }


    inline void addOption(const QString &longName, const char shortName = 0, const QString &help = QString(), const QString &valueName = QString(), const QString &def = QString())
    {
        addArgument(longName, CliParserArg(CliParserArg::CliArgOption, shortName, help, valueName, def));
    }


    virtual void usage() = 0;

    virtual ~AbstractCliParser() {};

protected:
    struct CliParserArg {
        enum CliArgType {
            CliArgInvalid,
            CliArgSwitch,
            CliArgOption
        };

        CliParserArg(const CliArgType type = CliArgInvalid, const char shortName = 0, const QString &help = QString(), const QString &valueName = QString(), const QString &def = QString())
            : type(type)
            , shortName(shortName)
            , help(help)
            , valueName(valueName)
            , def(def)
            {};

        CliArgType type;
        char shortName;
        QString help;
        QString valueName;
        QString def;
        QString value;
        bool boolValue = false;
    };

    virtual void addArgument(const QString &longName, const CliParserArg &arg) = 0;
};


#endif
