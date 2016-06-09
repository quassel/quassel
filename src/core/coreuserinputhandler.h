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

#ifndef COREUSERINPUTHANDLER_H
#define COREUSERINPUTHANDLER_H

#include "corebasichandler.h"
#include "corenetwork.h"

class Cipher;
class Server;

class CoreUserInputHandler : public CoreBasicHandler
{
    Q_OBJECT

public:
    CoreUserInputHandler(CoreNetwork *parent = 0);
    inline CoreNetwork *coreNetwork() const { return qobject_cast<CoreNetwork *>(parent()); }

    void handleUserInput(const BufferInfo &bufferInfo, const QString &text);
    int lastParamOverrun(const QString &cmd, const QList<QByteArray> &params);

public slots:
    void handleAway(const BufferInfo &bufferInfo, const QString &text);
    void handleBan(const BufferInfo &bufferInfo, const QString &text);
    void handleUnban(const BufferInfo &bufferInfo, const QString &text);
    void handleCtcp(const BufferInfo &bufferInfo, const QString &text);
    void handleDelkey(const BufferInfo &bufferInfo, const QString &text);
    void handleDeop(const BufferInfo& bufferInfo, const QString &nicks);
    void handleDehalfop(const BufferInfo& bufferInfo, const QString &nicks);
    void handleDevoice(const BufferInfo& bufferInfo, const QString &nicks);
    void handleInvite(const BufferInfo &bufferInfo, const QString &text);
    void handleJoin(const BufferInfo &bufferInfo, const QString &text);
    void handleKeyx(const BufferInfo &bufferInfo, const QString &text);
    void handleKick(const BufferInfo &bufferInfo, const QString &text);
    void handleKill(const BufferInfo &bufferInfo, const QString &text);
    void handleList(const BufferInfo &bufferInfo, const QString &text);
    void handleMe(const BufferInfo &bufferInfo, const QString &text);
    void handleMode(const BufferInfo &bufferInfo, const QString &text);
    void handleMsg(const BufferInfo &bufferInfo, const QString &text);
    void handleNick(const BufferInfo &bufferInfo, const QString &text);
    void handleNotice(const BufferInfo &bufferInfo, const QString &text);
    void handleOper(const BufferInfo &bufferInfo, const QString &text);
    void handleOp(const BufferInfo& bufferInfo, const QString &nicks);
    void handleHalfop(const BufferInfo& bufferInfo, const QString &nicks);
    void handlePart(const BufferInfo &bufferInfo, const QString &text);
    void handlePing(const BufferInfo &bufferInfo, const QString &text);
    void handlePrint(const BufferInfo &bufferInfo, const QString &text);
    void handleQuery(const BufferInfo &bufferInfo, const QString &text);
    void handleQuit(const BufferInfo &bufferInfo, const QString &text);
    void handleQuote(const BufferInfo &bufferInfo, const QString &text);
    void handleSay(const BufferInfo &bufferInfo, const QString &text);
    void handleSetkey(const BufferInfo &bufferInfo, const QString &text);
    void handleShowkey(const BufferInfo &bufferInfo, const QString &text);
    void handleTopic(const BufferInfo &bufferInfo, const QString &text);
    void handleVoice(const BufferInfo &bufferInfo, const QString &text);
    void handleWait(const BufferInfo &bufferInfo, const QString &text);
    void handleWho(const BufferInfo &bufferInfo, const QString &text);
    void handleWhois(const BufferInfo &bufferInfo, const QString &text);
    void handleWhowas(const BufferInfo &bufferInfo, const QString &text);

    void defaultHandler(QString cmd, const BufferInfo &bufferInfo, const QString &text);

    /**
     * Send a QUIT to the IRC server, optionally skipping the command queue.
     *
     * @param reason          Reason for quitting, often displayed to other IRC clients
     * @param forceImmediate  Immediately quit, skipping queue of other commands
     */
    void issueQuit(const QString &reason, bool forceImmediate = false);
    void issueAway(const QString &msg, bool autoCheck = true);

protected:
    void timerEvent(QTimerEvent *event);

private:
    void doMode(const BufferInfo& bufferInfo, const QChar &addOrRemove, const QChar &mode, const QString &nickList);
    void banOrUnban(const BufferInfo &bufferInfo, const QString &text, bool ban);
    void putPrivmsg(const QString &target, const QString &message, std::function<QByteArray(const QString &, const QString &)> encodeFunc, Cipher *cipher = 0);

#ifdef HAVE_QCA2
    QByteArray encrypt(const QString &target, const QByteArray &message, bool *didEncrypt = 0) const;
#endif

    struct Command {
        BufferInfo bufferInfo;
        QString command;
        Command(const BufferInfo &info, const QString &command) : bufferInfo(info), command(command) {}
        Command() {}
    };

    QHash<int, Command> _delayedCommands;
};


#endif
