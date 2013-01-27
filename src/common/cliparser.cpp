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

#include "cliparser.h"

#include <QDir>
#include <QDebug>
#include <QString>
#include <QFileInfo>

#include <iostream>

CliParser::CliParser() : AbstractCliParser()
{
}


void CliParser::addArgument(const QString &longName_, const CliParserArg &arg)
{
    QString longName = longName_;
    longName.remove(QRegExp("\\s*<.*>\\s*")); // KCmdLineArgs takes args of the form "arg <defval>"
    if (argsMap.contains(longName)) qWarning() << "Warning: Multiple definition of argument" << longName;
    if (arg.shortName != 0 && !lnameOfShortArg(arg.shortName).isNull())
        qWarning().nospace() << "Warning: Redefining shortName '" << arg.shortName << "' for " << longName << " previously defined for " << lnameOfShortArg(arg.shortName);
    argsMap.insert(longName, arg);
}


bool CliParser::addLongArg(const CliParserArg::CliArgType type, const QString &name, const QString &value)
{
    if (argsMap.contains(name)) {
        if (type == CliParserArg::CliArgOption && argsMap.value(name).type == type) {
            argsMap[name].value = escapedValue(value);
            return true;
        }
        else if (type == CliParserArg::CliArgSwitch && argsMap.value(name).type == type) {
            argsMap[name].boolValue = true;
            return true;
        }
    }
    qWarning() << "Warning: Unrecognized argument:" << name;
    return false;
}


bool CliParser::addShortArg(const CliParserArg::CliArgType type, const char shortName, const QString &value)
{
    QString longName = lnameOfShortArg(shortName);
    if (!longName.isNull()) {
        if (type == CliParserArg::CliArgOption && argsMap.value(longName).type == type) {
            argsMap[longName].value = escapedValue(value);
            return true;
        }
        else if (type == CliParserArg::CliArgSwitch) {
            if (argsMap.value(longName).type == type) {
                argsMap[longName].boolValue = true;
                return true;
            }
            // arg is an option but detected as a switch -> argument is missing
            else {
                qWarning().nospace() << "Warning: '" << shortName << "' is an option which needs an argument";
                return false;
            }
        }
    }
    qWarning().nospace() << "Warning: Unrecognized argument: '" << shortName << "'";
    return false;
}


QString CliParser::escapedValue(const QString &value)
{
    QString escapedValue = value;
    if (escapedValue.startsWith("~"))
        escapedValue.replace(0, 1, QDir::homePath());

    return escapedValue;
}


bool CliParser::init(const QStringList &args)
{
    argsRaw = args;
    QStringList::const_iterator currentArg;
    for (currentArg = argsRaw.constBegin(); currentArg != argsRaw.constEnd(); ++currentArg) {
        if (currentArg->startsWith("--")) {
            // long
            QString name;
            if (currentArg->contains("=")) {
                // option
                QStringList tmp = currentArg->mid(2).split("=");
                name = tmp.at(0);
                QString value;
                // this is needed to allow --option=""
                if (tmp.at(1).isNull()) value = QString("");
                else value = tmp.at(1);
                if (!addLongArg(CliParserArg::CliArgOption, name, value)) return false;
            }
            else {
                // switch
                name = currentArg->mid(2);
                if (!addLongArg(CliParserArg::CliArgSwitch, name)) return false;
            }
        }
        else if (currentArg->startsWith("-")) {
            // short
            char name;
            QStringList::const_iterator nextArg = currentArg+1;
            // if next arg is a short/long option/switch the current arg is one too
            if (nextArg == argsRaw.constEnd() || nextArg->startsWith("-")) {
                // switch
                for (int i = 0; i < currentArg->mid(1).toAscii().size(); i++) {
                    name = currentArg->mid(1).toAscii().at(i);
                    if (!addShortArg(CliParserArg::CliArgSwitch, name)) return false;
                }
            }
            // if next arg is is no option/switch it's an argument to a shortoption
            else {
                // option
                // short options are not freely mixable with other shortargs
                if (currentArg->mid(1).toAscii().size() > 1) {
                    qWarning() << "Warning: Shortoptions may not be combined with other shortoptions or switches";
                    return false;
                }
                QString value;
                bool skipNext = false;
                if (nextArg != argsRaw.constEnd()) {
                    value = nextArg->toLocal8Bit();
                    skipNext = true;
                }
                else value = currentArg->toLocal8Bit();
                name = currentArg->mid(1).toAscii().at(0);
                // we took one argument as argument to an option so skip it next time
                if (skipNext) currentArg++;
                if (!addShortArg(CliParserArg::CliArgOption, name, value)) return false;
            }
        }
        else {
            // we don't support plain arguments without -/--
            if (currentArg->toLatin1() != argsRaw.at(0)) return false;
        }
    }
    return true;
}


void CliParser::usage()
{
    std::cout << "Usage: " << qPrintable(QFileInfo(argsRaw.at(0)).completeBaseName()) << " [arguments]" << std::endl;

    // get size of longName field
    QStringList keys = argsMap.keys();
    uint lnameFieldSize = 0;
    foreach(QString key, keys) {
        uint size = 0;
        if (argsMap.value(key).type == CliParserArg::CliArgOption)
            size += key.size()*2;
        else
            size += key.size();
        // this is for " --...=[....] "
        size += 8;
        if (size > lnameFieldSize) lnameFieldSize = size;
    }

    QMap<QString, CliParserArg>::const_iterator arg;
    for (arg = argsMap.constBegin(); arg != argsMap.constEnd(); ++arg) {
        QString output;
        QString lnameField;

        if (arg.value().shortName) {
            output.append(" -").append(arg.value().shortName).append(",");
        }
        else output.append("    ");
        lnameField.append(" --").append(arg.key());
        if (arg.value().type == CliParserArg::CliArgOption) {
            lnameField.append("=[").append(arg.key().toUpper()).append("]");
        }
        output.append(lnameField.leftJustified(lnameFieldSize));
        if (!arg.value().help.isEmpty()) {
            output.append(arg.value().help);
        }
        if (arg.value().type == CliParserArg::CliArgOption && !arg.value().def.isNull()) {
            output.append(". Default is: ").append(arg.value().def);
        }
        std::cout << qPrintable(output) << std::endl;
    }
}


QString CliParser::value(const QString &longName)
{
    if (argsMap.contains(longName) && argsMap.value(longName).type == CliParserArg::CliArgOption) {
        if (!argsMap.value(longName).value.isNull())
            return argsMap.value(longName).value;
        else
            return argsMap.value(longName).def;
    }
    else {
        qWarning() << "Warning: Requested value of not defined argument" << longName << "or argument is a switch";
        return QString();
    }
}


bool CliParser::isSet(const QString &longName)
{
    if (argsMap.contains(longName)) {
        if (argsMap.value(longName).type == CliParserArg::CliArgOption) return !argsMap.value(longName).value.isNull();
        else return argsMap.value(longName).boolValue;
    }
    else {
        qWarning() << "Warning: Requested isSet of not defined argument" << longName;
        return false;
    }
}


QString CliParser::lnameOfShortArg(const char arg)
{
    QMap<QString, CliParserArg>::const_iterator cur;
    for (cur = argsMap.constBegin(); cur != argsMap.constEnd(); ++cur) {
        if (cur.value().shortName == arg) return cur.key();
    }
    return QString();
}
