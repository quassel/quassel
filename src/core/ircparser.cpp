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

#include "ircparser.h"

#include <QDebug>
#include <QRegularExpression>
#include <QTimeZone>

#include "corenetwork.h"
#include "eventmanager.h"
#include "ircdecoder.h"
#include "ircevent.h"
#include "irctags.h"
#include "messageevent.h"
#include "util.h"

#ifdef HAVE_QCA2
#    include "cipher.h"
#    include "keyevent.h"
#endif

IrcParser::IrcParser(CoreSession* session)
    : QObject(session)
    , _coreSession(session)
{
    _debugLogRawIrc = (Quassel::isOptionSet("debug-irc") || Quassel::isOptionSet("debug-irc-id"));
    _debugLogRawNetId = Quassel::optionValue("debug-irc-id").toInt();
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

void IrcParser::processNetworkIncoming(NetworkDataEvent* e)
{
    if (!e) {
        qWarning() << Q_FUNC_INFO << "Received null NetworkDataEvent!";
        return;
    }
    qDebug() << Q_FUNC_INFO << "Processing NetworkDataEvent at address:" << e;
    Network* network = e->network();
    if (!network) {
        qWarning() << Q_FUNC_INFO << "Null network pointer in NetworkDataEvent at address:" << e;
        return;
    }
    qDebug() << Q_FUNC_INFO << "Network pointer:" << network;
    auto* net = qobject_cast<CoreNetwork*>(network);
    if (!net) {
        qWarning() << Q_FUNC_INFO << "Network is not a CoreNetwork: " << network;
        return;
    }
    qDebug() << Q_FUNC_INFO << "Processing NetworkDataEvent for network:" << net->networkId() << "at address:" << net;

    net->resetPingTimeout();

    const QByteArray rawMsg = e->data();
    if (rawMsg.isEmpty()) {
        qWarning() << "Received empty string from server!";
        return;
    }

    if (_debugLogRawIrc && (_debugLogRawNetId == -1 || net->networkId().toInt() == _debugLogRawNetId)) {
        qDebug() << "IRC net" << net->networkId() << "<<" << rawMsg;
    }

    QHash<IrcTagKey, QString> tags;
    QString prefix;
    QString cmd;
    QList<QByteArray> params;
    IrcDecoder::parseMessage([&net](const QByteArray& data) { return net->serverDecode(data); }, rawMsg, tags, prefix, cmd, params);

    if (_debugLogParsedIrc && (_debugLogParsedNetId == -1 || net->networkId().toInt() == _debugLogParsedNetId)) {
        qDebug() << "IRC net" << net->networkId() << "<<" << tags << prefix << cmd << params;
    }

    if (net->enabledCaps().contains(IrcCap::SERVER_TIME) && tags.contains(IrcTags::SERVER_TIME)) {
        QDateTime serverTime = QDateTime::fromString(tags[IrcTags::SERVER_TIME], "yyyy-MM-ddThh:mm:ss.zzzZ");
        serverTime.setTimeZone(QTimeZone("UTC"));
        if (serverTime.isValid()) {
            e->setTimestamp(serverTime);
        }
        else {
            qDebug() << "Invalid timestamp from server-time tag:" << tags[IrcTags::SERVER_TIME];
        }
    }

    if (net->enabledCaps().contains(IrcCap::ACCOUNT_TAG) && tags.contains(IrcTags::ACCOUNT)) {
        IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
        if (ircuser) {
            ircuser->setAccount(tags[IrcTags::ACCOUNT]);
        }
    }

    QList<Event*> events;
    EventManager::EventType type = EventManager::Invalid;

    QString messageTarget;
    uint num = cmd.toUInt();
    if (num > 0) {
        if (params.count() == 0) {
            qWarning() << "Message received from server violates RFC and is ignored!" << rawMsg;
            return;
        }
        messageTarget = net->serverDecode(params.takeFirst());
        type = EventManager::IrcEventNumeric;
    }
    else {
        QString typeName = QLatin1String("IrcEvent") + cmd.at(0).toUpper() + cmd.mid(1).toLower();
        type = EventManager::eventTypeByName(typeName);
        if (type == EventManager::Invalid) {
            type = EventManager::eventTypeByName("IrcEventUnknown");
            Q_ASSERT(type != EventManager::Invalid);
        }
    }

    QStringList decParams;
    bool defaultHandling = true;

    switch (type) {
    case EventManager::IrcEventPrivmsg:
        defaultHandling = false;

        if (checkParamCount(cmd, params, 1)) {
            QString senderNick = nickFromMask(prefix);
            IrcUser* ircuser = net->ircUser(senderNick);
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }

            if (ircuser && net->enabledCaps().contains(IrcCap::ACCOUNT_TAG)) {
                if (tags.contains(IrcTags::ACCOUNT)) {
                    ircuser->setAccount(tags[IrcTags::ACCOUNT]);
                }
                else {
                    ircuser->setAccount("*");
                }
            }

            bool isSelfMessage = net->isMyNick(senderNick);

            QByteArray msg = params.count() < 2 ? QByteArray() : params.at(1);

            QStringList targets = net->serverDecode(params.at(0)).split(',', Qt::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = net->isChannelName(*targetIter) || net->isStatusMsg(*targetIter) || isSelfMessage ? *targetIter : senderNick;

                msg = decrypt(net, target, msg);

                IrcEventRawMessage* rawMessage
                    = new IrcEventRawMessage(EventManager::IrcEventRawPrivmsg, net, tags, msg, prefix, target, e->timestamp());
                if (isSelfMessage) {
                    rawMessage->setFlag(EventManager::Self);
                }
                events << rawMessage;
            }
        }
        break;

    case EventManager::IrcEventNotice:
        defaultHandling = false;

        if (checkParamCount(cmd, params, 2)) {
            bool isSelfMessage = net->isMyNick(nickFromMask(prefix));

            bool updatedFromPrefix = false;

            QStringList targets = net->serverDecode(params.at(0)).split(',', Qt::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = *targetIter;

                if (!net->isChannelName(target)) {
                    QString decMsg = net->serverDecode(params.at(1));
                    QRegularExpression welcomeRegExp(R"(^\[([^\]]+)\] )");
                    QRegularExpressionMatch match = welcomeRegExp.match(decMsg);
                    if (match.hasMatch()) {
                        QString channelname = match.captured(1);
                        decMsg = decMsg.mid(match.capturedLength(0));
                        CoreIrcChannel* chan = static_cast<CoreIrcChannel*>(net->ircChannel(channelname));
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
                        if (!isSelfMessage) {
                            target = nickFromMask(prefix);
                        }

                        if (!updatedFromPrefix) {
                            updatedFromPrefix = true;

                            IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
                            if (!ircuser) {
                                ircuser = net->newIrcUser(prefix);
                            }

                            if (ircuser && net->enabledCaps().contains(IrcCap::ACCOUNT_TAG)) {
                                if (tags.contains(IrcTags::ACCOUNT)) {
                                    ircuser->setAccount(tags[IrcTags::ACCOUNT]);
                                }
                                else {
                                    ircuser->setAccount("*");
                                }
                            }
                        }
                    }
                }

#ifdef HAVE_QCA2
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
                    IrcEventRawMessage* rawMessage
                        = new IrcEventRawMessage(EventManager::IrcEventRawNotice, net, tags, params[1], prefix, target, e->timestamp());
                    if (isSelfMessage) {
                        rawMessage->setFlag(EventManager::Self);
                    }
                    events << rawMessage;
                }
            }
        }
        break;

    case EventManager::IrcEventKick:
        if (params.count() >= 3) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel << net->serverDecode(params.at(1));
            decParams << net->channelDecode(channel, params.at(2));
            IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }
        }
        break;

    case EventManager::IrcEventPart:
        if (params.count() >= 2) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel;
            decParams << net->userDecode(nickFromMask(prefix), params.at(1));
            IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }
        }
        break;

    case EventManager::IrcEventQuit:
        if (params.count() >= 1) {
            decParams << net->userDecode(nickFromMask(prefix), params.at(0));
            IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }
        }
        break;

    case EventManager::IrcEventTagmsg:
        defaultHandling = false;

        if (checkParamCount(cmd, params, 1)) {
            QString senderNick = nickFromMask(prefix);
            IrcUser* ircuser = net->ircUser(senderNick);
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }

            bool isSelfMessage = net->isMyNick(senderNick);

            QStringList targets = net->serverDecode(params.at(0)).split(',', Qt::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = net->isChannelName(*targetIter) || net->isStatusMsg(*targetIter) || isSelfMessage ? *targetIter : senderNick;

                IrcEvent* tagMsg = new IrcEvent(EventManager::IrcEventTagmsg, net, tags, prefix, {target});
                if (isSelfMessage) {
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
            IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
            if (!ircuser) {
                ircuser = net->newIrcUser(prefix);
            }
        }
        break;

    case EventManager::IrcEventAway: {
        IrcUser* ircuser = net->ircUser(nickFromMask(prefix));
        if (!ircuser) {
            ircuser = net->newIrcUser(prefix);
        }
        QString nick = nickFromMask(prefix);
        decParams << nick;
        decParams << (params.count() >= 1 ? net->userDecode(nick, params.at(0)) : QString());
    } break;

    case EventManager::IrcEventNumeric:
        switch (num) {
        case 301:
            if (params.count() >= 2) {
                QString nick = net->serverDecode(params.at(0));
                decParams << nick;
                decParams << net->userDecode(nick, params.at(1));
            }
            break;

        case 332:
            if (params.count() >= 2) {
                QString channel = net->serverDecode(params.at(0));
                decParams << channel;
                decParams << net->channelDecode(channel, decrypt(net, channel, params.at(1), true));
            }
            break;

        case 333:
            if (params.count() >= 3) {
                QString channel = net->serverDecode(params.at(0));
                decParams << channel << net->serverDecode(params.at(1));
                decParams << net->channelDecode(channel, params.at(2));
            }
            break;
        case 451:
            if (messageTarget.compare("CAP", Qt::CaseInsensitive) == 0) {
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
