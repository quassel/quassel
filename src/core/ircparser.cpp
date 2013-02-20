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

#include "ircparser.h"

#include "corenetwork.h"
#include "eventmanager.h"
#include "ircevent.h"
#include "messageevent.h"
#include "networkevent.h"

#ifdef HAVE_QCA2
#  include "cipher.h"
#  include "keyevent.h"
#endif

IrcParser::IrcParser(CoreSession *session) :
    QObject(session),
    _coreSession(session)
{
    connect(this, SIGNAL(newEvent(Event *)), coreSession()->eventManager(), SLOT(postEvent(Event *)));
}


bool IrcParser::checkParamCount(const QString &cmd, const QList<QByteArray> &params, int minParams)
{
    if (params.count() < minParams) {
        qWarning() << "Expected" << minParams << "params for IRC command" << cmd << ", got:" << params;
        return false;
    }
    return true;
}


QByteArray IrcParser::decrypt(Network *network, const QString &bufferName, const QByteArray &message, bool isTopic)
{
#ifdef HAVE_QCA2
    if (message.isEmpty())
        return message;

    if (!Cipher::neededFeaturesAvailable())
        return message;

    Cipher *cipher = qobject_cast<CoreNetwork *>(network)->cipher(bufferName);
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
void IrcParser::processNetworkIncoming(NetworkDataEvent *e)
{
    CoreNetwork *net = qobject_cast<CoreNetwork *>(e->network());
    if (!net) {
        qWarning() << "Received network event without valid network pointer!";
        return;
    }

    // note that the IRC server is still alive
    net->resetPingTimeout();

    QByteArray msg = e->data();
    if (msg.isEmpty()) {
        qWarning() << "Received empty string from server!";
        return;
    }

    // Now we split the raw message into its various parts...
    QString prefix;
    QByteArray trailing;
    QString cmd, target;

    // First, check for a trailing parameter introduced by " :", since this might screw up splitting the msg
    // NOTE: This assumes that this is true in raw encoding, but well, hopefully there are no servers running in japanese on protocol level...
    int idx = msg.indexOf(" :");
    if (idx >= 0) {
        if (msg.length() > idx + 2)
            trailing = msg.mid(idx + 2);
        msg = msg.left(idx);
    }
    // OK, now it is safe to split...
    QList<QByteArray> params = msg.split(' ');

    // This could still contain empty elements due to (faulty?) ircds sending multiple spaces in a row
    // Also, QByteArray is not nearly as convenient to work with as QString for such things :)
    QList<QByteArray>::iterator iter = params.begin();
    while (iter != params.end()) {
        if (iter->isEmpty())
            iter = params.erase(iter);
        else
            ++iter;
    }

    if (!trailing.isEmpty())
        params << trailing;
    if (params.count() < 1) {
        qWarning() << "Received invalid string from server!";
        return;
    }

    QString foo = net->serverDecode(params.takeFirst());

    // a colon as the first chars indicates the existence of a prefix
    if (foo[0] == ':') {
        foo.remove(0, 1);
        prefix = foo;
        if (params.count() < 1) {
            qWarning() << "Received invalid string from server!";
            return;
        }
        foo = net->serverDecode(params.takeFirst());
    }

    // next string without a whitespace is the command
    cmd = foo.trimmed();

    QList<Event *> events;
    EventManager::EventType type = EventManager::Invalid;

    uint num = cmd.toUInt();
    if (num > 0) {
        // numeric reply
        if (params.count() == 0) {
            qWarning() << "Message received from server violates RFC and is ignored!" << msg;
            return;
        }
        // numeric replies have the target as first param (RFC 2812 - 2.4). this is usually our own nick. Remove this!
        target = net->serverDecode(params.takeFirst());
        type = EventManager::IrcEventNumeric;
    }
    else {
        // any other irc command
        QString typeName = QLatin1String("IrcEvent") + cmd.at(0).toUpper() + cmd.mid(1).toLower();
        type = eventManager()->eventTypeByName(typeName);
        if (type == EventManager::Invalid) {
            type = eventManager()->eventTypeByName("IrcEventUnknown");
            Q_ASSERT(type != EventManager::Invalid);
        }
        target = QString();
    }

    // Almost always, all params are server-encoded. There's a few exceptions, let's catch them here!
    // Possibly not the best option, we might want something more generic? Maybe yet another layer of
    // unencoded events with event handlers for the exceptions...
    // Also, PRIVMSG and NOTICE need some special handling, we put this in here as well, so we get out
    // nice pre-parsed events that the CTCP handler can consume.

    QStringList decParams;
    bool defaultHandling = true; // whether to automatically copy the remaining params and send the event

    switch (type) {
    case EventManager::IrcEventPrivmsg:
        defaultHandling = false; // this might create a list of events

        if (checkParamCount(cmd, params, 1)) {
            QString senderNick = nickFromMask(prefix);
            QByteArray msg = params.count() < 2 ? QByteArray() : params.at(1);

            QStringList targets = net->serverDecode(params.at(0)).split(',', QString::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = net->isChannelName(*targetIter) ? *targetIter : senderNick;

                msg = decrypt(net, target, msg);

                events << new IrcEventRawMessage(EventManager::IrcEventRawPrivmsg, net, msg, prefix, target, e->timestamp());
            }
        }
        break;

    case EventManager::IrcEventNotice:
        defaultHandling = false;

        if (checkParamCount(cmd, params, 2)) {
            QStringList targets = net->serverDecode(params.at(0)).split(',', QString::SkipEmptyParts);
            QStringList::const_iterator targetIter;
            for (targetIter = targets.constBegin(); targetIter != targets.constEnd(); ++targetIter) {
                QString target = *targetIter;

                // special treatment for welcome messages like:
                // :ChanServ!ChanServ@services. NOTICE egst :[#apache] Welcome, this is #apache. Please read the in-channel topic message. This channel is being logged by IRSeekBot. If you have any question please see http://blog.freenode.net/?p=68
                if (!net->isChannelName(target)) {
                    QString decMsg = net->serverDecode(params.at(1));
                    QRegExp welcomeRegExp("^\\[([^\\]]+)\\] ");
                    if (welcomeRegExp.indexIn(decMsg) != -1) {
                        QString channelname = welcomeRegExp.cap(1);
                        decMsg = decMsg.mid(welcomeRegExp.matchedLength());
                        CoreIrcChannel *chan = static_cast<CoreIrcChannel *>(net->ircChannel(channelname)); // we only have CoreIrcChannels in the core, so this cast is safe
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
                    if (!net->isChannelName(target))
                        target = nickFromMask(prefix);
                }

#ifdef HAVE_QCA2
                // Handle DH1080 key exchange
                if (params[1].startsWith("DH1080_INIT")) {
                    events << new KeyEvent(EventManager::KeyEvent, net, prefix, target, KeyEvent::Init, params[1].mid(12));
                } else if (params[1].startsWith("DH1080_FINISH")) {
                    events << new KeyEvent(EventManager::KeyEvent, net, prefix, target, KeyEvent::Finish, params[1].mid(14));
                } else
#endif
                    events << new IrcEventRawMessage(EventManager::IrcEventRawNotice, net, params[1], prefix, target, e->timestamp());
            }
        }
        break;

    // the following events need only special casing for param decoding
    case EventManager::IrcEventKick:
        if (params.count() >= 3) { // we have a reason
            decParams << net->serverDecode(params.at(0)) << net->serverDecode(params.at(1));
            decParams << net->channelDecode(decParams.first(), params.at(2)); // kick reason
        }
        break;

    case EventManager::IrcEventPart:
        if (params.count() >= 2) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel;
            decParams << net->userDecode(nickFromMask(prefix), params.at(1));
        }
        break;

    case EventManager::IrcEventQuit:
        if (params.count() >= 1) {
            decParams << net->userDecode(nickFromMask(prefix), params.at(0));
        }
        break;

    case EventManager::IrcEventTopic:
        if (params.count() >= 1) {
            QString channel = net->serverDecode(params.at(0));
            decParams << channel;
            decParams << (params.count() >= 2 ? net->channelDecode(channel, decrypt(net, channel, params.at(1), true)) : QString());
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

        IrcEvent *event;
        if (type == EventManager::IrcEventNumeric)
            event = new IrcEventNumeric(num, net, prefix, target);
        else
            event = new IrcEvent(type, net, prefix);
        event->setParams(decParams);
        event->setTimestamp(e->timestamp());
        events << event;
    }

    foreach(Event *event, events) {
        emit newEvent(event);
    }
}
