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

#include "eventstringifier.h"

#include "coresession.h"
#include "messageevent.h"

EventStringifier::EventStringifier(CoreSession *parent) : QObject(parent),
  _coreSession(parent),
  _whois(false)
{

}

void EventStringifier::displayMsg(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
                                  const QString &target, Message::Flags msgFlags) {
  MessageEvent *msgEvent = createMessageEvent(event, msgType, msg, sender, target, msgFlags);
  sendMessageEvent(msgEvent);
}

MessageEvent *EventStringifier::createMessageEvent(NetworkEvent *event, Message::Type msgType, const QString &msg, const QString &sender,
                        const QString &target, Message::Flags msgFlags) {
  MessageEvent *msgEvent = new MessageEvent(msgType, event->network(), msg, sender, target, msgFlags);
  return msgEvent;
}

void EventStringifier::sendMessageEvent(MessageEvent *event) {
  coreSession()->eventManager()->sendEvent(event);
}

void EventStringifier::processIrcEventNumeric(IrcEventNumeric *e) {
  //qDebug() << e->number();
  switch(e->number()) {
  // Welcome, status, info messages. Just display these.
  case 2: case 3: case 4: case 5: case 251: case 252: case 253: case 254: case 255: case 372: case 375:
    displayMsg(e, Message::Server, e->params().join(" "), e->prefix());
    break;

  // Server error messages without param, just display them
  case 409: case 411: case 412: case 422: case 424: case 445: case 446: case 451: case 462:
  case 463: case 464: case 465: case 466: case 472: case 481: case 483: case 485: case 491: case 501: case 502:
  case 431: // ERR_NONICKNAMEGIVEN
    displayMsg(e, Message::Error, e->params().join(" "), e->prefix());
    break;

  // Server error messages, display them in red. First param will be appended.
  case 401: {
    QString target = e->params().takeFirst();
    displayMsg(e, Message::Error, e->params().join(" ") + " " + target, e->prefix(), target, Message::Redirected);
    break;
  }

  case 402: case 403: case 404: case 406: case 408: case 415: case 421: case 442: {
    QString channelName = e->params().takeFirst();
    displayMsg(e, Message::Error, e->params().join(" ") + " " + channelName, e->prefix());
    break;
  }

  // Server error messages which will be displayed with a colon between the first param and the rest
  case 413: case 414: case 423: case 441: case 444: case 461:  // FIXME see below for the 47x codes
  case 467: case 471: case 473: case 474: case 475: case 476: case 477: case 478: case 482:
  case 436: // ERR_NICKCOLLISION
  {
    QString p = e->params().takeFirst();
    displayMsg(e, Message::Error, p + ": " + e->params().join(" "));
    break;
  }

  // Ignore these commands.
  case 321: case 366: case 376:
    break;

  // CAP stuff
  case 903: case 904: case 905: case 906: case 907:
  {
    displayMsg(e, Message::Info, "CAP: " + e->params().join(""));
    break;
  }

  // Everything else will be marked in red, so we can add them somewhere.
  default:
    if(_whois) {
      // many nets define their own WHOIS fields. we fetch those not in need of special attention here:
      displayMsg(e, Message::Server, "[Whois] " + e->params().join(" "), e->prefix());
    } else {
      // FIXME figure out how/where to do this in the future
      //if(coreSession()->ircListHelper()->requestInProgress(network()->networkId()))
      //  coreSession()->ircListHelper()->reportError(params.join(" "));
      //else
        displayMsg(e, Message::Error, QString("%1 %2").arg(e->number(), 3, 10, QLatin1Char('0')).arg(e->params().join(" ")), e->prefix());
    }
  }
}
