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

#ifndef ALIASMANAGER_H
#define ALIASMANAGER_H

#include <QVariantMap>

#include "bufferinfo.h"
#include "syncableobject.h"

class Network;

class AliasManager : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    inline AliasManager(QObject *parent = 0) : SyncableObject(parent) { setAllowClientUpdates(true); }
    AliasManager &operator=(const AliasManager &other);

    struct Alias {
        QString name;
        QString expansion;
        Alias(const QString &name_, const QString &expansion_) : name(name_), expansion(expansion_) {}
    };
    typedef QList<Alias> AliasList;

    int indexOf(const QString &name) const;
    inline bool contains(const QString &name) const { return indexOf(name) != -1; }
    inline bool isEmpty() const { return _aliases.isEmpty(); }
    inline int count() const { return _aliases.count(); }
    inline void removeAt(int index) { _aliases.removeAt(index); }
    inline Alias &operator[](int i) { return _aliases[i]; }
    inline const Alias &operator[](int i) const { return _aliases.at(i); }
    inline const AliasList &aliases() const { return _aliases; }

    static AliasList defaults();

    typedef QList<QPair<BufferInfo, QString> > CommandList;

    CommandList processInput(const BufferInfo &info, const QString &message);

public slots:
    virtual QVariantMap initAliases() const;
    virtual void initSetAliases(const QVariantMap &aliases);

    virtual void addAlias(const QString &name, const QString &expansion);

protected:
    void setAliases(const QList<Alias> &aliases) { _aliases = aliases; }
    virtual const Network *network(NetworkId) const = 0; // core and client require different access

private:
    void processInput(const BufferInfo &info, const QString &message, CommandList &previousCommands);
    void expand(const QString &alias, const BufferInfo &bufferInfo, const QString &msg, CommandList &previousCommands);

    AliasList _aliases;
};


#endif //ALIASMANAGER_H
