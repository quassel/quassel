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

#include "coreuserinputhandler.h"

#include "util.h"

#include "ctcpparser.h"

#include <QRegExp>

#ifdef HAVE_QCA2
#  include "cipher.h"
#endif

CoreUserInputHandler::CoreUserInputHandler(CoreNetwork *parent)
    : CoreBasicHandler(parent)
{
}


void CoreUserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg)
{
    if (msg.isEmpty())
        return;

    AliasManager::CommandList list = coreSession()->aliasManager().processInput(bufferInfo, msg);

    for (int i = 0; i < list.count(); i++) {
        QString cmd = list.at(i).second.section(' ', 0, 0).remove(0, 1).toUpper();
        QString payload = list.at(i).second.section(' ', 1);
        handle(cmd, Q_ARG(BufferInfo, list.at(i).first), Q_ARG(QString, payload));
    }
}


// ====================
//  Public Slots
// ====================
void CoreUserInputHandler::handleAway(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    if (msg.startsWith("-all")) {
        if (msg.length() == 4) {
            coreSession()->globalAway();
            return;
        }
        Q_ASSERT(msg.length() > 4);
        if (msg[4] == ' ') {
            coreSession()->globalAway(msg.mid(5));
            return;
        }
    }
    issueAway(msg);
}


void CoreUserInputHandler::issueAway(const QString &msg, bool autoCheck)
{
    QString awayMsg = msg;
    IrcUser *me = network()->me();

    // if there is no message supplied we have to check if we are already away or not
    if (autoCheck && msg.isEmpty()) {
        if (me && !me->isAway()) {
            Identity *identity = network()->identityPtr();
            if (identity) {
                awayMsg = identity->awayReason();
            }
            if (awayMsg.isEmpty()) {
                awayMsg = tr("away");
            }
        }
    }
    if (me)
        me->setAwayMessage(awayMsg);

    putCmd("AWAY", serverEncode(awayMsg));
}


void CoreUserInputHandler::handleBan(const BufferInfo &bufferInfo, const QString &msg)
{
    banOrUnban(bufferInfo, msg, true);
}


void CoreUserInputHandler::handleUnban(const BufferInfo &bufferInfo, const QString &msg)
{
    banOrUnban(bufferInfo, msg, false);
}


void CoreUserInputHandler::banOrUnban(const BufferInfo &bufferInfo, const QString &msg, bool ban)
{
    QString banChannel;
    QString banUser;

    QStringList params = msg.split(" ");

    if (!params.isEmpty() && isChannelName(params[0])) {
        banChannel = params.takeFirst();
    }
    else if (bufferInfo.type() == BufferInfo::ChannelBuffer) {
        banChannel = bufferInfo.bufferName();
    }
    else {
        emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: channel unknown in command: /BAN %1").arg(msg));
        return;
    }

    if (!params.isEmpty() && !params.contains("!") && network()->ircUser(params[0])) {
        IrcUser *ircuser = network()->ircUser(params[0]);
        // generalizedHost changes <nick> to  *!ident@*.sld.tld.
        QString generalizedHost = ircuser->host();
        if (generalizedHost.isEmpty()) {
            emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: host unknown in command: /BAN %1").arg(msg));
            return;
        }

        static QRegExp ipAddress("\\d+\\.\\d+\\.\\d+\\.\\d+");
        if (ipAddress.exactMatch(generalizedHost))    {
            int lastDotPos = generalizedHost.lastIndexOf('.') + 1;
            generalizedHost.replace(lastDotPos, generalizedHost.length() - lastDotPos, '*');
        }
        else if (generalizedHost.lastIndexOf(".") != -1 && generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1) != -1) {
            int secondLastPeriodPosition = generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1);
            generalizedHost.replace(0, secondLastPeriodPosition, "*");
        }
        banUser = QString("*!%1@%2").arg(ircuser->user(), generalizedHost);
    }
    else {
        banUser = params.join(" ");
    }

    QString banMode = ban ? "+b" : "-b";
    QString banMsg = QString("MODE %1 %2 %3").arg(banChannel, banMode, banUser);
    emit putRawLine(serverEncode(banMsg));
}


void CoreUserInputHandler::handleCtcp(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)

    QString nick = msg.section(' ', 0, 0);
    QString ctcpTag = msg.section(' ', 1, 1).toUpper();
    if (ctcpTag.isEmpty())
        return;

    QString message = msg.section(' ', 2);
    QString verboseMessage = tr("sending CTCP-%1 request to %2").arg(ctcpTag).arg(nick);

    if (ctcpTag == "PING") {
        message = QString::number(QDateTime::currentMSecsSinceEpoch());
    }

    // FIXME make this a proper event
    coreNetwork()->coreSession()->ctcpParser()->query(coreNetwork(), nick, ctcpTag, message);
    emit displayMsg(Message::Action, BufferInfo::StatusBuffer, "", verboseMessage, network()->myNick());
}


void CoreUserInputHandler::handleDelkey(const BufferInfo &bufferInfo, const QString &msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin."));
        return;
    }

    QStringList parms = msg.split(' ', QString::SkipEmptyParts);

    if (parms.isEmpty() && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());

    if (parms.isEmpty()) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname,
            tr("[usage] /delkey <nick|channel> deletes the encryption key for nick or channel or just /delkey when in a channel or query."));
        return;
    }

    QString target = parms.at(0);

    if (network()->cipherKey(target).isEmpty()) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("No key has been set for %1.").arg(target));
        return;
    }

    network()->setCipherKey(target, QByteArray());
    emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("The key for %1 has been deleted.").arg(target));

#else
    Q_UNUSED(msg)
    emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: Setting an encryption key requires Quassel to have been built "
                                                                    "with support for the Qt Cryptographic Architecture (QCA2) library. "
                                                                    "Contact your distributor about a Quassel package with QCA2 "
                                                                    "support, or rebuild Quassel with QCA2 present."));
#endif
}

void CoreUserInputHandler::doMode(const BufferInfo &bufferInfo, const QChar& addOrRemove, const QChar& mode, const QString &nicks)
{
    QString m;
    bool isNumber;
    int maxModes = network()->support("MODES").toInt(&isNumber);
    if (!isNumber || maxModes == 0) maxModes = 1;

    QStringList nickList;
    if (nicks == "*" && bufferInfo.type() == BufferInfo::ChannelBuffer) { // All users in channel
        const QList<IrcUser*> users = network()->ircChannel(bufferInfo.bufferName())->ircUsers();
        foreach(IrcUser *user, users) {
            if ((addOrRemove == '+' && !network()->ircChannel(bufferInfo.bufferName())->userModes(user).contains(mode))
                || (addOrRemove == '-' && network()->ircChannel(bufferInfo.bufferName())->userModes(user).contains(mode)))
                nickList.append(user->nick());
        }
    } else {
        nickList = nicks.split(' ', QString::SkipEmptyParts);
    }

    if (nickList.count() == 0) return;

    while (!nickList.isEmpty()) {
        int amount = qMin(nickList.count(), maxModes);
        QString m = addOrRemove; for(int i = 0; i < amount; i++) m += mode;
        QStringList params;
        params << bufferInfo.bufferName() << m;
        for(int i = 0; i < amount; i++) params << nickList.takeFirst();
        emit putCmd("MODE", serverEncode(params));
    }
}


void CoreUserInputHandler::handleDeop(const BufferInfo &bufferInfo, const QString &nicks)
{
    doMode(bufferInfo, '-', 'o', nicks);
}


void CoreUserInputHandler::handleDehalfop(const BufferInfo &bufferInfo, const QString &nicks)
{
    doMode(bufferInfo, '-', 'h', nicks);
}


void CoreUserInputHandler::handleDevoice(const BufferInfo &bufferInfo, const QString &nicks)
{
    doMode(bufferInfo, '-', 'v', nicks);
}

void CoreUserInputHandler::handleHalfop(const BufferInfo &bufferInfo, const QString &nicks)
{
    doMode(bufferInfo, '+', 'h', nicks);
}

void CoreUserInputHandler::handleOp(const BufferInfo &bufferInfo, const QString &nicks) {
  doMode(bufferInfo, '+', 'o', nicks);
}


void CoreUserInputHandler::handleInvite(const BufferInfo &bufferInfo, const QString &msg)
{
    QStringList params;
    params << msg << bufferInfo.bufferName();
    emit putCmd("INVITE", serverEncode(params));
}


void CoreUserInputHandler::handleJoin(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo);

    // trim spaces before chans or keys
    QString sane_msg = msg;
    sane_msg.replace(QRegExp(", +"), ",");
    QStringList params = sane_msg.trimmed().split(" ");

    QStringList chans = params[0].split(",", QString::SkipEmptyParts);
    QStringList keys;
    if (params.count() > 1)
        keys = params[1].split(",");

    int i;
    for (i = 0; i < chans.count(); i++) {
        if (!network()->isChannelName(chans[i]))
            chans[i].prepend('#');

        if (i < keys.count()) {
            network()->addChannelKey(chans[i], keys[i]);
        }
        else {
            network()->removeChannelKey(chans[i]);
        }
    }

    static const char *cmd = "JOIN";
    i = 0;
    QStringList joinChans, joinKeys;
    int slicesize = chans.count();
    QList<QByteArray> encodedParams;

    // go through all to-be-joined channels and (re)build the join list
    while (i < chans.count()) {
        joinChans.append(chans.at(i));
        if (i < keys.count())
            joinKeys.append(keys.at(i));

        // if the channel list we built so far either contains all requested channels or exceeds
        // the desired amount of channels in this slice, try to send what we have so far
        if (++i == chans.count() || joinChans.count() >= slicesize) {
            params.clear();
            params.append(joinChans.join(","));
            params.append(joinKeys.join(","));
            encodedParams = serverEncode(params);
            // check if it fits in one command
            if (lastParamOverrun(cmd, encodedParams) == 0) {
                emit putCmd(cmd, encodedParams);
            }
            else if (slicesize > 1) {
                // back to start of slice, try again with half the amount of channels
                i -= slicesize;
                slicesize /= 2;
            }
            joinChans.clear();
            joinKeys.clear();
        }
    }
}


void CoreUserInputHandler::handleKeyx(const BufferInfo &bufferInfo, const QString &msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin."));
        return;
    }

    QStringList parms = msg.split(' ', QString::SkipEmptyParts);

    if (parms.count() == 0 && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());
    else if (parms.count() != 1) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname,
            tr("[usage] /keyx [<nick>] Initiates a DH1080 key exchange with the target."));
        return;
    }

    QString target = parms.at(0);

    if (network()->isChannelName(target)) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("It is only possible to exchange keys in a query buffer."));
        return;
    }

    Cipher *cipher = network()->cipher(target);
    if (!cipher) // happens when there is no CoreIrcChannel for the target
        return;

    QByteArray pubKey = cipher->initKeyExchange();
    if (pubKey.isEmpty())
        emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Failed to initiate key exchange with %1.").arg(target));
    else {
        QList<QByteArray> params;
        params << serverEncode(target) << serverEncode("DH1080_INIT ") + pubKey;
        emit putCmd("NOTICE", params);
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("Initiated key exchange with %1.").arg(target));
    }
#else
    Q_UNUSED(msg)
    emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: Setting an encryption key requires Quassel to have been built "
                                                                "with support for the Qt Cryptographic Architecture (QCA) library. "
                                                                "Contact your distributor about a Quassel package with QCA "
                                                                "support, or rebuild Quassel with QCA present."));
#endif
}


void CoreUserInputHandler::handleKick(const BufferInfo &bufferInfo, const QString &msg)
{
    QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
    QString reason = msg.section(' ', 1, -1, QString::SectionSkipEmpty).trimmed();
    if (reason.isEmpty())
        reason = network()->identityPtr()->kickReason();

    QList<QByteArray> params;
    params << serverEncode(bufferInfo.bufferName()) << serverEncode(nick) << channelEncode(bufferInfo.bufferName(), reason);
    emit putCmd("KICK", params);
}


void CoreUserInputHandler::handleKill(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
    QString pass = msg.section(' ', 1, -1, QString::SectionSkipEmpty);
    QList<QByteArray> params;
    params << serverEncode(nick) << serverEncode(pass);
    emit putCmd("KILL", params);
}


void CoreUserInputHandler::handleList(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("LIST", serverEncode(msg.split(' ', QString::SkipEmptyParts)));
}


void CoreUserInputHandler::handleMe(const BufferInfo &bufferInfo, const QString &msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer
    // FIXME make this a proper event
    coreNetwork()->coreSession()->ctcpParser()->query(coreNetwork(), bufferInfo.bufferName(), "ACTION", msg);
    emit displayMsg(Message::Action, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}


void CoreUserInputHandler::handleMode(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)

    QStringList params = msg.split(' ', QString::SkipEmptyParts);
    // if the first argument is neither a channel nor us (user modes are only to oneself) the current buffer is assumed to be the target
    if (!params.isEmpty()) {
        if (!network()->isChannelName(params[0]) && !network()->isMyNick(params[0]))
            params.prepend(bufferInfo.bufferName());
        if (network()->isMyNick(params[0]) && params.count() == 2)
            network()->updateIssuedModes(params[1]);
        if (params[0] == "-reset" && params.count() == 1) {
            // FIXME: give feedback to the user (I don't want to add new strings right now)
            network()->resetPersistentModes();
            return;
        }
    }

    // TODO handle correct encoding for buffer modes (channelEncode())
    emit putCmd("MODE", serverEncode(params));
}


// TODO: show privmsgs
void CoreUserInputHandler::handleMsg(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo);
    if (!msg.contains(' '))
        return;

    QString target = msg.section(' ', 0, 0);
    QString msgSection = msg.section(' ', 1);

    std::function<QByteArray(const QString &, const QString &)> encodeFunc = [this] (const QString &target, const QString &message) -> QByteArray {
        return userEncode(target, message);
    };

#ifdef HAVE_QCA2
    putPrivmsg(target, msgSection, encodeFunc, network()->cipher(target));
#else
    putPrivmsg(target, msgSection, encodeFunc);
#endif
}


void CoreUserInputHandler::handleNick(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    QString nick = msg.section(' ', 0, 0);
    emit putCmd("NICK", serverEncode(nick));
}


void CoreUserInputHandler::handleNotice(const BufferInfo &bufferInfo, const QString &msg)
{
    QString bufferName = msg.section(' ', 0, 0);
    QString payload = msg.section(' ', 1);
    QList<QByteArray> params;
    params << serverEncode(bufferName) << channelEncode(bufferInfo.bufferName(), payload);
    emit putCmd("NOTICE", params);
    emit displayMsg(Message::Notice, typeByTarget(bufferName), bufferName, payload, network()->myNick(), Message::Self);
}



void CoreUserInputHandler::handleOper(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putRawLine(serverEncode(QString("OPER %1").arg(msg)));
}


void CoreUserInputHandler::handlePart(const BufferInfo &bufferInfo, const QString &msg)
{
    QList<QByteArray> params;
    QString partReason;

    // msg might contain either a channel name and/or a reaon, so we have to check if the first word is a known channel
    QString channelName = msg.section(' ', 0, 0);
    if (channelName.isEmpty() || !network()->ircChannel(channelName)) {
        channelName = bufferInfo.bufferName();
        partReason = msg;
    }
    else {
        partReason = msg.mid(channelName.length() + 1);
    }

    if (partReason.isEmpty())
        partReason = network()->identityPtr()->partReason();

    params << serverEncode(channelName) << channelEncode(bufferInfo.bufferName(), partReason);
    emit putCmd("PART", params);
}


void CoreUserInputHandler::handlePing(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)

    QString param = msg;
    if (param.isEmpty())
        param = QTime::currentTime().toString("hh:mm:ss.zzz");

    // Take priority so this won't get stuck behind other queued messages.
    putCmd("PING", serverEncode(param), QByteArray(), true);
}


void CoreUserInputHandler::handlePrint(const BufferInfo &bufferInfo, const QString &msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer

    QByteArray encMsg = channelEncode(bufferInfo.bufferName(), msg);
    emit displayMsg(Message::Info, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}


// TODO: implement queries
void CoreUserInputHandler::handleQuery(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    QString target = msg.section(' ', 0, 0);
    QString message = msg.section(' ', 1);
    if (message.isEmpty())
        emit displayMsg(Message::Server, BufferInfo::QueryBuffer, target, tr("Starting query with %1").arg(target), network()->myNick(), Message::Self);
    else
        emit displayMsg(Message::Plain, BufferInfo::QueryBuffer, target, message, network()->myNick(), Message::Self);
    handleMsg(bufferInfo, msg);
}


void CoreUserInputHandler::handleQuit(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    network()->disconnectFromIrc(true, msg);
}


void CoreUserInputHandler::issueQuit(const QString &reason, bool forceImmediate)
{
    // If needing an immediate QUIT (e.g. core shutdown), prepend this to the queue
    emit putCmd("QUIT", serverEncode(reason), QByteArray(), forceImmediate);
}


void CoreUserInputHandler::handleQuote(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putRawLine(serverEncode(msg));
}


void CoreUserInputHandler::handleSay(const BufferInfo &bufferInfo, const QString &msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer

    std::function<QByteArray(const QString &, const QString &)> encodeFunc = [this] (const QString &target, const QString &message) -> QByteArray {
        return channelEncode(target, message);
    };

#ifdef HAVE_QCA2
    putPrivmsg(bufferInfo.bufferName(), msg, encodeFunc, network()->cipher(bufferInfo.bufferName()));
#else
    putPrivmsg(bufferInfo.bufferName(), msg, encodeFunc);
#endif
    emit displayMsg(Message::Plain, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}


void CoreUserInputHandler::handleSetkey(const BufferInfo &bufferInfo, const QString &msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin."));
        return;
    }

    QStringList parms = msg.split(' ', QString::SkipEmptyParts);

    if (parms.count() == 1 && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());
    else if (parms.count() != 2) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname,
            tr("[usage] /setkey <nick|channel> <key> sets the encryption key for nick or channel. "
               "/setkey <key> when in a channel or query buffer sets the key for it."));
        return;
    }

    QString target = parms.at(0);
    QByteArray key = parms.at(1).toLocal8Bit();
    network()->setCipherKey(target, key);

    emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("The key for %1 has been set.").arg(target));
#else
    Q_UNUSED(msg)
    emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: Setting an encryption key requires Quassel to have been built "
                                                                "with support for the Qt Cryptographic Architecture (QCA) library. "
                                                                "Contact your distributor about a Quassel package with QCA "
                                                                "support, or rebuild Quassel with QCA present."));
#endif
}


void CoreUserInputHandler::handleShowkey(const BufferInfo &bufferInfo, const QString &msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin."));
        return;
    }

    QStringList parms = msg.split(' ', QString::SkipEmptyParts);

    if (parms.isEmpty() && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());

    if (parms.isEmpty()) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("[usage] /showkey <nick|channel> shows the encryption key for nick or channel or just /showkey when in a channel or query."));
        return;
    }

    QString target = parms.at(0);
    QByteArray key = network()->cipherKey(target);

    if (key.isEmpty()) {
        emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("No key has been set for %1.").arg(target));
        return;
    }

    emit displayMsg(Message::Info, typeByTarget(bufname), bufname, tr("The key for %1 is %2:%3").arg(target, network()->cipherUsesCBC(target) ? "CBC" : "ECB", QString(key)));

#else
    Q_UNUSED(msg)
    emit displayMsg(Message::Error, typeByTarget(bufname), bufname, tr("Error: Setting an encryption key requires Quassel to have been built "
                                                                    "with support for the Qt Cryptographic Architecture (QCA2) library. "
                                                                    "Contact your distributor about a Quassel package with QCA2 "
                                                                    "support, or rebuild Quassel with QCA2 present."));
#endif
}


void CoreUserInputHandler::handleTopic(const BufferInfo &bufferInfo, const QString &msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;

    QList<QByteArray> params;
    params << serverEncode(bufferInfo.bufferName());

    if (!msg.isEmpty()) {
#   ifdef HAVE_QCA2
        params << encrypt(bufferInfo.bufferName(), channelEncode(bufferInfo.bufferName(), msg));
#   else
        params << channelEncode(bufferInfo.bufferName(), msg);
#   endif
    }

    emit putCmd("TOPIC", params);
}


void CoreUserInputHandler::handleVoice(const BufferInfo &bufferInfo, const QString &msg)
{
    QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
    QString m = "+"; for (int i = 0; i < nicks.count(); i++) m += 'v';
    QStringList params;
    params << bufferInfo.bufferName() << m << nicks;
    emit putCmd("MODE", serverEncode(params));
}


void CoreUserInputHandler::handleWait(const BufferInfo &bufferInfo, const QString &msg)
{
    int splitPos = msg.indexOf(';');
    if (splitPos <= 0)
        return;

    bool ok;
    int delay = msg.left(splitPos).trimmed().toInt(&ok);
    if (!ok)
        return;

    delay *= 1000;

    QString command = msg.mid(splitPos + 1).trimmed();
    if (command.isEmpty())
        return;

    _delayedCommands[startTimer(delay)] = Command(bufferInfo, command);
}


void CoreUserInputHandler::handleWho(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHO", serverEncode(msg.split(' ')));
}


void CoreUserInputHandler::handleWhois(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHOIS", serverEncode(msg.split(' ')));
}


void CoreUserInputHandler::handleWhowas(const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHOWAS", serverEncode(msg.split(' ')));
}


void CoreUserInputHandler::defaultHandler(QString cmd, const BufferInfo &bufferInfo, const QString &msg)
{
    Q_UNUSED(bufferInfo);
    emit putCmd(serverEncode(cmd.toUpper()), serverEncode(msg.split(" ")));
}


void CoreUserInputHandler::putPrivmsg(const QString &target, const QString &message, std::function<QByteArray(const QString &, const QString &)> encodeFunc, Cipher *cipher)
{
    Q_UNUSED(cipher);
    QString cmd("PRIVMSG");
    QByteArray targetEnc = serverEncode(target);

    std::function<QList<QByteArray>(QString &)> cmdGenerator = [&] (QString &splitMsg) -> QList<QByteArray> {
        QByteArray splitMsgEnc = encodeFunc(target, splitMsg);

#ifdef HAVE_QCA2
        if (cipher && !cipher->key().isEmpty() && !splitMsg.isEmpty()) {
            cipher->encrypt(splitMsgEnc);
        }
#endif
        return QList<QByteArray>() << targetEnc << splitMsgEnc;
    };

    putCmd(cmd, network()->splitMessage(cmd, message, cmdGenerator));
}


// returns 0 if the message will not be chopped by the irc server or number of chopped bytes if message is too long
int CoreUserInputHandler::lastParamOverrun(const QString &cmd, const QList<QByteArray> &params)
{
    // the server will pass our message truncated to 512 bytes including CRLF with the following format:
    // ":prefix COMMAND param0 param1 :lastparam"
    // where prefix = "nickname!user@host"
    // that means that the last message can be as long as:
    // 512 - nicklen - userlen - hostlen - commandlen - sum(param[0]..param[n-1])) - 2 (for CRLF) - 4 (":!@" + 1space between prefix and command) - max(paramcount - 1, 0) (space for simple params) - 2 (space and colon for last param)
    IrcUser *me = network()->me();
    int maxLen = 480 - cmd.toLatin1().count(); // educated guess in case we don't know us (yet?)

    if (me)
        maxLen = 512 - serverEncode(me->nick()).count() - serverEncode(me->user()).count() - serverEncode(me->host()).count() - cmd.toLatin1().count() - 6;

    if (!params.isEmpty()) {
        for (int i = 0; i < params.count() - 1; i++) {
            maxLen -= (params[i].count() + 1);
        }
        maxLen -= 2; // " :" last param separator;

        if (params.last().count() > maxLen) {
            return params.last().count() - maxLen;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
}


#ifdef HAVE_QCA2
QByteArray CoreUserInputHandler::encrypt(const QString &target, const QByteArray &message_, bool *didEncrypt) const
{
    if (didEncrypt)
        *didEncrypt = false;

    if (message_.isEmpty())
        return message_;

    if (!Cipher::neededFeaturesAvailable())
        return message_;

    Cipher *cipher = network()->cipher(target);
    if (!cipher || cipher->key().isEmpty())
        return message_;

    QByteArray message = message_;
    bool result = cipher->encrypt(message);
    if (didEncrypt)
        *didEncrypt = result;

    return message;
}


#endif

void CoreUserInputHandler::timerEvent(QTimerEvent *event)
{
    if (!_delayedCommands.contains(event->timerId())) {
        QObject::timerEvent(event);
        return;
    }
    BufferInfo bufferInfo = _delayedCommands[event->timerId()].bufferInfo;
    QString rawCommand = _delayedCommands[event->timerId()].command;
    _delayedCommands.remove(event->timerId());
    event->accept();

    // the stored command might be the result of an alias expansion, so we need to split it up again
    QStringList commands = rawCommand.split(QRegExp("; ?"));
    foreach(QString command, commands) {
        handleUserInput(bufferInfo, command);
    }
}
