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

#include "eventstringifier.h"

#include <QTimeZone>

#include "coresession.h"
#include "ctcpevent.h"
#include "irctags.h"
#include "messageevent.h"
#include "util.h"  // Added for nickFromMask

EventStringifier::EventStringifier(CoreSession* parent)
    : BasicHandler("handleCtcp", parent)
    , _coreSession(parent)
    , _whois(false)
{
    connect(this, &EventStringifier::newMessageEvent, coreSession()->eventManager(), &EventManager::postEvent);
}

void EventStringifier::displayMsg(NetworkEvent* event, Message::Type msgType, QString msg, QString sender, QString target, Message::Flags msgFlags)
{
    if (event->flags().testFlag(EventManager::Silent))
        return;

    MessageEvent* msgEvent = createMessageEvent(event, msgType, std::move(msg), std::move(sender), std::move(target), msgFlags);
    emit newMessageEvent(msgEvent);
}

MessageEvent* EventStringifier::createMessageEvent(
    NetworkEvent* event, Message::Type msgType, QString msg, QString sender, QString target, Message::Flags msgFlags)
{
    MessageEvent* msgEvent
        = new MessageEvent(msgType, event->network(), std::move(msg), std::move(sender), std::move(target), msgFlags, event->timestamp());
    return msgEvent;
}

bool EventStringifier::checkParamCount(IrcEvent* e, int minParams)
{
    if (e->params().count() < minParams) {
        if (e->type() == EventManager::IrcEventNumeric) {
            qWarning() << "Command " << static_cast<IrcEventNumeric*>(e)->number() << " requires " << minParams
                       << "params, got: " << e->params();
        }
        else {
            QString name = EventManager::enumName(e->type());
            qWarning() << qPrintable(name) << "requires" << minParams << "params, got:" << e->params();
        }
        e->stop();
        return false;
    }
    return true;
}

/* These are only for legacy reasons; remove as soon as we handle NetworkSplitEvents properly */
void EventStringifier::processNetworkSplitJoin(NetworkSplitEvent* e)
{
    QString msg = e->users().join("#:#") + "#:#" + e->quitMessage();
    displayMsg(e, Message::NetsplitJoin, msg, QString(), e->channel());
}

void EventStringifier::processNetworkSplitQuit(NetworkSplitEvent* e)
{
    QString msg = e->users().join("#:#") + "#:#" + e->quitMessage();
    displayMsg(e, Message::NetsplitQuit, msg, QString(), e->channel());
}

/* End legacy */

void EventStringifier::processIrcEventNumeric(IrcEventNumeric* e)
{
    switch (e->number()) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 221:
    case 250:
    case 251:
    case 252:
    case 253:
    case 254:
    case 255:
    case 256:
    case 257:
    case 258:
    case 259:
    case 265:
    case 266:
    case 372:
    case 375:
        displayMsg(e, Message::Server, e->params().join(" "), e->prefix());
        break;

    case 263:
    case 409:
    case 411:
    case 412:
    case 422:
    case 424:
    case 445:
    case 446:
    case 451:
    case 462:
    case 463:
    case 464:
    case 465:
    case 466:
    case 472:
    case 481:
    case 483:
    case 485:
    case 491:
    case 501:
    case 502:
    case 431:
        displayMsg(e, Message::Error, e->params().join(" "), e->prefix());
        break;

    case 401: {
        if (!checkParamCount(e, 1))
            return;

        QStringList params = e->params();
        QString target = params.takeFirst();
        displayMsg(e, Message::Error, target + ": " + params.join(" "), e->prefix(), target, Message::Redirected);
        break;
    }

    case 402:
    case 403:
    case 404:
    case 406:
    case 408:
    case 415:
    case 421:
    case 442: {
        if (!checkParamCount(e, 1))
            return;

        QStringList params = e->params();
        QString channelName = params.takeFirst();
        displayMsg(e, Message::Error, channelName + ": " + params.join(" "), e->prefix());
        break;
    }

    case 413:
    case 414:
    case 423:
    case 441:
    case 444:
    case 461:
    case 467:
    case 471:
    case 473:
    case 474:
    case 475:
    case 476:
    case 477:
    case 478:
    case 482:
    case 436: {
        if (!checkParamCount(e, 1))
            return;

        QStringList params = e->params();
        QString p = params.takeFirst();
        displayMsg(e, Message::Error, p + ": " + params.join(" "));
        break;
    }

    case 321:
    case 353:
    case 366:
    case 376:
        break;

    case 900:
    case 901: {
        if (!checkParamCount(e, 3))
            return;
        displayMsg(e, Message::Server, "SASL: " + e->params().at(2));
        break;
    }
    case 903:
        break;
    case 902:
    case 904:
    case 905:
    case 906:
    case 907:
    case 908: {
        displayMsg(e, Message::Server, "SASL: " + e->params().join(""));
        break;
    }

    default:
        if (_whois) {
            displayMsg(e, Message::Server, tr("[Whois] ") + e->params().join(" "), e->prefix());
        }
        else {
            displayMsg(e, Message::Error, QString("%1 %2").arg(e->number(), 3, 10, QLatin1Char('0')).arg(e->params().join(" ")), e->prefix());
        }
    }
}

void EventStringifier::processIrcEventInvite(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString target = e->params().at(0);
    QString channel = e->params().at(1);
    if (e->network()->isMyNick(target)) {
        displayMsg(e, Message::Invite, tr("%1 invited you to channel %2").arg(e->nick(), channel));
    }
    else {
        displayMsg(e, Message::Invite, tr("%1 invited %2 to channel %3").arg(e->nick(), target, channel));
    }
}

void EventStringifier::processIrcEventJoin(IrcEvent* e)
{
    if (e->testFlag(EventManager::Netsplit))
        return;

    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        msgFlags = Message::Self;
    }

    displayMsg(e, Message::Join, e->params()[0], e->prefix(), e->params()[0], msgFlags);
}

void EventStringifier::processIrcEventKick(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    IrcUser* victim = e->network()->ircUser(e->params().at(1));
    if (victim) {
        QString channel = e->params().at(0);
        QString msg = victim->nick();
        if (e->params().count() > 2)
            msg += " " + e->params().at(2);

        Message::Flag msgFlags = Message::Flag::None;
        if (e->testFlag(EventManager::Self)) {
            msgFlags = Message::Self;
        }

        displayMsg(e, Message::Kick, msg, e->prefix(), channel, msgFlags);
    }
}

void EventStringifier::processIrcEventMode(IrcEvent* e)
{
    if (e->network()->isChannelName(e->params().first())) {
        displayMsg(e, Message::Mode, e->params().join(" "), e->prefix(), e->params().first());
    }
    else {
        Message::Flag msgFlags = Message::Flag::None;
        if (e->testFlag(EventManager::Self)) {
            msgFlags = Message::Self;
        }
        displayMsg(e, Message::Mode, e->params().join(" "), e->prefix(), QString(), msgFlags);
    }
}

void EventStringifier::processIrcEventNick(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    IrcUser* ircuser = e->network()->ircUser(nickFromMask(e->prefix()));
    if (!ircuser) {
        ircuser = e->network()->newIrcUser(e->prefix());
    }
    if (!ircuser) {
        qWarning() << Q_FUNC_INFO << "Unknown IrcUser!";
        return;
    }

    QString newnick = e->params().at(0);

    QString sender;
    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        sender = newnick;
        msgFlags = Message::Self;
    }
    else {
        sender = e->prefix();
    }

    for (const QString& channel : ircuser->channels()) {
        displayMsg(e, Message::Nick, newnick, sender, channel, msgFlags);
    }
}

void EventStringifier::processIrcEventPart(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    QString channel = e->params().at(0);
    QString msg = e->params().count() > 1 ? e->params().at(1) : QString();

    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        msgFlags = Message::Self;
    }

    displayMsg(e, Message::Part, msg, e->prefix(), channel, msgFlags);
}

void EventStringifier::processIrcEventPong(IrcEvent* e)
{
    displayMsg(e, Message::Server, "PONG " + e->params().join(" "), e->prefix());
}

void EventStringifier::processIrcEventQuit(IrcEvent* e)
{
    if (e->testFlag(EventManager::Netsplit))
        return;

    IrcUser* ircuser = e->network()->ircUser(nickFromMask(e->prefix()));
    if (!ircuser)
        return;

    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        msgFlags = Message::Self;
    }

    for (const QString& channel : ircuser->channels()) {
        displayMsg(e, Message::Quit, e->params().count() ? e->params().first() : QString(), e->prefix(), channel, msgFlags);
    }
}

void EventStringifier::processIrcEventTopic(IrcEvent* e)
{
    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        msgFlags = Message::Self;
    }

    displayMsg(e,
               Message::Topic,
               tr("%1 has changed topic for %2 to: \"%3\"").arg(e->nick(), e->params().at(0), e->params().at(1)),
               QString(),
               e->params().at(0),
               msgFlags);
}

void EventStringifier::processIrcEventError(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    displayMsg(e, Message::Server, tr("Error from server: ") + e->params().join(""));
}

void EventStringifier::processIrcEventWallops(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("[Operwall] %1: %2").arg(e->nick(), e->params().join(" ")));
}

void EventStringifier::processIrcEvent005(IrcEvent* e)
{
    if (!e->params().last().contains(QRegularExpression("are supported (by|on) this server")))
        displayMsg(e, Message::Error, tr("Received non-RFC-compliant RPL_ISUPPORT: this can lead to unexpected behavior!"), e->prefix());
    displayMsg(e, Message::Server, e->params().join(" "), e->prefix());
}

void EventStringifier::processIrcEvent301(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString nick = e->params().at(0);
    QString awayMsg = e->params().at(1);
    QString msg, target;

    if (_whois) {
        msg = tr("[Whois] ");
    }
    else {
        target = nick;
    }

    IrcUser* ircuser = e->network()->ircUser(nick);
    if (ircuser) {
        QDateTime now = QDateTime::currentDateTime();
        now.setTimeZone(QTimeZone("UTC"));
        ircuser->setLastAwayMessageTime(now);
    }

    displayMsg(e, Message::Server, msg + tr("%1 is away: \"%2\"").arg(nick, awayMsg), QString(), target);
}

void EventStringifier::processIrcEvent305(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("You are no longer marked as being away"));
}

void EventStringifier::processIrcEvent306(IrcEvent* e)
{
    if (!e->network()->autoAwayActive())
        displayMsg(e, Message::Server, tr("You have been marked as being away"));
}

void EventStringifier::processIrcEvent311(IrcEvent* e)
{
    _whois = true;

    const QString whoisUserString = tr("[Whois] %1 is %2 (%3)");

    IrcUser* ircuser = e->network()->ircUser(e->params().at(0));
    if (ircuser)
        displayMsg(e, Message::Server, whoisUserString.arg(ircuser->nick(), ircuser->hostmask(), ircuser->realName()));
    else {
        QString host = QString("%1!%2@%3").arg(e->params().at(0), e->params().at(1), e->params().at(2));
        displayMsg(e, Message::Server, whoisUserString.arg(e->params().at(0), host, e->params().last()));
    }
}

void EventStringifier::processIrcEvent312(IrcEvent* e)
{
    if (_whois)
        displayMsg(e, Message::Server, tr("[Whois] %1 is online via %2 (%3)").arg(e->params().at(0), e->params().at(1), e->params().last()));
    else
        displayMsg(e, Message::Server, tr("[Whowas] %1 was online via %2 (%3)").arg(e->params().at(0), e->params().at(1), e->params().last()));
}

void EventStringifier::processIrcEvent314(IrcEvent* e)
{
    if (!checkParamCount(e, 3))
        return;

    displayMsg(e, Message::Server, tr("[Whowas] %1 was %2@%3 (%4)").arg(e->params()[0], e->params()[1], e->params()[2], e->params().last()));
}

void EventStringifier::processIrcEvent315(IrcEvent* e)
{
    QStringList p = e->params();
    p.takeLast();
    displayMsg(e, Message::Server, tr("[Who] End of /WHO list for %1").arg(p.join(" ")));
}

void EventStringifier::processIrcEvent317(IrcEvent* e)
{
    int idleSecs = e->params()[1].toInt();

    if (e->params().count() > 3) {
        QDateTime loginTime = QDateTime::fromSecsSinceEpoch(e->params()[2].toLongLong()).toUTC();
        displayMsg(e,
                   Message::Server,
                   tr("[Whois] %1 is logged in since %2").arg(e->params()[0], loginTime.toString("yyyy-MM-dd hh:mm:ss UTC")));
    }
    QDateTime idlingSince = e->timestamp().toLocalTime().addSecs(-idleSecs).toUTC();
    displayMsg(e,
               Message::Server,
               tr("[Whois] %1 is idling for %2 (since %3)")
                   .arg(e->params()[0], secondsToString(idleSecs), idlingSince.toString("yyyy-MM-dd hh:mm:ss UTC")));
}

void EventStringifier::processIrcEvent318(IrcEvent* e)
{
    _whois = false;
    displayMsg(e, Message::Server, tr("[Whois] End of /WHOIS list"));
}

void EventStringifier::processIrcEvent319(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString nick = e->params().first();
    QStringList op;
    QStringList voice;
    QStringList user;
    for (QString channel : e->params().last().split(" ")) {
        if (channel.startsWith("@"))
            op.append(channel.remove(0, 1));
        else if (channel.startsWith("+"))
            voice.append(channel.remove(0, 1));
        else
            user.append(channel);
    }
    if (!user.isEmpty())
        displayMsg(e, Message::Server, tr("[Whois] %1 is a user on channels: %2").arg(nick, user.join(" ")));
    if (!voice.isEmpty())
        displayMsg(e, Message::Server, tr("[Whois] %1 has voice on channels: %2").arg(nick, voice.join(" ")));
    if (!op.isEmpty())
        displayMsg(e, Message::Server, tr("[Whois] %1 is an operator on channels: %2").arg(nick, op.join(" ")));
}

void EventStringifier::processIrcEvent322(IrcEvent* e)
{
    QString channelName;
    quint32 userCount = 0;
    QString topic;

    switch (e->params().count()) {
    case 3:
        topic = e->params()[2];
    case 2:
        userCount = e->params()[1].toUInt();
    case 1:
        channelName = e->params()[0];
    default:
        break;
    }
    displayMsg(e, Message::Server, tr("Channel %1 has %2 users. Topic is: \"%3\"").arg(channelName).arg(userCount).arg(topic));
}

void EventStringifier::processIrcEvent323(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("End of channel list"));
}

void EventStringifier::processIrcEvent324(IrcEvent* e)
{
    processIrcEventMode(e);
}

void EventStringifier::processIrcEvent328(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString channel = e->params()[0];
    displayMsg(e, Message::Topic, tr("Homepage for %1 is %2").arg(channel, e->params()[1]), QString(), channel);
}

void EventStringifier::processIrcEvent329(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString channel = e->params()[0];
    qint64 unixtime = e->params()[1].toLongLong();
    if (!unixtime) {
        qWarning() << Q_FUNC_INFO << "received invalid timestamp:" << e->params()[1];
        return;
    }
    QDateTime time = QDateTime::fromSecsSinceEpoch(unixtime).toUTC();
    displayMsg(e, Message::Topic, tr("Channel %1 created on %2").arg(channel, time.toString("yyyy-MM-dd hh:mm:ss UTC")), QString(), channel);
}

void EventStringifier::processIrcEvent330(IrcEvent* e)
{
    if (e->params().count() < 3)
        return;

    if (_whois) {
        displayMsg(e, Message::Server, tr("[Whois] %1 is authed as %2").arg(e->params()[0], e->params()[1]));
    }
    else {
        displayMsg(e, Message::Server, tr("[Whowas] %1 was authed as %2").arg(e->params()[0], e->params()[1]));
    }
}

void EventStringifier::processIrcEvent331(IrcEvent* e)
{
    QString channel = e->params().first();
    displayMsg(e, Message::Topic, tr("No topic is set for %1.").arg(channel), QString(), channel);
}

void EventStringifier::processIrcEvent332(IrcEvent* e)
{
    QString channel = e->params().first();
    displayMsg(e, Message::Topic, tr("Topic for %1 is \"%2\"").arg(channel, e->params()[1]), QString(), channel);
}

void EventStringifier::processIrcEvent333(IrcEvent* e)
{
    if (!checkParamCount(e, 3))
        return;

    QString channel = e->params().first();
    QDateTime topicSetTime = QDateTime::fromSecsSinceEpoch(e->params()[2].toLongLong()).toUTC();
    displayMsg(e,
               Message::Topic,
               tr("Topic set by %1 on %2").arg(e->params()[1], topicSetTime.toString("yyyy-MM-dd hh:mm:ss UTC")),
               QString(),
               channel);
}

void EventStringifier::processIrcEvent341(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    QString channel = e->params()[1];
    displayMsg(e, Message::Server, tr("%1 has been invited to %2").arg(e->params().first(), channel), QString(), channel);
}

void EventStringifier::processIrcEvent352(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("[Who] %1").arg(e->params().join(" ")));
}

void EventStringifier::processIrcEvent354(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("[WhoX] %1").arg(e->params().join(" ")));
}

void EventStringifier::processIrcEvent369(IrcEvent* e)
{
    displayMsg(e, Message::Server, tr("End of /WHOWAS"));
}

void EventStringifier::processIrcEvent432(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    displayMsg(e, Message::Error, tr("Nick %1 contains illegal characters").arg(e->params()[0]));
}

void EventStringifier::processIrcEvent433(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    displayMsg(e, Message::Error, tr("Nick already in use: %1").arg(e->params()[0]));
}

void EventStringifier::processIrcEvent437(IrcEvent* e)
{
    if (!checkParamCount(e, 1))
        return;

    displayMsg(e, Message::Error, tr("Nick/channel is temporarily unavailable: %1").arg(e->params()[0]));
}

void EventStringifier::processIrcEvent524(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    displayMsg(e, Message::Error, tr("[Help] %1").arg(e->params()[1]));
}

void EventStringifier::processIrcEvent704(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    displayMsg(e, Message::Server, tr("[Help] %1").arg(e->params()[1]));
}

void EventStringifier::processIrcEvent705(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    displayMsg(e, Message::Server, tr("[Help] %1").arg(e->params()[1]));
}

void EventStringifier::processIrcEvent706(IrcEvent* e)
{
    if (!checkParamCount(e, 2))
        return;

    displayMsg(e, Message::Server, tr("[Help] %1").arg(e->params()[1]));
}

void EventStringifier::processCtcpEvent(CtcpEvent* e)
{
    if (e->type() != EventManager::CtcpEvent)
        return;

    if (e->testFlag(EventManager::Self) && e->ctcpType() == CtcpEvent::CtcpType::Query && e->ctcpCmd() != "ACTION") {
        displayMsg(e,
                   Message::Action,
                   tr("sending CTCP-%1 request to %2").arg(e->ctcpCmd(), e->target()),
                   e->network()->myNick(),
                   QString(),
                   Message::Flag::Self);
        return;
    }

    QString ctcpCmd = e->ctcpCmd().toUpper();
    if (ctcpCmd == "ACTION") {
        handleCtcpAction(e);
    }
    else if (ctcpCmd == "PING") {
        handleCtcpPing(e);
    }
    else {
        defaultHandler(ctcpCmd, e);
    }
}

void EventStringifier::defaultHandler(const QString& ctcpCmd, CtcpEvent* e)
{
    Q_UNUSED(ctcpCmd);
    if (e->ctcpType() == CtcpEvent::Query) {
        QString unknown;
        if (e->reply().isNull())
            unknown = tr("unknown") + ' ';
        displayMsg(e, Message::Server, tr("Received %1CTCP-%2 request by %3").arg(unknown, e->ctcpCmd(), e->prefix()));
    }
    else {
        if (!e->testFlag(EventManager::Self)) {
            displayMsg(e, Message::Server, tr("Received CTCP-%1 answer from %2: %3").arg(e->ctcpCmd(), nickFromMask(e->prefix()), e->param()));
        }
    }
}

void EventStringifier::handleCtcpAction(CtcpEvent* e)
{
    Message::Flag msgFlags = Message::Flag::None;
    if (e->testFlag(EventManager::Self)) {
        msgFlags = Message::Self;
    }

    displayMsg(e, Message::Action, e->param(), e->prefix(), e->target(), msgFlags);
}

void EventStringifier::handleCtcpPing(CtcpEvent* e)
{
    if (e->ctcpType() == CtcpEvent::Query)
        defaultHandler(e->ctcpCmd(), e);
    else {
        displayMsg(e,
                   Message::Server,
                   tr("Received CTCP-PING answer from %1 with %2 milliseconds round trip time")
                       .arg(nickFromMask(e->prefix()))
                       .arg(QDateTime::fromMSecsSinceEpoch(e->param().toULongLong()).msecsTo(e->timestamp())));
    }
}
