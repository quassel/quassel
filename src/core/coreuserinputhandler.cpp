/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <QRegularExpression>

#include "ctcpparser.h"
#include "util.h"

#ifdef HAVE_QCA2
#    include "cipher.h"
#endif

CoreUserInputHandler::CoreUserInputHandler(CoreNetwork* parent)
    : CoreBasicHandler(parent)
{
}

void CoreUserInputHandler::handleUserInput(const BufferInfo& bufferInfo, const QString& msg)
{
    if (msg.isEmpty())
        return;

    AliasManager::CommandList list = coreSession()->aliasManager().processInput(bufferInfo, msg);

    for (int i = 0; i < list.size(); i++) {
        QString cmd = list.at(i).second.section(' ', 0, 0).remove(0, 1).toUpper();
        QString payload = list.at(i).second.section(' ', 1);
        BufferInfo bufferInfoArg = list.at(i).first;
        handle(cmd, QGenericArgument("BufferInfo", &bufferInfoArg), QGenericArgument("QString", &payload));
    }
}

// ====================
//  Public Slots
// ====================
void CoreUserInputHandler::handleAway(const BufferInfo& bufferInfo, const QString& msg, const bool skipFormatting)
{
    Q_UNUSED(bufferInfo)
    if (msg.startsWith("-all")) {
        if (msg.length() == 4) {
            coreSession()->globalAway(QString(), skipFormatting);
            return;
        }
        Q_ASSERT(msg.length() > 4);
        if (msg[4] == ' ') {
            coreSession()->globalAway(msg.mid(5), skipFormatting);
            return;
        }
    }
    issueAway(msg, true /* force away */, skipFormatting);
}

void CoreUserInputHandler::issueAway(const QString& msg, bool autoCheck, const bool skipFormatting)
{
    QString awayMsg = msg;
    IrcUser* me = network()->ircUser(network()->myNick());

    // Only apply timestamp formatting when requested
    // This avoids re-processing any existing away message when the core restarts, so chained escape
    // percent signs won't get down-processed.
    if (!skipFormatting) {
        // Apply the timestamp formatting to the away message (if empty, nothing will happen)
        awayMsg = formatCurrentDateTimeInString(awayMsg);
    }

    // if there is no message supplied we have to check if we are already away or not
    if (autoCheck && msg.isEmpty()) {
        if (me && !me->away()) {
            Identity* identity = network()->identityPtr();
            if (identity) {
                awayMsg = formatCurrentDateTimeInString(identity->awayReason());
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

void CoreUserInputHandler::handleBan(const BufferInfo& bufferInfo, const QString& msg)
{
    banOrUnban(bufferInfo, msg, true);
}

void CoreUserInputHandler::handleUnban(const BufferInfo& bufferInfo, const QString& msg)
{
    banOrUnban(bufferInfo, msg, false);
}

void CoreUserInputHandler::banOrUnban(const BufferInfo& bufferInfo, const QString& msg, bool ban)
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
        emit displayMsg(NetworkInternalMessage(Message::Error,
                                               BufferInfo::StatusBuffer,
                                               "",
                                               QString("Error: channel unknown in command: /BAN %1").arg(msg)));
        return;
    }

    if (!params.isEmpty() && !params.contains("!") && network()->ircUser(params[0])) {
        IrcUser* ircuser = network()->ircUser(params[0]);
        // generalizedHost changes <nick> to  *!ident@*.sld.tld.
        QString generalizedHost = ircuser->host();
        if (generalizedHost.isEmpty()) {
            emit displayMsg(NetworkInternalMessage(Message::Error,
                                                   BufferInfo::StatusBuffer,
                                                   "",
                                                   QString("Error: host unknown in command: /BAN %1").arg(msg)));
            return;
        }

        static QRegularExpression ipAddress(R"(\d+\.\d+\.\d+\.\d+)");
        if (ipAddress.match(generalizedHost).hasMatch()) {
            int lastDotPos = generalizedHost.lastIndexOf('.') + 1;
            generalizedHost.replace(lastDotPos, generalizedHost.length() - lastDotPos, '*');
        }
        else if (generalizedHost.lastIndexOf(".") != -1 && generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".") - 1) != -1) {
            int secondLastPeriodPosition = generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".") - 1);
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

void CoreUserInputHandler::handleCtcp(const BufferInfo& bufferInfo, const QString& msg)
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
    if (!network()->enabledCaps().contains(IrcCap::ECHO_MESSAGE)) {
        emit displayMsg(
            NetworkInternalMessage(Message::Action, BufferInfo::StatusBuffer, "", verboseMessage, network()->myNick(), Message::Flag::Self));
    }
}

void CoreUserInputHandler::handleDelkey(const BufferInfo& bufferInfo, const QString& msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(NetworkInternalMessage(Message::Error,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin.")));
        return;
    }

    QStringList parms = msg.split(' ', Qt::SkipEmptyParts);

    if (parms.isEmpty() && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());

    if (parms.isEmpty()) {
        emit displayMsg(NetworkInternalMessage(
            Message::Info,
            typeByTarget(bufname),
            bufname,
            tr("[usage] /delkey <nick|channel> deletes the encryption key for nick or channel or just /delkey when in a "
               "channel or query.")));
        return;
    }

    QString target = parms.at(0);

    if (network()->cipherKey(target).isEmpty()) {
        emit displayMsg(NetworkInternalMessage(Message::Info, typeByTarget(bufname), bufname, tr("No key has been set for %1.").arg(target)));
        return;
    }

    network()->setCipherKey(target, QByteArray());
    emit displayMsg(NetworkInternalMessage(Message::Info, typeByTarget(bufname), bufname, tr("The key for %1 has been deleted.").arg(target)));

#else
    Q_UNUSED(msg)
    emit displayMsg(NetworkInternalMessage(Message::Error,
                                           typeByTarget(bufname),
                                           bufname,
                                           tr("Error: Setting an encryption key requires Quassel to have been built "
                                              "with support for the Qt Cryptographic Architecture (QCA2) library. "
                                              "Contact your distributor about a Quassel package with QCA2 "
                                              "support, or rebuild Quassel with QCA2 present.")));
#endif
}

void CoreUserInputHandler::doMode(const BufferInfo& bufferInfo, const QChar& addOrRemove, const QChar& mode, const QString& nicks)
{
    bool isNumber;
    QString modesValue = network()->supports().value("MODES", "1");
    int maxModes = modesValue.toInt(&isNumber);
    if (!isNumber || maxModes <= 0)
        maxModes = 1;

    QStringList nickList;
    if (nicks == "*" && bufferInfo.type() == BufferInfo::ChannelBuffer) {  // All users in channel
        const QStringList nicknames = network()->ircChannel(bufferInfo.bufferName())->userList();
        for (const QString& nick : nicknames) {
            IrcUser* user = network()->ircUser(nick);
            if (user
                && ((addOrRemove == '+' && !network()->ircChannel(bufferInfo.bufferName())->userModes(user).contains(mode))
                    || (addOrRemove == '-' && network()->ircChannel(bufferInfo.bufferName())->userModes(user).contains(mode))))
                nickList.append(nick);
        }
    }
    else {
        nickList = nicks.split(' ', Qt::SkipEmptyParts);
    }

    if (nickList.isEmpty())
        return;

    while (!nickList.isEmpty()) {
        int amount = qMin(nickList.size(), maxModes);
        QString m = addOrRemove;
        for (int i = 0; i < amount; i++)
            m += mode;
        QStringList params;
        params << bufferInfo.bufferName() << m;
        for (int i = 0; i < amount; i++)
            params << nickList.takeFirst();
        emit putCmd("MODE", serverEncode(params));
    }
}

void CoreUserInputHandler::handleDeop(const BufferInfo& bufferInfo, const QString& nicks)
{
    doMode(bufferInfo, '-', 'o', nicks);
}

void CoreUserInputHandler::handleDehalfop(const BufferInfo& bufferInfo, const QString& nicks)
{
    doMode(bufferInfo, '-', 'h', nicks);
}

void CoreUserInputHandler::handleDevoice(const BufferInfo& bufferInfo, const QString& nicks)
{
    doMode(bufferInfo, '-', 'v', nicks);
}

void CoreUserInputHandler::handleHalfop(const BufferInfo& bufferInfo, const QString& nicks)
{
    doMode(bufferInfo, '+', 'h', nicks);
}

void CoreUserInputHandler::handleOp(const BufferInfo& bufferInfo, const QString& nicks)
{
    doMode(bufferInfo, '+', 'o', nicks);
}

void CoreUserInputHandler::handleInvite(const BufferInfo& bufferInfo, const QString& msg)
{
    QStringList params;
    params << msg << bufferInfo.bufferName();
    emit putCmd("INVITE", serverEncode(params));
}

void CoreUserInputHandler::handleJoin(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo);

    // trim spaces before chans or keys
    QString sane_msg = msg;
    sane_msg.replace(QRegularExpression(", +"), ",");
    QStringList params = sane_msg.trimmed().split(" ");

    QStringList chans = params[0].split(",", Qt::SkipEmptyParts);
    QStringList keys;
    if (params.size() > 1)
        keys = params[1].split(",");

    int i;
    for (i = 0; i < chans.size(); i++) {
        if (!network()->isChannelName(chans[i]))
            chans[i].prepend('#');

        if (i < keys.size()) {
            network()->addChannelKey(chans[i], keys[i]);
        }
        else {
            network()->removeChannelKey(chans[i]);
        }
    }

    static const char* cmd = "JOIN";
    i = 0;
    QStringList joinChans, joinKeys;
    int slicesize = chans.size();
    QList<QByteArray> encodedParams;

    // go through all to-be-joined channels and (re)build the join list
    while (i < chans.size()) {
        joinChans.append(chans.at(i));
        if (i < keys.size())
            joinKeys.append(keys.at(i));

        // if the channel list we built so far either contains all requested channels or exceeds
        // the desired amount of channels in this slice, try to send what we have so far
        if (++i == chans.size() || joinChans.size() >= slicesize) {
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

void CoreUserInputHandler::handleKeyx(const BufferInfo& bufferInfo, const QString& msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(NetworkInternalMessage(Message::Error,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin.")));
        return;
    }

    QStringList parms = msg.split(' ', Qt::SkipEmptyParts);

    QString target;
    if (!bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages()) {
        target = bufferInfo.bufferName();  // default is current buffer
    }

    bool wants_cbc = false;  // default
    bool decode_error = false;

    if (parms.size() > 0) {
        if (parms.at(0) == "-cbc") {
            wants_cbc = true;
            if (parms.size() == 2) {
                target = parms.at(1);
            }
            else if (parms.size() > 2) {
                decode_error = true;
            }
        }
        else {
            if (parms.size() == 1) {
                target = parms.at(0);
            }
            else {
                decode_error = true;
            }
        }
    }

    if (decode_error) {
        emit displayMsg(NetworkInternalMessage(Message::Info,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("[usage] /keyx [-cbc] [<nick>] Initiates a DH1080 key exchange with the target.")));
        return;
    }

    if (network()->isChannelName(target)) {
        emit displayMsg(NetworkInternalMessage(Message::Info,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("It is only possible to exchange keys in a query buffer.")));
        return;
    }

    Cipher* cipher = network()->cipher(target);
    if (!cipher)  // happens when there is no CoreIrcChannel for the target
        return;

    QByteArray pubKey = cipher->initKeyExchange(wants_cbc);
    if (pubKey.isEmpty())
        emit displayMsg(
            NetworkInternalMessage(Message::Error, typeByTarget(bufname), bufname, tr("Failed to initiate key exchange with %1.").arg(target)));
    else {
        QList<QByteArray> params;
        params << serverEncode(target) << serverEncode("DH1080_INIT ") + pubKey + (wants_cbc ? " CBC" : "");
        emit putCmd("NOTICE", params);
        emit displayMsg(
            NetworkInternalMessage(Message::Info, typeByTarget(bufname), bufname, tr("Initiated key exchange with %1.").arg(target)));
    }
#else
    Q_UNUSED(msg)
    emit displayMsg(NetworkInternalMessage(Message::Error,
                                           typeByTarget(bufname),
                                           bufname,
                                           tr("Error: Setting an encryption key requires Quassel to have been built "
                                              "with support for the Qt Cryptographic Architecture (QCA) library. "
                                              "Contact your distributor about a Quassel package with QCA2 "
                                              "support, or rebuild Quassel with QCA2 present.")));
#endif
}

void CoreUserInputHandler::handleKick(const BufferInfo& bufferInfo, const QString& msg)
{
    QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
    QString reason = msg.section(' ', 1, -1, QString::SectionSkipEmpty).trimmed();
    if (reason.isEmpty())
        reason = network()->identityPtr()->kickReason();

    QList<QByteArray> params;
    params << serverEncode(bufferInfo.bufferName()) << serverEncode(nick) << channelEncode(bufferInfo.bufferName(), reason);
    emit putCmd("KICK", params);
}

void CoreUserInputHandler::handleKill(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
    QString pass = msg.section(' ', 1, -1, QString::SectionSkipEmpty);
    QList<QByteArray> params;
    params << serverEncode(nick) << serverEncode(pass);
    emit putCmd("KILL", params);
}

void CoreUserInputHandler::handleList(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("LIST", serverEncode(msg.split(' ', Qt::SkipEmptyParts)));
}

void CoreUserInputHandler::handleMe(const BufferInfo& bufferInfo, const QString& msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer
    // FIXME make this a proper event

    // Split apart messages at line feeds.  The IRC protocol uses those to separate commands, so
    // they need to be split into multiple messages.
    QStringList messages = msg.split(QChar::LineFeed);

    for (const auto& message : messages) {
        // Handle each separated message independently
        coreNetwork()->coreSession()->ctcpParser()->query(coreNetwork(), bufferInfo.bufferName(), "ACTION", message);
        if (!network()->enabledCaps().contains(IrcCap::ECHO_MESSAGE)) {
            emit displayMsg(
                NetworkInternalMessage(Message::Action, bufferInfo.type(), bufferInfo.bufferName(), message, network()->myNick(), Message::Self));
        }
    }
}

void CoreUserInputHandler::handleMode(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)

    QStringList params = msg.split(' ', Qt::SkipEmptyParts);
    if (!params.isEmpty()) {
        if (params[0] == "-reset" && params.size() == 1) {
            network()->resetPersistentModes();
            emit displayMsg(NetworkInternalMessage(Message::Info, BufferInfo::StatusBuffer, "", tr("Your persistent modes have been reset.")));
            return;
        }
        if (!network()->isChannelName(params[0]) && !network()->isMyNick(params[0]))
            // If the first argument is neither a channel nor us (user modes are only to oneself)
            // the current buffer is assumed to be the target.
            // If the current buffer returns no name (e.g. status buffer), assume target is us.
            params.prepend(!bufferInfo.bufferName().isEmpty() ? bufferInfo.bufferName() : network()->myNick());
        if (network()->isMyNick(params[0]) && params.size() == 2)
            network()->updateIssuedModes(params[1]);
    }

    // TODO handle correct encoding for buffer modes (channelEncode())
    emit putCmd("MODE", serverEncode(params));
}

// TODO: show privmsgs
void CoreUserInputHandler::handleMsg(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo);
    if (!msg.contains(' '))
        return;

    QString target = msg.section(' ', 0, 0);
    QString msgSection = msg.section(' ', 1);

    std::function<QByteArray(const QString&, const QString&)> encodeFunc =
        [this](const QString& target, const QString& message) -> QByteArray { return userEncode(target, message); };

#ifdef HAVE_QCA2
    putPrivmsg(target, msgSection, encodeFunc, network()->cipher(target));
#else
    putPrivmsg(target, msgSection, encodeFunc);
#endif
}

void CoreUserInputHandler::handleNick(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    QString nick = msg.section(' ', 0, 0);
    emit putCmd("NICK", serverEncode(nick));
}

void CoreUserInputHandler::handleNotice(const BufferInfo& bufferInfo, const QString& msg)
{
    QString bufferName = msg.section(' ', 0, 0);
    QList<QByteArray> params;
    // Split apart messages at line feeds.  The IRC protocol uses those to separate commands, so
    // they need to be split into multiple messages.
    QStringList messages = msg.section(' ', 1).split(QChar::LineFeed);

    for (const auto& message : messages) {
        // Handle each separated message independently
        params.clear();
        params << serverEncode(bufferName) << channelEncode(bufferInfo.bufferName(), message);
        emit putCmd("NOTICE", params);
        if (!network()->enabledCaps().contains(IrcCap::ECHO_MESSAGE)) {
            emit displayMsg(
                NetworkInternalMessage(Message::Notice, typeByTarget(bufferName), bufferName, message, network()->myNick(), Message::Self));
        }
    }
}

void CoreUserInputHandler::handleOper(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putRawLine(serverEncode(QString("OPER %1").arg(msg)));
}

void CoreUserInputHandler::handlePart(const BufferInfo& bufferInfo, const QString& msg)
{
    QList<QByteArray> params;
    QString partReason;

    // msg might contain either a channel name and/or a reason, so we have to check if the first word is a known channel
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

void CoreUserInputHandler::handlePing(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)

    QString param = msg;
    if (param.isEmpty())
        param = QTime::currentTime().toString("hh:mm:ss.zzz");

    // Take priority so this won't get stuck behind other queued messages.
    putCmd("PING", serverEncode(param), {}, {}, true);
}

void CoreUserInputHandler::handlePrint(const BufferInfo& bufferInfo, const QString& msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer

    QByteArray encMsg = channelEncode(bufferInfo.bufferName(), msg);
    emit displayMsg(
        NetworkInternalMessage(Message::Info, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self));
}

void CoreUserInputHandler::handleQuery(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    QString target = msg.section(' ', 0, 0);
    // Split apart messages at line feeds.  The IRC protocol uses those to separate commands, so
    // they need to be split into multiple messages.
    QStringList messages = msg.section(' ', 1).split(QChar::LineFeed);

    for (const auto& message : messages) {
        // Handle each separated message independently
        if (message.isEmpty()) {
            emit displayMsg(NetworkInternalMessage(Message::Server,
                                                   BufferInfo::QueryBuffer,
                                                   target,
                                                   tr("Starting query with %1").arg(target),
                                                   network()->myNick(),
                                                   Message::Self));
            // handleMsg is a no-op if message is empty
        }
        else {
            if (!network()->enabledCaps().contains(IrcCap::ECHO_MESSAGE)) {
                emit displayMsg(
                    NetworkInternalMessage(Message::Plain, BufferInfo::QueryBuffer, target, message, network()->myNick(), Message::Self));
            }
            // handleMsg needs the target specified at the beginning of the message
            handleMsg(bufferInfo, target + " " + message);
        }
    }
}

void CoreUserInputHandler::handleQuit(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    network()->disconnectFromIrc(true, msg);
}

void CoreUserInputHandler::issueQuit(const QString& reason, bool forceImmediate)
{
    // If needing an immediate QUIT (e.g. core shutdown), prepend this to the queue
    emit putCmd("QUIT", serverEncode(reason), {}, {}, forceImmediate);
}

void CoreUserInputHandler::handleQuote(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putRawLine(serverEncode(msg));
}

void CoreUserInputHandler::handleSay(const BufferInfo& bufferInfo, const QString& msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;  // server buffer

    std::function<QByteArray(const QString&, const QString&)> encodeFunc =
        [this](const QString& target, const QString& message) -> QByteArray { return channelEncode(target, message); };

    // Split apart messages at line feeds.  The IRC protocol uses those to separate commands, so
    // they need to be split into multiple messages.
    QStringList messages = msg.split(QChar::LineFeed, Qt::SkipEmptyParts);

    for (const auto& message : messages) {
        // Handle each separated message independently
#ifdef HAVE_QCA2
        putPrivmsg(bufferInfo.bufferName(), message, encodeFunc, network()->cipher(bufferInfo.bufferName()));
#else
        putPrivmsg(bufferInfo.bufferName(), message, encodeFunc);
#endif
        if (!network()->enabledCaps().contains(IrcCap::ECHO_MESSAGE)) {
            emit displayMsg(
                NetworkInternalMessage(Message::Plain, bufferInfo.type(), bufferInfo.bufferName(), message, network()->myNick(), Message::Self));
        }
    }
}

void CoreUserInputHandler::handleSetkey(const BufferInfo& bufferInfo, const QString& msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(NetworkInternalMessage(Message::Error,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin.")));
        return;
    }

    QStringList parms = msg.split(' ', Qt::SkipEmptyParts);

    if (parms.size() == 1 && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());
    else if (parms.size() != 2) {
        emit displayMsg(
            NetworkInternalMessage(Message::Info,
                                   typeByTarget(bufname),
                                   bufname,
                                   tr("[usage] /setkey <nick|channel> <key> sets the encryption key for nick or channel. "
                                      "/setkey <key> when in a channel or query buffer sets the key for it. "
                                      "Prefix <key> by cbc: or ebc: to explicitly set the encryption mode respectively. Default is CBC.")));
        return;
    }

    QString target = parms.at(0);
    QByteArray key = parms.at(1).toLocal8Bit();
    network()->setCipherKey(target, key);

    emit displayMsg(NetworkInternalMessage(Message::Info, typeByTarget(bufname), bufname, tr("The key for %1 has been set.").arg(target)));
#else
    Q_UNUSED(msg)
    emit displayMsg(NetworkInternalMessage(Message::Error,
                                           typeByTarget(bufname),
                                           bufname,
                                           tr("Error: Setting an encryption key requires Quassel to have been built "
                                              "with support for the Qt Cryptographic Architecture (QCA2) library. "
                                              "Contact your distributor about a Quassel package with QCA2 "
                                              "support, or rebuild Quassel with QCA2 present.")));
#endif
}

void CoreUserInputHandler::handleSetname(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("SETNAME", serverEncode(msg));
}

void CoreUserInputHandler::handleShowkey(const BufferInfo& bufferInfo, const QString& msg)
{
    QString bufname = bufferInfo.bufferName().isNull() ? "" : bufferInfo.bufferName();
#ifdef HAVE_QCA2
    if (!bufferInfo.isValid())
        return;

    if (!Cipher::neededFeaturesAvailable()) {
        emit displayMsg(NetworkInternalMessage(Message::Error,
                                               typeByTarget(bufname),
                                               bufname,
                                               tr("Error: QCA provider plugin not found. It is usually provided by the qca-ossl plugin.")));
        return;
    }

    QStringList parms = msg.split(' ', Qt::SkipEmptyParts);

    if (parms.isEmpty() && !bufferInfo.bufferName().isEmpty() && bufferInfo.acceptsRegularMessages())
        parms.prepend(bufferInfo.bufferName());

    if (parms.isEmpty()) {
        emit displayMsg(NetworkInternalMessage(
            Message::Info,
            typeByTarget(bufname),
            bufname,
            tr("[usage] /showkey <nick|channel> shows the encryption key for nick or channel or just /showkey when in a "
               "channel or query.")));
        return;
    }

    QString target = parms.at(0);
    QByteArray key = network()->cipherKey(target);

    if (key.isEmpty()) {
        emit displayMsg(NetworkInternalMessage(Message::Info, typeByTarget(bufname), bufname, tr("No key has been set for %1.").arg(target)));
        return;
    }

    emit displayMsg(NetworkInternalMessage(Message::Info,
                                           typeByTarget(bufname),
                                           bufname,
                                           tr("The key for %1 is (Cipher Mode %2) %3")
                                               .arg(target, network()->cipherUsesCBC(target) ? "CBC" : "ECB", QString(key))));

#else
    Q_UNUSED(msg)
    emit displayMsg(NetworkInternalMessage(Message::Error,
                                           typeByTarget(bufname),
                                           bufname,
                                           tr("Error: Setting an encryption key requires Quassel to have been built "
                                              "with support for the Qt Cryptographic Architecture (QCA2) library. "
                                              "Contact your distributor about a Quassel package with QCA2 "
                                              "support, or rebuild Quassel with QCA2 present.")));
#endif
}

void CoreUserInputHandler::handleTopic(const BufferInfo& bufferInfo, const QString& msg)
{
    if (bufferInfo.bufferName().isEmpty() || !bufferInfo.acceptsRegularMessages())
        return;

    QList<QByteArray> params;
    params << serverEncode(bufferInfo.bufferName());

    if (!msg.isEmpty()) {
#ifdef HAVE_QCA2
        params << encrypt(bufferInfo.bufferName(), channelEncode(bufferInfo.bufferName(), msg));
#else
        params << channelEncode(bufferInfo.bufferName(), msg);
#endif
    }

    emit putCmd("TOPIC", params);
}

void CoreUserInputHandler::handleVoice(const BufferInfo& bufferInfo, const QString& msg)
{
    QStringList nicks = msg.split(' ', Qt::SkipEmptyParts);
    QString m = "+";
    for (int i = 0; i < nicks.size(); i++)
        m += 'v';
    QStringList params;
    params << bufferInfo.bufferName() << m << nicks;
    emit putCmd("MODE", serverEncode(params));
}

void CoreUserInputHandler::handleWait(const BufferInfo& bufferInfo, const QString& msg)
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

void CoreUserInputHandler::handleWho(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHO", serverEncode(msg.split(' ')));
}

void CoreUserInputHandler::handleWhois(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHOIS", serverEncode(msg.split(' ')));
}

void CoreUserInputHandler::handleWhowas(const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo)
    emit putCmd("WHOWAS", serverEncode(msg.split(' ')));
}

void CoreUserInputHandler::defaultHandler(QString cmd, const BufferInfo& bufferInfo, const QString& msg)
{
    Q_UNUSED(bufferInfo);
    emit putCmd(serverEncode(cmd.toUpper()), serverEncode(msg.split(" ")));
}

void CoreUserInputHandler::putPrivmsg(const QString& target,
                                      const QString& message,
                                      std::function<QByteArray(const QString&, const QString&)> encodeFunc,
                                      Cipher* cipher)
{
    Q_UNUSED(cipher);
    QString cmd("PRIVMSG");
    QByteArray targetEnc = serverEncode(target);

    std::function<QList<QByteArray>(QString&)> cmdGenerator = [&](QString& splitMsg) -> QList<QByteArray> {
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

int CoreUserInputHandler::lastParamOverrun(const QString& cmd, const QList<QByteArray>& params)
{
    // the server will pass our message truncated to 512 bytes including CRLF with the following format:
    // ":prefix COMMAND param0 param1 :lastparam"
    // where prefix = "nickname!user@host"
    // that means that the last message can be as long as:
    // 512 - nicklen - userlen - hostlen - commandlen - sum(param[0]..param[n-1])) - 2 (for CRLF) - 4 (":!@" + 1space between prefix and
    // command) - max(paramcount - 1, 0) (space for simple params) - 2 (space and colon for last param)
    IrcUser* me = network()->ircUser(network()->myNick());
    int maxLen = 480 - cmd.toLatin1().size();  // educated guess in case we don't know us (yet?)

    if (me)
        maxLen = 512 - serverEncode(me->nick()).size() - serverEncode(me->user()).size() - serverEncode(me->host()).size()
                 - cmd.toLatin1().size() - 6;

    if (!params.isEmpty()) {
        for (int i = 0; i < params.size() - 1; i++) {
            maxLen -= (params[i].size() + 1);
        }
        maxLen -= 2;  // " :" last param separator;

        if (params.last().size() > maxLen) {
            return params.last().size() - maxLen;
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
QByteArray CoreUserInputHandler::encrypt(const QString& target, const QByteArray& message_, bool* didEncrypt) const
{
    if (didEncrypt)
        *didEncrypt = false;

    if (message_.isEmpty())
        return message_;

    if (!Cipher::neededFeaturesAvailable())
        return message_;

    Cipher* cipher = network()->cipher(target);
    if (!cipher || cipher->key().isEmpty())
        return message_;

    QByteArray message = message_;
    bool result = cipher->encrypt(message);
    if (didEncrypt)
        *didEncrypt = result;

    return message;
}
#endif

void CoreUserInputHandler::timerEvent(QTimerEvent* event)
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
    QStringList commands = rawCommand.split(QRegularExpression("; ?"));
    for (const QString& command : commands) {
        handleUserInput(bufferInfo, command);
    }
}
