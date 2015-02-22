/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include "ctcpparser.h"

#include "corenetworkconfig.h"
#include "coresession.h"
#include "ctcpevent.h"
#include "messageevent.h"
#include "coreuserinputhandler.h"

const QByteArray XDELIM = "\001";

CtcpParser::CtcpParser(CoreSession *coreSession, QObject *parent)
    : QObject(parent),
    _coreSession(coreSession)
{
    QByteArray MQUOTE = QByteArray("\020");
    _ctcpMDequoteHash[MQUOTE + '0'] = QByteArray(1, '\000');
    _ctcpMDequoteHash[MQUOTE + 'n'] = QByteArray(1, '\n');
    _ctcpMDequoteHash[MQUOTE + 'r'] = QByteArray(1, '\r');
    _ctcpMDequoteHash[MQUOTE + MQUOTE] = MQUOTE;

    setStandardCtcp(_coreSession->networkConfig()->standardCtcp());

    connect(_coreSession->networkConfig(), SIGNAL(standardCtcpSet(bool)), this, SLOT(setStandardCtcp(bool)));
    connect(this, SIGNAL(newEvent(Event *)), _coreSession->eventManager(), SLOT(postEvent(Event *)));
}


void CtcpParser::setStandardCtcp(bool enabled)
{
    QByteArray XQUOTE = QByteArray("\134");
    if (enabled)
        _ctcpXDelimDequoteHash[XQUOTE + XQUOTE] = XQUOTE;
    else
        _ctcpXDelimDequoteHash.remove(XQUOTE + XQUOTE);
    _ctcpXDelimDequoteHash[XQUOTE + QByteArray("a")] = XDELIM;
}


void CtcpParser::displayMsg(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
    const QString &target, Message::Flags msgFlags)
{
    if (event->testFlag(EventManager::Silent))
        return;

    MessageEvent *msgEvent = new MessageEvent(msgType, event->network(), msg, sender, target, msgFlags);
    msgEvent->setTimestamp(event->timestamp());

    emit newEvent(msgEvent);
}


QByteArray CtcpParser::lowLevelQuote(const QByteArray &message)
{
    QByteArray quotedMessage = message;

    QHash<QByteArray, QByteArray> quoteHash = _ctcpMDequoteHash;
    QByteArray MQUOTE = QByteArray("\020");
    quoteHash.remove(MQUOTE + MQUOTE);
    quotedMessage.replace(MQUOTE, MQUOTE + MQUOTE);

    QHash<QByteArray, QByteArray>::const_iterator quoteIter = quoteHash.constBegin();
    while (quoteIter != quoteHash.constEnd()) {
        quotedMessage.replace(quoteIter.value(), quoteIter.key());
        quoteIter++;
    }
    return quotedMessage;
}


QByteArray CtcpParser::lowLevelDequote(const QByteArray &message)
{
    QByteArray dequotedMessage;
    QByteArray messagepart;
    QHash<QByteArray, QByteArray>::iterator ctcpquote;

    // copy dequote Message
    for (int i = 0; i < message.size(); i++) {
        messagepart = message.mid(i, 1);
        if (i+1 < message.size()) {
            for (ctcpquote = _ctcpMDequoteHash.begin(); ctcpquote != _ctcpMDequoteHash.end(); ++ctcpquote) {
                if (message.mid(i, 2) == ctcpquote.key()) {
                    messagepart = ctcpquote.value();
                    ++i;
                    break;
                }
            }
        }
        dequotedMessage += messagepart;
    }
    return dequotedMessage;
}


QByteArray CtcpParser::xdelimQuote(const QByteArray &message)
{
    QByteArray quotedMessage = message;
    QHash<QByteArray, QByteArray>::const_iterator quoteIter = _ctcpXDelimDequoteHash.constBegin();
    while (quoteIter != _ctcpXDelimDequoteHash.constEnd()) {
        quotedMessage.replace(quoteIter.value(), quoteIter.key());
        quoteIter++;
    }
    return quotedMessage;
}


QByteArray CtcpParser::xdelimDequote(const QByteArray &message)
{
    QByteArray dequotedMessage;
    QByteArray messagepart;
    QHash<QByteArray, QByteArray>::iterator xdelimquote;

    for (int i = 0; i < message.size(); i++) {
        messagepart = message.mid(i, 1);
        if (i+1 < message.size()) {
            for (xdelimquote = _ctcpXDelimDequoteHash.begin(); xdelimquote != _ctcpXDelimDequoteHash.end(); ++xdelimquote) {
                if (message.mid(i, 2) == xdelimquote.key()) {
                    messagepart = xdelimquote.value();
                    i++;
                    break;
                }
            }
        }
        dequotedMessage += messagepart;
    }
    return dequotedMessage;
}


void CtcpParser::processIrcEventRawNotice(IrcEventRawMessage *event)
{
    parse(event, Message::Notice);
}


void CtcpParser::processIrcEventRawPrivmsg(IrcEventRawMessage *event)
{
    parse(event, Message::Plain);
}


void CtcpParser::parse(IrcEventRawMessage *e, Message::Type messagetype)
{
    //lowlevel message dequote
    QByteArray dequotedMessage = lowLevelDequote(e->rawMessage());

    CtcpEvent::CtcpType ctcptype = e->type() == EventManager::IrcEventRawNotice
                                   ? CtcpEvent::Reply
                                   : CtcpEvent::Query;

    Message::Flags flags = (ctcptype == CtcpEvent::Reply && !e->network()->isChannelName(e->target()))
                           ? Message::Redirected
                           : Message::None;

    if (coreSession()->networkConfig()->standardCtcp())
        parseStandard(e, messagetype, dequotedMessage, ctcptype, flags);
    else
        parseSimple(e, messagetype, dequotedMessage, ctcptype, flags);
}


// only accept CTCPs in their simplest form, i.e. one ctcp, from start to
// end, no text around it; not as per the 'specs', but makes people happier
void CtcpParser::parseSimple(IrcEventRawMessage *e, Message::Type messagetype, QByteArray dequotedMessage, CtcpEvent::CtcpType ctcptype, Message::Flags flags)
{
    if (dequotedMessage.count(XDELIM) != 2 || dequotedMessage[0] != '\001' || dequotedMessage[dequotedMessage.count() -1] != '\001') {
        displayMsg(e, messagetype, targetDecode(e, dequotedMessage), e->prefix(), e->target(), flags);
    } else {
        int spacePos = -1;
        QString ctcpcmd, ctcpparam;

        QByteArray ctcp = xdelimDequote(dequotedMessage.mid(1, dequotedMessage.count() - 2));
        spacePos = ctcp.indexOf(' ');
        if (spacePos != -1) {
            ctcpcmd = targetDecode(e, ctcp.left(spacePos));
            ctcpparam = targetDecode(e, ctcp.mid(spacePos + 1));
        } else {
            ctcpcmd = targetDecode(e, ctcp);
            ctcpparam = QString();
        }
        ctcpcmd = ctcpcmd.toUpper();

        // we don't want to block /me messages by the CTCP ignore list
        if (ctcpcmd == QLatin1String("ACTION") || !coreSession()->ignoreListManager()->ctcpMatch(e->prefix(), e->network()->networkName(), ctcpcmd)) {
            QUuid uuid = QUuid::createUuid();
            _replies.insert(uuid, CtcpReply(coreNetwork(e), nickFromMask(e->prefix())));
            CtcpEvent *event = new CtcpEvent(EventManager::CtcpEvent, e->network(), e->prefix(), e->target(),
                ctcptype, ctcpcmd, ctcpparam, e->timestamp(), uuid);
            emit newEvent(event);
            CtcpEvent *flushEvent = new CtcpEvent(EventManager::CtcpEventFlush, e->network(), e->prefix(), e->target(),
                ctcptype, "INVALID", QString(), e->timestamp(), uuid);
            emit newEvent(flushEvent);
        }
    }
}


void CtcpParser::parseStandard(IrcEventRawMessage *e, Message::Type messagetype, QByteArray dequotedMessage, CtcpEvent::CtcpType ctcptype, Message::Flags flags)
{
    QByteArray ctcp;

    QList<CtcpEvent *> ctcpEvents;
    QUuid uuid; // needed to group all replies together

    // extract tagged / extended data
    int xdelimPos = -1;
    int xdelimEndPos = -1;
    int spacePos = -1;
    while ((xdelimPos = dequotedMessage.indexOf(XDELIM)) != -1) {
        if (xdelimPos > 0)
            displayMsg(e, messagetype, targetDecode(e, dequotedMessage.left(xdelimPos)), e->prefix(), e->target(), flags);

        xdelimEndPos = dequotedMessage.indexOf(XDELIM, xdelimPos + 1);
        if (xdelimEndPos == -1) {
            // no matching end delimiter found... treat rest of the message as ctcp
            xdelimEndPos = dequotedMessage.count();
        }
        ctcp = xdelimDequote(dequotedMessage.mid(xdelimPos + 1, xdelimEndPos - xdelimPos - 1));
        dequotedMessage = dequotedMessage.mid(xdelimEndPos + 1);

        //dispatch the ctcp command
        QString ctcpcmd = targetDecode(e, ctcp.left(spacePos));
        QString ctcpparam = targetDecode(e, ctcp.mid(spacePos + 1));

        spacePos = ctcp.indexOf(' ');
        if (spacePos != -1) {
            ctcpcmd = targetDecode(e, ctcp.left(spacePos));
            ctcpparam = targetDecode(e, ctcp.mid(spacePos + 1));
        }
        else {
            ctcpcmd = targetDecode(e, ctcp);
            ctcpparam = QString();
        }

        ctcpcmd = ctcpcmd.toUpper();

        // we don't want to block /me messages by the CTCP ignore list
        if (ctcpcmd == QLatin1String("ACTION") || !coreSession()->ignoreListManager()->ctcpMatch(e->prefix(), e->network()->networkName(), ctcpcmd)) {
            if (uuid.isNull())
                uuid = QUuid::createUuid();

            CtcpEvent *event = new CtcpEvent(EventManager::CtcpEvent, e->network(), e->prefix(), e->target(),
                ctcptype, ctcpcmd, ctcpparam, e->timestamp(), uuid);
            ctcpEvents << event;
        }
    }
    if (!ctcpEvents.isEmpty()) {
        _replies.insert(uuid, CtcpReply(coreNetwork(e), nickFromMask(e->prefix())));
        CtcpEvent *flushEvent = new CtcpEvent(EventManager::CtcpEventFlush, e->network(), e->prefix(), e->target(),
            ctcptype, "INVALID", QString(), e->timestamp(), uuid);
        ctcpEvents << flushEvent;
        foreach(CtcpEvent *event, ctcpEvents) {
            emit newEvent(event);
        }
    }

    if (!dequotedMessage.isEmpty())
        displayMsg(e, messagetype, targetDecode(e, dequotedMessage), e->prefix(), e->target(), flags);
}


void CtcpParser::sendCtcpEvent(CtcpEvent *e)
{
    CoreNetwork *net = coreNetwork(e);
    if (e->type() == EventManager::CtcpEvent) {
        QByteArray quotedReply;
        QString bufname = nickFromMask(e->prefix());
        if (e->ctcpType() == CtcpEvent::Query && !e->reply().isNull()) {
            if (_replies.contains(e->uuid()))
                _replies[e->uuid()].replies << lowLevelQuote(pack(net->serverEncode(e->ctcpCmd()),
                        net->userEncode(bufname, e->reply())));
            else
                // reply not caused by a request processed in here, so send it off immediately
                reply(net, bufname, e->ctcpCmd(), e->reply());
        }
    }
    else if (e->type() == EventManager::CtcpEventFlush && _replies.contains(e->uuid())) {
        CtcpReply reply = _replies.take(e->uuid());
        if (reply.replies.count())
            packedReply(net, reply.bufferName, reply.replies);
    }
}


QByteArray CtcpParser::pack(const QByteArray &ctcpTag, const QByteArray &message)
{
    if (message.isEmpty())
        return XDELIM + ctcpTag + XDELIM;

    return XDELIM + ctcpTag + ' ' + xdelimQuote(message) + XDELIM;
}


void CtcpParser::query(CoreNetwork *net, const QString &bufname, const QString &ctcpTag, const QString &message)
{
    static const char *cmd = "PRIVMSG";
    QByteArray bufnameEnc = net->serverEncode(bufname);
    QByteArray ctcpTagEnc = net->serverEncode(ctcpTag);
    
    QString wrkMsg(message);
    int splitPos = wrkMsg.size();
    int prevSplitPos = 0;
    QTextBoundaryFinder::BoundaryType boundaryType = QTextBoundaryFinder::Word;
    while (wrkMsg.size() > 0) {
        // Detect if we aren't making any progress and stop to a avoid an infinite loop.
        if (splitPos == prevSplitPos) {
            qWarning() << "Failed to split message";
            break;
        }
        prevSplitPos = splitPos;
        
        QString splitMsg(wrkMsg.left(splitPos));
        QByteArray splitMsgEnc = net->userEncode(bufname, splitMsg);
        
        int overrun = net->userInputHandler()->lastParamOverrun(cmd, QList<QByteArray>() << bufnameEnc << lowLevelQuote(pack(ctcpTagEnc, splitMsgEnc)));
        if (overrun) {
            // If the message was too long to be sent, use QTextBoundaryFinder to
            // locate a logical boundary for splitting the message.  We start at 
            // splitMsgEnc.size()-overrun (which is the maximum byte length for
            // messages to be sent) because it is known that that will be somewhere
            // at or after the point where we need to split, so we can start there
            // and work backwards until a usable split point is found.
            QTextBoundaryFinder qtbf(boundaryType, splitMsg);
            qtbf.setPosition(splitMsgEnc.size() - overrun);
            splitPos = qtbf.toPreviousBoundary();
            if (splitPos < 1) {
                // If a valid split point could not be found. then switch to Grapheme mode
                // and start working backwards from the end of the message again.  It is
                // necessary to set prevSplitPos to 0 to avoid erroneously activating the
                // code that halts the splitting due to lack of progress.
                boundaryType = QTextBoundaryFinder::Grapheme;
                splitPos = wrkMsg.size();
                prevSplitPos = 0;
            }
        }
        else {
            // If the message fits within the byte limit, switch back to Word mode
            // (in case we had to fall back to Grapheme mode), chop the message to
            // be sent off of the message remaining to be sent, reset splitPos to
            // the length of the new message remaining to be sent, and send the message.
            boundaryType = QTextBoundaryFinder::Word;
            wrkMsg.remove(0, splitPos);
            splitPos = wrkMsg.size();
            net->putCmd(cmd, QList<QByteArray>() << bufnameEnc << lowLevelQuote(pack(ctcpTagEnc, splitMsgEnc)));
        }
    }
}


void CtcpParser::reply(CoreNetwork *net, const QString &bufname, const QString &ctcpTag, const QString &message)
{
    QList<QByteArray> params;
    params << net->serverEncode(bufname) << lowLevelQuote(pack(net->serverEncode(ctcpTag), net->userEncode(bufname, message)));
    net->putCmd("NOTICE", params);
}


void CtcpParser::packedReply(CoreNetwork *net, const QString &bufname, const QList<QByteArray> &replies)
{
    QList<QByteArray> params;

    int answerSize = 0;
    for (int i = 0; i < replies.count(); i++) {
        answerSize += replies.at(i).size();
    }

    QByteArray quotedReply;
    quotedReply.reserve(answerSize);
    for (int i = 0; i < replies.count(); i++) {
        quotedReply.append(replies.at(i));
    }

    params << net->serverEncode(bufname) << quotedReply;
    // FIXME user proper event
    net->putCmd("NOTICE", params);
}
