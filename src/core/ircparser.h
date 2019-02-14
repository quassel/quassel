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

#pragma once

#include "coresession.h"
#include "irctag.h"

class Event;
class EventManager;
class IrcEvent;
class NetworkDataEvent;

class IrcParser : public QObject
{
    Q_OBJECT

public:
    explicit IrcParser(CoreSession* session);

    inline CoreSession* coreSession() const { return _coreSession; }
    inline EventManager* eventManager() const { return coreSession()->eventManager(); }

signals:
    void newEvent(Event*);

protected:
    Q_INVOKABLE void processNetworkIncoming(NetworkDataEvent* e);

    bool checkParamCount(const QString& cmd, const QList<QByteArray>& params, int minParams);

    // no-op if we don't have crypto support!
    QByteArray decrypt(Network* network, const QString& target, const QByteArray& message, bool isTopic = false);

private:
    CoreSession* _coreSession;

    bool _debugLogRawIrc;      ///< If true, include raw IRC socket messages in the debug log
    qint32 _debugLogRawNetId;  ///< Network ID for logging raw IRC socket messages, or -1 for all
    bool _debugLogParsedIrc;      ///< If true, include parsed IRC socket messages in the debug log
    qint32 _debugLogParsedNetId;  ///< Network ID for logging parsed IRC socket messages, or -1 for all
};
