/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef COREBASICHANDLER_H
#define COREBASICHANDLER_H

#include <QString>
#include <QStringList>

#include "basichandler.h"
#include "message.h"

#include "corenetwork.h"

class CoreSession;

class CoreBasicHandler : public BasicHandler
{
    Q_OBJECT

public:
    CoreBasicHandler(CoreNetwork *parent = 0);

    QString serverDecode(const QByteArray &string);
    QStringList serverDecode(const QList<QByteArray> &stringlist);
    QString channelDecode(const QString &bufferName, const QByteArray &string);
    QStringList channelDecode(const QString &bufferName, const QList<QByteArray> &stringlist);
    QString userDecode(const QString &userNick, const QByteArray &string);
    QStringList userDecode(const QString &userNick, const QList<QByteArray> &stringlist);

    QByteArray serverEncode(const QString &string);
    QList<QByteArray> serverEncode(const QStringList &stringlist);
    QByteArray channelEncode(const QString &bufferName, const QString &string);
    QList<QByteArray> channelEncode(const QString &bufferName, const QStringList &stringlist);
    QByteArray userEncode(const QString &userNick, const QString &string);
    QList<QByteArray> userEncode(const QString &userNick, const QStringList &stringlist);

signals:
    void displayMsg(Message::Type, BufferInfo::Type, const QString &target, const QString &text, const QString &sender = "", Message::Flags flags = Message::None);

    /**
     * Sends the raw (encoded) line, adding to the queue if needed, optionally with higher priority.
     *
     * @see CoreNetwork::putRawLine()
     */
    void putRawLine(const QByteArray &msg, const bool prepend = false);

    /**
     * Sends the command with encoded parameters, with optional prefix or high priority.
     *
     * @see CoreNetwork::putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray(), const bool prepend = false)
     */
    void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray(), const bool prepend = false);

    /**
     * Sends the command for each set of encoded parameters, with optional prefix or high priority.
     *
     * @see CoreNetwork::putCmd(const QString &cmd, const QList<QList<QByteArray>> &params, const QByteArray &prefix = QByteArray(), const bool prepend = false)
     */
    void putCmd(const QString &cmd, const QList<QList<QByteArray>> &params, const QByteArray &prefix = QByteArray(), const bool prepend = false);

protected:
    /**
     * Sends the command with one parameter, with optional prefix or high priority.
     *
     * @param[in] cmd      Command to send, ignoring capitalization
     * @param[in] param    Parameter for the command, encoded within a QByteArray
     * @param[in] prefix   Optional command prefix
     * @param[in] prepend
     * @parmblock
     * If true, the command is prepended into the start of the queue, otherwise, it's appended to
     * the end.  This should be used sparingly, for if either the core or the IRC server cannot
     * maintain PING/PONG replies, the other side will close the connection.
     * @endparmblock
     */
    void putCmd(const QString &cmd, const QByteArray &param, const QByteArray &prefix = QByteArray(), const bool prepend = false);

    inline CoreNetwork *network() const { return _network; }
    inline CoreSession *coreSession() const { return _network->coreSession(); }

    BufferInfo::Type typeByTarget(const QString &target) const;

private:
    CoreNetwork *_network;
};


#endif
