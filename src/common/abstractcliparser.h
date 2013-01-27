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


    inline void addOption(const QString &longName, const char shortName = 0, const QString &help = QString(), const QString &def = QString())
    {
        addArgument(longName, CliParserArg(CliParserArg::CliArgOption, shortName, help, def));
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

    virtual void addArgument(const QString &longName, const CliParserArg &arg) = 0;
};


#endif
