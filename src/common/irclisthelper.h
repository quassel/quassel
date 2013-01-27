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

#ifndef IRCLISTHELPER_H
#define IRCLISTHELPER_H

#include "syncableobject.h"
#include "types.h"

/*
 * This is a little helper to display channel lists of a network.
 * The whole process is done in 3 steps:
 *  1.) the client requests to issue a LIST command with requestChannelList()
 *  2.) RPL_LIST fills on the core the list of available channels
 *      when RPL_LISTEND is received the clients will be informed, that they can pull the data
 *  3.) client pulls the data by calling requestChannelList again. receiving the data in receiveChannelList
 */
class IrcListHelper : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    inline IrcListHelper(QObject *parent = 0) : SyncableObject(parent) { setInitialized(); };

    struct ChannelDescription {
        QString channelName;
        quint32 userCount;
        QString topic;
        ChannelDescription(const QString &channelName_, quint32 userCount_, const QString &topic_) : channelName(channelName_), userCount(userCount_), topic(topic_) {};
    };

public slots:
    inline virtual QVariantList requestChannelList(const NetworkId &netId, const QStringList &channelFilters) { REQUEST(ARG(netId), ARG(channelFilters)); return QVariantList(); }
    inline virtual void receiveChannelList(const NetworkId &, const QStringList &, const QVariantList &) {};
    inline virtual void reportFinishedList(const NetworkId &netId) { SYNC(ARG(netId)) }
    inline virtual void reportError(const QString &error) { SYNC(ARG(error)) }
};


#endif //IRCLISTHELPER_H
