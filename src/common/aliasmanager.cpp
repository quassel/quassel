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

#include <QDebug>
#include <QStringList>

#include "aliasmanager.h"
#include "network.h"

INIT_SYNCABLE_OBJECT(AliasManager)
AliasManager &AliasManager::operator=(const AliasManager &other)
{
    if (this == &other)
        return *this;

    SyncableObject::operator=(other);
    _aliases = other._aliases;
    return *this;
}


int AliasManager::indexOf(const QString &name) const
{
    for (int i = 0; i < _aliases.count(); i++) {
        if (_aliases[i].name == name)
            return i;
    }
    return -1;
}


QVariantMap AliasManager::initAliases() const
{
    QVariantMap aliases;
    QStringList names;
    QStringList expansions;

    for (int i = 0; i < _aliases.count(); i++) {
        names << _aliases[i].name;
        expansions << _aliases[i].expansion;
    }

    aliases["names"] = names;
    aliases["expansions"] = expansions;
    return aliases;
}


void AliasManager::initSetAliases(const QVariantMap &aliases)
{
    QStringList names = aliases["names"].toStringList();
    QStringList expansions = aliases["expansions"].toStringList();

    if (names.count() != expansions.count()) {
        qWarning() << "AliasesManager::initSetAliases: received" << names.count() << "alias names but only" << expansions.count() << "expansions!";
        return;
    }

    _aliases.clear();
    for (int i = 0; i < names.count(); i++) {
        _aliases << Alias(names[i], expansions[i]);
    }
}


void AliasManager::addAlias(const QString &name, const QString &expansion)
{
    if (contains(name)) {
        return;
    }

    _aliases << Alias(name, expansion);

    SYNC(ARG(name), ARG(expansion))
}


AliasManager::AliasList AliasManager::defaults()
{
    AliasList aliases;
    aliases << Alias("j", "/join $0")
            << Alias("ns", "/msg nickserv $0")
            << Alias("nickserv", "/msg nickserv $0")
            << Alias("cs", "/msg chanserv $0")
            << Alias("chanserv",  "/msg chanserv $0")
            << Alias("hs", "/msg hostserv $0")
            << Alias("hostserv", "/msg hostserv $0")
            << Alias("wii", "/whois $0 $0")
            << Alias("back", "/quote away");

#ifdef Q_OS_LINUX
    // let's add aliases for scripts that only run on linux
    aliases << Alias("inxi", "/exec inxi $0")
            << Alias("sysinfo", "/exec inxi -d");
#endif

    return aliases;
}


AliasManager::CommandList AliasManager::processInput(const BufferInfo &info, const QString &msg)
{
    CommandList result;
    processInput(info, msg, result);
    return result;
}


void AliasManager::processInput(const BufferInfo &info, const QString &msg_, CommandList &list)
{
    QString msg = msg_;

    // leading slashes indicate there's a command to call unless there is another one in the first section (like a path /proc/cpuinfo)
    int secondSlashPos = msg.indexOf('/', 1);
    int firstSpacePos = msg.indexOf(' ');
    if (!msg.startsWith('/') || (secondSlashPos != -1 && (secondSlashPos < firstSpacePos || firstSpacePos == -1))) {
        if (msg.startsWith("//"))
            msg.remove(0, 1);  // //asdf is transformed to /asdf
        msg.prepend("/SAY "); // make sure we only send proper commands to the core
    }
    else {
        // check for aliases
        QString cmd = msg.section(' ', 0, 0).remove(0, 1).toUpper();
        for (int i = 0; i < count(); i++) {
            if ((*this)[i].name.toUpper() == cmd) {
                expand((*this)[i].expansion, info, msg.section(' ', 1), list);
                return;
            }
        }
    }

    list.append(qMakePair(info, msg));
}


void AliasManager::expand(const QString &alias, const BufferInfo &bufferInfo, const QString &msg, CommandList &list)
{
    const Network *net = network(bufferInfo.networkId());
    if (!net) {
        // FIXME send error as soon as we have a method for that!
        return;
    }

    QRegExp paramRangeR("\\$(\\d+)\\.\\.(\\d*)");
    QStringList commands = alias.split(QRegExp("; ?"));
    QStringList params = msg.split(' ');
    QStringList expandedCommands;
    for (int i = 0; i < commands.count(); i++) {
        QString command = commands[i];

        // replace ranges like $1..3
        if (!params.isEmpty()) {
            int pos;
            while ((pos = paramRangeR.indexIn(command)) != -1) {
                int start = paramRangeR.cap(1).toInt();
                bool ok;
                int end = paramRangeR.cap(2).toInt(&ok);
                if (!ok) {
                    end = params.count();
                }
                if (end < start)
                    command = command.replace(pos, paramRangeR.matchedLength(), QString());
                else {
                    command = command.replace(pos, paramRangeR.matchedLength(), QStringList(params.mid(start - 1, end - start + 1)).join(" "));
                }
            }
        }

        for (int j = params.count(); j > 0; j--) {
            IrcUser *ircUser = net->ircUser(params[j - 1]);
            command = command.replace(QString("$%1:hostname").arg(j), ircUser ? ircUser->host() : QString("*"));
            command = command.replace(QString("$%1").arg(j), params[j - 1]);
        }
        command = command.replace("$0", msg);
        command = command.replace("$channelname", bufferInfo.bufferName()); // legacy
        command = command.replace("$channel", bufferInfo.bufferName());
        command = command.replace("$currentnick", net->myNick()); // legacy
        command = command.replace("$nick", net->myNick());
        expandedCommands << command;
    }

    while (!expandedCommands.isEmpty()) {
        QString command;
        if (expandedCommands[0].trimmed().toLower().startsWith("/wait")) {
            command = expandedCommands.join("; ");
            expandedCommands.clear();
        }
        else {
            command = expandedCommands.takeFirst();
        }
        list.append(qMakePair(bufferInfo, command));
    }
}
