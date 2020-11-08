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

#include "ircparser.h"

#include <QDebug>

#include "corenetwork.h"
#include "eventmanager.h"
#include "ircdecoder.h"
#include "ircevent.h"
#include "irctags.h"
#include "messageevent.h"
#include "networkevent.h"

#ifdef HAVE_QCA2
#    include "cipher.h"
#    include "keyevent.h"
#endif

IrcParser::IrcParser(CoreSession* session)
    : QObject(session)
    , _coreSession(session)
{
    // Check if raw IRC logging is enabled
    _debugLogRawIrc = (Quassel::isOptionSet("debug-irc") || Quassel::isOptionSet("debug-irc-id"));
    _debugLogRawNetId = Quassel::optionValue("debug-irc-id").toInt();
    // Check if parsed IRC logging is enabled
    _debugLogParsedIrc = (Quassel::isOptionSet("debug-irc-parsed") || Quassel::isOptionSet("debug-irc-parsed-id"));
    _debugLogParsedNetId = Quassel::optionValue("debug-irc-parsed-id").toInt();

    connect(this, &IrcParser::newEvent, coreSession()->eventManager(), &EventManager::postEvent);
}

bool IrcParser::checkParamCount(const QString& cmd, const QList<QByteArray>& params, int minParams)
{
    if (params.count() < minParams) {
        qWarning() << "Expected" << minParams << "params for IRC command" << cmd << ", got:" << params;
        return false;
    }
    return true;
}

QByteArray IrcParser::decrypt(Network* network, const QString& bufferName, const QByteArray& message, bool isTopic)
{
#ifdef HAVE_QCA2
    if (message.isEmpty())
        return message;

    if (!Cipher::neededFeaturesAvailable())
        return message;

    Cipher* cipher = qobject_cast<CoreNetwork*>(network)->cipher(bufferName);
    if (!cipher || cipher->key().isEmpty())
        return message;

    return isTopic ? cipher->decryptTopic(message) : cipher->decrypt(message);
#else
    Q_UNUSED(network);
    Q_UNUSED(bufferName);
    Q_UNUSED(isTopic);
    return message;
#endif
}

/* parse the raw server string and generate an appropriate event */
/* used to be handleServerMsg()                                  */
void IrcParser::processNetworkIncoming(NetworkDataEvent* e)
{
    auto* net = qobject_cast<CoreNetwork*>(e->network());
    if (!net) {
        qWarning() << "Received network event without valid network pointer!";
        return;
    }

    // note that the IRC server is still alive
    net->resetPingTimeout();

    const QByteArray rawMsg = e->data();
    if (rawMsg.isEmpty()) {
        qWarning() << "Received empty string from server!";
        return;
    }

    // Log the message if enabled and network ID matches or allows all
    if (_debugLogRawIrc && (_debugLogRawNetId == -1 || net->networkId().toInt() == _debugLogRawNetId)) {
        // Include network ID
        qDebug() << "IRC net" << net->networkId() << "<<" << rawMsg;
    }

    QHash<IrcTagKey, QString> tags;
    QString prefix;
    QString cmd;
    QList<QByteArray> params;
    IrcDecoder::parseMessage([&net](const QByteArray& data) {
        return net->serverDecode(data);
    }, rawMsg, tags, prefix, cmd, params);

    // Log the message if enabled and network ID matches or allows all
    if (_debugLogParsedIrc && (_debugLogParsedNetId == -1 || net->networkId().toInt() == _debugLogParsedNetId)) {
        // Include network ID
        qDebug() << "IRC net" << net->networkId() << "<<" << tags << prefix << cmd << params;
    }

    if (net->capEnabled(IrcCap::SERVER_TIME) && tags.contains(IrcTags::SERVER_TIME)) {
        QDateTime serverTime = QDateTime::fromString(tags[IrcTags::SERVER_TIME], "yyyy-MM-ddThh:mm:ss.zzzZ");
        serverTime.setTimeSpec(Qt::UTC);
        if (serverTime.isValid()) {
            e->setTimestamp(serverTime);
        } else {
            qDebug() << "Invalid timestamp from server-time tag:" << tags[IrcTags::SERVER_TIME];
        }
    }

    if (net->capEnabled(IrcCap::ACCOUNT_TAG) && tags.contains(IrcTags::ACCOUNT)) {
        // Whenever account-tag is specified, update the relevant IrcUser if it exists
        // Logged-out status is handled in specific commands (PRIVMSG, NOTICE, etc)
        //
        // Don't use "updateNickFromMask" here to ensure this only updates existing IrcUsers and
        // won't create a new IrcUser.  This guards against an IRC server setting "account" tag in
        // nonsensical places, e.g. for messages that are not user sent.
        IrcUser* ircuser = net->ircUser(prefix);
        if (ircuser) {
            ircuser->setAccount(tags[IrcTags::ACCOUNT]);
        }

        // NOTE: if "account-tag" is enabled and no "account" tag is sent, the given user isn't
        // logged in ONLY IF it is a user-initiated command.  Quassel follows a mixture of what
        // other clients do here - only marking as logged out via PRIVMSG/NOTICE, but marking logged
        // in via any message.
        //
        // See https://ircv3.net/specs/extensions/account-tag-3.2
    }

    QList<Event*> events;
    EventManager::EventType type = EventManager::Invalid;

    QString messageTarget;
    uint num = cmd.toUInt();
    if (num > 0) {
        // numeric reply
        if (params.count() == 0) {
            qWarning() << "Message received from server violates RFC and is ignored!" << rawMsg;
            return;
        }
        // numeric replies have the target as first param (RFC 2812 - 2.4). this is usually our own nick. Remove this!
        messageTarget = net->serverDecode(params.takeFirst());
        type = EventManager::IrcEventNumeric;
    }
    else {
        // any other irc command
        QString typeName = QLatin1String("IrcEvent") + cmd.at(0).toUpper() + cmd.mid(1).toLower();
        type = EventManager::eventTypeByName(typeName);
        if (type == EventManager::Invalid) {
            type = EventManager::eventTypeByName("IrcEventUnknown");
            Q_ASSERT(type != EventManager::Invalid);
        }
    }

    // Almost always, all params are server-encoded. There's a few exceptions, let's catch them here!
    // Possibly not the best option, we might want something more generic? Maybe yet another layer of
    // unencoded events with event handlers for the exceptions...
    // Also, PRIVMSG and NOTICE need some special handling, we put this in here as well, so we get out
    // nice pre-parsed events that the CTCP handler can consume.

    QStringList decParams;
    bool defaultHandling = true;  // whether to automatically copy the remaining params and send the event

    switch (type) {
    case EventManager::IrcEventPrivmsg:
        defaultHandling = false;  // this might create a list of events

        if (checkParamCount(cmd, params, 1)) {
            QString senderNick = nickFromMask(prefix);
            // Fetch/create the relevant IrcUser, and store it for later updates
            IrcUser* ircuser = net->updateNickFromMask(prefix);

            // Handle account-tag
            if (ircuser && net->capEnabled(IrcCap::ACCOUNT_TAG)) {
                if (tags.contains(IrcTags::ACCOUNT)) {
                    // Account tag available, set account.
                    // This duplicates the generic account-tag handling in case a new IrcUser object
                    // was just created.
                    ircuser->setAccount(tags[IrcTags::ACCOUNT]);
                }
                else {
                    // PRIVMSG is user sent; it's safe to assume the user has logged out.
                    // "*" is used to represent logged-out.
                    ircuser->setAccount("*");
                }
            }

            // Check if the sender is our own nick.  If so, treat message as if sent by ourself.
            // See http://ircv3.net/specs/extensions/echo-message-3.2.html
            // Cache the result to avoid multiple redundant comparisons
            bool isSelfMessage = net->isMyNick(senderNick);

            QByteArray msg = params.count() < 2 ? QByteArray() : params.at(1);

            QStringList targets = net->serverDecode(params.at(0)).split(',', QString::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                // For self-messages, keep the target, don't set it to the senderNick
                QString target = net->isChannelName(*targetIter) || net->isStatusMsg(*targetIter) || isSelfMessage ? *targetIter : senderNick;

                // Note: self-messages could be encrypted with a different key.  If issues arise,
                // consider including this within an if (!isSelfMessage) block
                msg = decrypt(net, target, msg);

                IrcEventRawMessage* rawMessage = new IrcEventRawMessage(EventManager::IrcEventRawPrivmsg,
                                                                        net,
                                                                        tags,
                                                                        msg,
                                                                        prefix,
                                                                        target,
                                                                        e->timestamp());
                if (isSelfMessage) {
                    // Self-messages need processed differently, tag as such via flag.
                    rawMessage->setFlag(EventManager::Self);
                }
                events << rawMessage;
            }
        }
        break;

    case EventManager::IrcEventNotice:
        defaultHandling = false;

        if (checkParamCount(cmd, params, 2)) {
            // Check if the sender is our own nick.  If so, treat message as if sent by ourself.
            // See http://ircv3.net/specs/extensions/echo-message-3.2.html
            // Cache the result to avoid multiple redundant comparisons
            bool isSelfMessage = net->isMyNick(nickFromMask(prefix));

            // Only update from the prefix once during the loop
            bool updatedFromPrefix = false;

            QStringList targets = net->serverDecode(params.at(0)).split(',', QString::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = *targetIter;

                // special treatment for welcome messages like:
                // :ChanServ!ChanServ@services. NOTICE egst :[#apache] Welcome, this is #apache. Please read the in-channel topic message.
                // This channel is being logged by IRSeekBot. If you have any question please see http://blog.freenode.net/?p=68
                if (!net->isChannelName(target)) {
                    QString decMsg = net->serverDecode(params.at(1));
                    QRegExp welcomeRegExp(R"(^\[([^\]]+)\] )");
                    if (welcomeRegExp.indexIn(decMsg) != -1) {
                        QString channelname = welcomeRegExp.cap(1);
                        decMsg = decMsg.mid(welcomeRegExp.matchedLength());
                        // we only have CoreIrcChannels in the core, so this cast is safe
                        CoreIrcChannel* chan = static_cast<CoreIrcChannel*>(net->ircChannel(channelname)); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
                        if (chan && !chan->receivedWelcomeMsg()) {
                            chan->setReceivedWelcomeMsg();
                            events << new MessageEvent(Message::Notice, net, decMsg, prefix, channelname, Message::None, e->timestamp());
                            continue;
                        }
                    }
                }

                if (prefix.isEmpty() || target == "AUTH") {
                    target = QString();
                }
                else {
                    if (!target.isEmpty() && net->prefixes().contains(target.at(0)))
                        target = target.mid(1);

                    if (!net->isChannelName(target)) {
                        // For self-messages, keep the target, don't set it to the sender prefix
                        if (!isSelfMessage) {
                            target = nickFromMask(prefix);
                        }

                        if (!updatedFromPrefix) {
                            // Don't repeat this within the loop, the prefix doesn't change
                            updatedFromPrefix = true;

                            // Fetch/create the relevant IrcUser, and store it for later updates
                            IrcUser* ircuser = net->updateNickFromMask(prefix);

                            // Handle account-tag
                            if (ircuser && net->capEnabled(IrcCap::ACCOUNT_TAG)) {
                                if (tags.contains(IrcTags::ACCOUNT)) {
                                    // Account tag available, set account.
                                    // This duplicates the generic account-tag handling in case a
                                    // new IrcUser object was just created.
                                    ircuser->setAccount(tags[IrcTags::ACCOUNT]);
                                }
                                else {
                                    // NOTICE is user sent; it's safe to assume the user has
                                    // logged out.  "*" is used to represent logged-out.
                                    ircuser->setAccount("*");
                                }
                            }
                        }
                    }
                }

#ifdef HAVE_QCA2
                // Handle DH1080 key exchange
                // Don't allow key exchange in channels, and don't allow it for self-messages.
                bool keyExchangeAllowed = (!net->isChannelName(target) && !isSelfMessage);
                if (params[1].startsWith("DH1080_INIT") && keyExchangeAllowed) {
                    events << new KeyEvent(EventManager::KeyEvent, net, tags, prefix, target, KeyEvent::Init, params[1].mid(12));
                }
                else if (params[1].startsWith("DH1080_FINISH") && keyExchangeAllowed) {
                    events << new KeyEvent(EventManager::KeyEvent, net, tags, prefix, target, KeyEvent::Finish, params[1].mid(14));
                }
                else
#endif
                {
                    IrcEventRawMessage* rawMessage = new IrcEventRawMessage(EventManager::IrcEventRawNotice,
                                                                            net,
                                                                            tags,
                                                                            params[1],
                                                                            prefix,
                                                                            target,
                                                                            e->timestamp());
                    if (isSelfMessage) {
                        // Self-messages need processed differently, tag as such via flag.
                        rawMessage->setFlag(EventManager::Self);
                    }
                    events << rawMessage;
                }
            }
        }
        break;

        // the following events need only special casing for param decoding
    case EventManager::IrcEventKick:
        if (params.count() >= 3) {  // we have a reason
            decParams << net->serverDecode(params.at(0)) << net->serverDecode(params.at(1));
            decParams << net->channelDecode(decParams.first(), params.at(2));  // kick reason
        }
        break;

    case EventManager::IrcEventPart:
        if (params.count() >= 2) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel;
            decParams << net->userDecode(nickFromMask(prefix), params.at(1));
            net->updateNickFromMask(prefix);
        }
        break;

    case EventManager::IrcEventQuit:
        if (params.count() >= 1) {
            decParams << net->userDecode(nickFromMask(prefix), params.at(0));
            net->updateNickFromMask(prefix);
        }
        break;

    case EventManager::IrcEventTagmsg:
        defaultHandling = false;  // this might create a list of events

        if (checkParamCount(cmd, params, 1)) {
            QString senderNick = nickFromMask(prefix);
            net->updateNickFromMask(prefix);
            // Check if the sender is our own nick.  If so, treat message as if sent by ourself.
            // See http://ircv3.net/specs/extensions/echo-message-3.2.html
            // Cache the result to avoid multiple redundant comparisons
            bool isSelfMessage = net->isMyNick(senderNick);

            QStringList targets = net->serverDecode(params.at(0)).split(',', QString::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                // For self-messages, keep the target, don't set it to the senderNick
                QString target = net->isChannelName(*targetIter) || net->isStatusMsg(*targetIter) || isSelfMessage ? *targetIter : senderNick;

                IrcEvent* tagMsg = new IrcEvent(EventManager::IrcEventTagmsg, net, tags, prefix, {target});
                if (isSelfMessage) {
                    // Self-messages need processed differently, tag as such via flag.
                    tagMsg->setFlag(EventManager::Self);
                }
                tagMsg->setTimestamp(e->timestamp());
                events << tagMsg;
            }
        }
        break;

    case EventManager::IrcEventTopic:
        if (params.count() >= 1) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel;
            decParams << (params.count() >= 2 ? net->channelDecode(channel, decrypt(net, channel, params.at(1), true)) : QString());
        }
        break;

    case EventManager::IrcEventAway:
        {
            // Update hostmask info first.  This will create the nick if it doesn't exist, e.g.
            // away-notify data being sent before JOIN messages.
            net->updateNickFromMask(prefix);
            // Separate nick in order to separate server and user decoding
            QString nick = nickFromMask(prefix);
            decParams << nick;
            decParams << (params.count() >= 1 ? net->userDecode(nick, params.at(0)) : QString());
        }
        break;

    case EventManager::IrcEventNumeric:
        switch (num) {
        case 301: /* RPL_AWAY */
            if (params.count() >= 2) {
                QString nick = net->serverDecode(params.at(0));
                decParams << nick;
                decParams << net->userDecode(nick, params.at(1));
            }
            break;

        case 332: /* RPL_TOPIC */
            if (params.count() >= 2) {
                QString channel = net->serverDecode(params.at(0));
                decParams << channel;
                decParams << net->channelDecode(channel, decrypt(net, channel, params.at(1), true));
            }
            break;

        case 333: /* Topic set by... */
            if (params.count() >= 3) {
                QString channel = net->serverDecode(params.at(0));
                decParams << channel << net->serverDecode(params.at(1));
                decParams << net->channelDecode(channel, params.at(2));
            }
            break;
        case 451: /* You have not registered... */
            if (messageTarget.compare("CAP", Qt::CaseInsensitive) == 0) {
                // :irc.server.com 451 CAP :You have not registered
                // If server doesn't support capabilities, it will report this message.  Turn it
                // into a nicer message since it's not a real error.
                defaultHandling = false;
                events << new MessageEvent(Message::Server,
                                           e->network(),
                                           tr("Capability negotiation not supported"),
                                           QString(),
                                           QString(),
                                           Message::None,
                                           e->timestamp());
            }
            break;
        }

    default:
        break;
    }

    if (defaultHandling && type != EventManager::Invalid) {
        for (int i = decParams.count(); i < params.count(); i++)
            decParams << net->serverDecode(params.at(i));

        // We want to trim the last param just in case, except for PRIVMSG and NOTICE
        // ... but those happen to be the only ones not using defaultHandling anyway
        if (!decParams.isEmpty() && decParams.last().endsWith(' '))
            decParams.append(decParams.takeLast().trimmed());

        IrcEvent* event;
        if (type == EventManager::IrcEventNumeric)
            event = new IrcEventNumeric(num, net, tags, prefix, messageTarget);
        else
            event = new IrcEvent(type, net, tags, prefix);
        event->setParams(decParams);
        event->setTimestamp(e->timestamp());
        events << event;
    }

    for (Event* event : events) {
        emit newEvent(event);
    }
}
