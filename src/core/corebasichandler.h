/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QString>
#include <QStringList>

#include "irc/ircmessage.h"
#include "basichandler.h"
#include "corenetwork.h"
#include "message.h"

class CoreSession;

class CoreBasicHandler : public BasicHandler
{
    Q_OBJECT

public:
    CoreBasicHandler(CoreNetwork* parent = nullptr);

    QString serverDecode(const QByteArray& string);
    QStringList serverDecode(const QList<QByteArray>& stringlist);
    QString channelDecode(const QString& bufferName, const QByteArray& string);
    QStringList channelDecode(const QString& bufferName, const QList<QByteArray>& stringlist);
    QString userDecode(const QString& userNick, const QByteArray& string);
    QStringList userDecode(const QString& userNick, const QList<QByteArray>& stringlist);

    QByteArray serverEncode(const QString& string);
    QList<QByteArray> serverEncode(const QStringList& stringlist);
    QByteArray channelEncode(const QString& bufferName, const QString& string);
    QList<QByteArray> channelEncode(const QString& bufferName, const QStringList& stringlist);
    QByteArray userEncode(const QString& userNick, const QString& string);
    QList<QByteArray> userEncode(const QString& userNick, const QStringList& stringlist);

signals:
    void sendMessage(const IrcMessage& message, bool prepend = false);
    void displayMessage(const IrcMessage& message);
    void displayMsg(const NetworkInternalMessage& msg);

    /**
     * Sends the raw (encoded) line, adding to the queue if needed, optionally with higher priority.
     *
     * @see CoreNetwork::putRawLine()
     */
    void putRawLine(const QByteArray& msg, bool prepend = false);
protected:
    inline CoreNetwork* network() const { return _network; }
    inline CoreSession* coreSession() const { return _network->coreSession(); }

    BufferInfo::Type typeByTarget(const QString& target) const;

private:
    CoreNetwork* _network;
};
