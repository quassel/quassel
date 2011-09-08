/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ctcpparser.h"

#include "coresession.h"
#include "ctcpevent.h"
#include "messageevent.h"

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

  QByteArray XQUOTE = QByteArray("\134");
  _ctcpXDelimDequoteHash[XQUOTE + XQUOTE] = XQUOTE;
  _ctcpXDelimDequoteHash[XQUOTE + QByteArray("a")] = XDELIM;

  connect(this, SIGNAL(newEvent(Event *)), _coreSession->eventManager(), SLOT(postEvent(Event *)));
}

void CtcpParser::displayMsg(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
                            const QString &target, Message::Flags msgFlags) {
  if(event->testFlag(EventManager::Silent))
    return;

  MessageEvent *msgEvent = new MessageEvent(msgType, event->network(), msg, sender, target, msgFlags);
  msgEvent->setTimestamp(event->timestamp());

  emit newEvent(msgEvent);
}

QByteArray CtcpParser::lowLevelQuote(const QByteArray &message) {
  QByteArray quotedMessage = message;

  QHash<QByteArray, QByteArray> quoteHash = _ctcpMDequoteHash;
  QByteArray MQUOTE = QByteArray("\020");
  quoteHash.remove(MQUOTE + MQUOTE);
  quotedMessage.replace(MQUOTE, MQUOTE + MQUOTE);

  QHash<QByteArray, QByteArray>::const_iterator quoteIter = quoteHash.constBegin();
  while(quoteIter != quoteHash.constEnd()) {
    quotedMessage.replace(quoteIter.value(), quoteIter.key());
    quoteIter++;
  }
  return quotedMessage;
}

QByteArray CtcpParser::lowLevelDequote(const QByteArray &message) {
  QByteArray dequotedMessage;
  QByteArray messagepart;
  QHash<QByteArray, QByteArray>::iterator ctcpquote;

  // copy dequote Message
  for(int i = 0; i < message.size(); i++) {
    messagepart = message.mid(i,1);
    if(i+1 < message.size()) {
      for(ctcpquote = _ctcpMDequoteHash.begin(); ctcpquote != _ctcpMDequoteHash.end(); ++ctcpquote) {
        if(message.mid(i,2) == ctcpquote.key()) {
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

QByteArray CtcpParser::xdelimQuote(const QByteArray &message) {
  QByteArray quotedMessage = message;
  QHash<QByteArray, QByteArray>::const_iterator quoteIter = _ctcpXDelimDequoteHash.constBegin();
  while(quoteIter != _ctcpXDelimDequoteHash.constEnd()) {
    quotedMessage.replace(quoteIter.value(), quoteIter.key());
    quoteIter++;
  }
  return quotedMessage;
}

QByteArray CtcpParser::xdelimDequote(const QByteArray &message) {
  QByteArray dequotedMessage;
  QByteArray messagepart;
  QHash<QByteArray, QByteArray>::iterator xdelimquote;

  for(int i = 0; i < message.size(); i++) {
    messagepart = message.mid(i,1);
    if(i+1 < message.size()) {
      for(xdelimquote = _ctcpXDelimDequoteHash.begin(); xdelimquote != _ctcpXDelimDequoteHash.end(); ++xdelimquote) {
        if(message.mid(i,2) == xdelimquote.key()) {
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

void CtcpParser::processIrcEventRawNotice(IrcEventRawMessage *event) {
  parse(event, Message::Notice);
}

void CtcpParser::processIrcEventRawPrivmsg(IrcEventRawMessage *event) {
  parse(event, Message::Plain);
}

void CtcpParser::parse(IrcEventRawMessage *e, Message::Type messagetype) {
  QByteArray ctcp;

  //lowlevel message dequote
  QByteArray dequotedMessage = lowLevelDequote(e->rawMessage());

  CtcpEvent::CtcpType ctcptype = e->type() == EventManager::IrcEventRawNotice
      ? CtcpEvent::Reply
      : CtcpEvent::Query;

  Message::Flags flags = (ctcptype == CtcpEvent::Reply && !e->network()->isChannelName(e->target()))
                          ? Message::Redirected
                          : Message::None;

  QList<CtcpEvent *> ctcpEvents;
  QUuid uuid; // needed to group all replies together

  // extract tagged / extended data
  int xdelimPos = -1;
  int xdelimEndPos = -1;
  int spacePos = -1;
  while((xdelimPos = dequotedMessage.indexOf(XDELIM)) != -1) {
    if(xdelimPos > 0)
      displayMsg(e, messagetype, targetDecode(e, dequotedMessage.left(xdelimPos)), e->prefix(), e->target(), flags);

    xdelimEndPos = dequotedMessage.indexOf(XDELIM, xdelimPos + 1);
    if(xdelimEndPos == -1) {
      // no matching end delimiter found... treat rest of the message as ctcp
      xdelimEndPos = dequotedMessage.count();
    }
    ctcp = xdelimDequote(dequotedMessage.mid(xdelimPos + 1, xdelimEndPos - xdelimPos - 1));
    dequotedMessage = dequotedMessage.mid(xdelimEndPos + 1);

    //dispatch the ctcp command
    QString ctcpcmd = targetDecode(e, ctcp.left(spacePos));
    QString ctcpparam = targetDecode(e, ctcp.mid(spacePos + 1));

    spacePos = ctcp.indexOf(' ');
    if(spacePos != -1) {
      ctcpcmd = targetDecode(e, ctcp.left(spacePos));
      ctcpparam = targetDecode(e, ctcp.mid(spacePos + 1));
    } else {
      ctcpcmd = targetDecode(e, ctcp);
      ctcpparam = QString();
    }

    ctcpcmd = ctcpcmd.toUpper();

    // we don't want to block /me messages by the CTCP ignore list
    if(ctcpcmd == QLatin1String("ACTION") || !coreSession()->ignoreListManager()->ctcpMatch(e->prefix(), e->network()->networkName(), ctcpcmd)) {
      if(uuid.isNull())
        uuid = QUuid::createUuid();

      CtcpEvent *event = new CtcpEvent(EventManager::CtcpEvent, e->network(), e->prefix(), e->target(),
                                       ctcptype, ctcpcmd, ctcpparam, e->timestamp(), uuid);
      ctcpEvents << event;
    }
  }
  if(!ctcpEvents.isEmpty()) {
    _replies.insert(uuid, CtcpReply(coreNetwork(e), nickFromMask(e->prefix())));
    CtcpEvent *flushEvent = new CtcpEvent(EventManager::CtcpEventFlush, e->network(), e->prefix(), e->target(),
                                          ctcptype, "INVALID", QString(), e->timestamp(), uuid);
    ctcpEvents << flushEvent;
    foreach(CtcpEvent *event, ctcpEvents) {
      emit newEvent(event);
    }
  }

  if(!dequotedMessage.isEmpty())
    displayMsg(e, messagetype, targetDecode(e, dequotedMessage), e->prefix(), e->target(), flags);
}

void CtcpParser::sendCtcpEvent(CtcpEvent *e) {
  CoreNetwork *net = coreNetwork(e);
  if(e->type() == EventManager::CtcpEvent) {
    QByteArray quotedReply;
    QString bufname = nickFromMask(e->prefix());
    if(e->ctcpType() == CtcpEvent::Query && !e->reply().isNull()) {
      if(_replies.contains(e->uuid()))
        _replies[e->uuid()].replies << lowLevelQuote(pack(net->serverEncode(e->ctcpCmd()),
                                                          net->userEncode(bufname, e->reply())));
      else
        // reply not caused by a request processed in here, so send it off immediately
        reply(net, bufname, e->ctcpCmd(), e->reply());
    }
  } else if(e->type() == EventManager::CtcpEventFlush && _replies.contains(e->uuid())) {
    CtcpReply reply = _replies.take(e->uuid());
    if(reply.replies.count())
      packedReply(net, reply.bufferName, reply.replies);
  }
}

QByteArray CtcpParser::pack(const QByteArray &ctcpTag, const QByteArray &message) {
  if(message.isEmpty())
    return XDELIM + ctcpTag + XDELIM;

  return XDELIM + ctcpTag + ' ' + xdelimQuote(message) + XDELIM;
}

void CtcpParser::query(CoreNetwork *net, const QString &bufname, const QString &ctcpTag, const QString &message) {
  QList<QByteArray> params;
  params << net->serverEncode(bufname) << lowLevelQuote(pack(net->serverEncode(ctcpTag), net->userEncode(bufname, message)));
  net->putCmd("PRIVMSG", params);
}

void CtcpParser::reply(CoreNetwork *net, const QString &bufname, const QString &ctcpTag, const QString &message) {
  QList<QByteArray> params;
  params << net->serverEncode(bufname) << lowLevelQuote(pack(net->serverEncode(ctcpTag), net->userEncode(bufname, message)));
  net->putCmd("NOTICE", params);
}

void CtcpParser::packedReply(CoreNetwork *net, const QString &bufname, const QList<QByteArray> &replies) {
  QList<QByteArray> params;

  int answerSize = 0;
  for(int i = 0; i < replies.count(); i++) {
    answerSize += replies.at(i).size();
  }

  QByteArray quotedReply;
  quotedReply.reserve(answerSize);
  for(int i = 0; i < replies.count(); i++) {
    quotedReply.append(replies.at(i));
  }

  params << net->serverEncode(bufname) << quotedReply;
  // FIXME user proper event
  net->putCmd("NOTICE", params);
}
