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

#ifndef COREIRCLISTHELPER_H
#define COREIRCLISTHELPER_H

#include "irclisthelper.h"

#include "coresession.h"

class QTimerEvent;

class CoreIrcListHelper : public IrcListHelper
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    inline CoreIrcListHelper(CoreSession *coreSession) : IrcListHelper(coreSession), _coreSession(coreSession) {};

    inline virtual const QMetaObject *syncMetaObject() const { return &IrcListHelper::staticMetaObject; }

    inline CoreSession *coreSession() const { return _coreSession; }

    inline bool requestInProgress(const NetworkId &netId) const { return _channelLists.contains(netId); }

public slots:
    virtual QVariantList requestChannelList(const NetworkId &netId, const QStringList &channelFilters);
    bool addChannel(const NetworkId &netId, const QString &channelName, quint32 userCount, const QString &topic);
    bool endOfChannelList(const NetworkId &netId);

protected:
    void timerEvent(QTimerEvent *event);

private:
    bool dispatchQuery(const NetworkId &netId, const QString &query);

private:
    CoreSession *_coreSession;

    QHash<NetworkId, QString> _queuedQuery;
    QHash<NetworkId, QList<ChannelDescription> > _channelLists;
    QHash<NetworkId, QVariantList> _finishedChannelLists;
    QHash<int, NetworkId> _queryTimeout;
};


#endif //COREIRCLISTHELPER_H
