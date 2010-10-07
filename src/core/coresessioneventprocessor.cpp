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

#include "coresessioneventprocessor.h"

#include "corenetwork.h"
#include "coresession.h"
#include "ircevent.h"

CoreSessionEventProcessor::CoreSessionEventProcessor(CoreSession *session)
  : QObject(session),
  _coreSession(session)
{

}

bool CoreSessionEventProcessor::checkParamCount(IrcEvent *e, int minParams) {
  if(e->params().count() < minParams) {
    if(e->type() == EventManager::IrcEventNumeric) {
      qWarning() << "Command " << static_cast<IrcEventNumeric *>(e)->number() << " requires " << minParams << "params, got: " << e->params();
    } else {
      QString name = coreSession()->eventManager()->enumName(e->type());
      qWarning() << qPrintable(name) << "requires" << minParams << "params, got:" << e->params();
    }
    e->stop();
    return false;
  }
  return true;
}

void CoreSessionEventProcessor::processIrcEventNumeric(IrcEventNumeric *e) {
  switch(e->number()) {

  // CAP stuff
  case 903: case 904: case 905: case 906: case 907:
    qobject_cast<CoreNetwork *>(e->network())->putRawLine("CAP END");
    break;

  default:
    break;
  }
}

void CoreSessionEventProcessor::processIrcEventAuthenticate(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  if(e->params().at(0) != "+") {
    qWarning() << "Invalid AUTHENTICATE" << e;
    return;
  }

  CoreNetwork *net = coreNetwork(e);

  QString construct = net->saslAccount();
  construct.append(QChar(QChar::Null));
  construct.append(net->saslAccount());
  construct.append(QChar(QChar::Null));
  construct.append(net->saslPassword());
  QByteArray saslData = QByteArray(construct.toAscii().toBase64());
  saslData.prepend("AUTHENTICATE ");
  net->putRawLine(saslData);
}

void CoreSessionEventProcessor::processIrcEventCap(IrcEvent *e) {
  // for SASL, there will only be a single param of 'sasl', however you can check here for
  // additional CAP messages (ls, multi-prefix, et cetera).

  if(e->params().count() == 3) {
    if(e->params().at(2) == "sasl") {
      // FIXME use event
      coreNetwork(e)->putRawLine(coreNetwork(e)->serverEncode("AUTHENTICATE PLAIN")); // Only working with PLAIN atm, blowfish later
    }
  }
}

void CoreSessionEventProcessor::processIrcEventInvite(IrcEvent *e) {
  if(checkParamCount(e, 2)) {
    e->network()->updateNickFromMask(e->prefix());
  }
}

void CoreSessionEventProcessor::processIrcEventKick(IrcEvent *e) {
  if(checkParamCount(e, 2)) {
    e->network()->updateNickFromMask(e->prefix());
    IrcUser *victim = e->network()->ircUser(e->params().at(1));
    if(victim) {
      victim->partChannel(e->params().at(0));
      //if(e->network()->isMe(victim)) e->network()->setKickedFromChannel(channel);
    }
  }
}

void CoreSessionEventProcessor::processIrcEventNick(IrcEvent *e) {
  if(checkParamCount(e, 1)) {
    IrcUser *ircuser = e->network()->updateNickFromMask(e->prefix());
    if(!ircuser) {
      qWarning() << Q_FUNC_INFO << "Unknown IrcUser!";
      return;
    }
    QString newnick = e->params().at(0);
    QString oldnick = ircuser->nick();

    // the order is cruicial
    // otherwise the client would rename the buffer, see that the assigned ircuser doesn't match anymore
    // and remove the ircuser from the querybuffer leading to a wrong on/offline state
    ircuser->setNick(newnick);
    coreSession()->renameBuffer(e->networkId(), newnick, oldnick);
  }
}

void CoreSessionEventProcessor::processIrcEventPart(IrcEvent *e) {
  if(checkParamCount(e, 1)) {
    IrcUser *ircuser = e->network()->updateNickFromMask(e->prefix());
    if(!ircuser) {
      qWarning() << Q_FUNC_INFO<< "Unknown IrcUser!";
      return;
    }
    QString channel = e->params().at(0);
    ircuser->partChannel(channel);
    if(e->network()->isMe(ircuser))
      qobject_cast<CoreNetwork *>(e->network())->setChannelParted(channel);
  }
}

void CoreSessionEventProcessor::processIrcEventPong(IrcEvent *e) {
  // the server is supposed to send back what we passed as param. and we send a timestamp
  // but using quote and whatnought one can send arbitrary pings, so we have to do some sanity checks
  if(checkParamCount(e, 2)) {
    QString timestamp = e->params().at(1);
    QTime sendTime = QTime::fromString(timestamp, "hh:mm:ss.zzz");
    if(sendTime.isValid())
      e->network()->setLatency(sendTime.msecsTo(QTime::currentTime()) / 2);
  }
}

void CoreSessionEventProcessor::processIrcEventTopic(IrcEvent *e) {
  if(checkParamCount(e, 2)) {
    e->network()->updateNickFromMask(e->prefix());
    IrcChannel *channel = e->network()->ircChannel(e->params().at(0));
    if(channel)
      channel->setTopic(e->params().at(1));
  }
}

/* RPL_WELCOME */
void CoreSessionEventProcessor::processIrcEvent001(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  QString myhostmask = e->params().at(0).section(' ', -1, -1);
  e->network()->setCurrentServer(e->prefix());
  e->network()->setMyNick(nickFromMask(myhostmask));
}

/* RPL_UMODEIS - "<user_modes> [<user_mode_params>]" */
void CoreSessionEventProcessor::processIrcEvent221(IrcEvent *) {
  // TODO: save information in network object
}

/* RPL_STATSCONN - "Highest connection cout: 8000 (7999 clients)" */
void CoreSessionEventProcessor::processIrcEvent250(IrcEvent *) {
  // TODO: save information in network object
}

/* RPL_LOCALUSERS - "Current local user: 5024  Max: 7999 */
void CoreSessionEventProcessor::processIrcEvent265(IrcEvent *) {
  // TODO: save information in network object
}

/* RPL_GLOBALUSERS - "Current global users: 46093  Max: 47650" */
void CoreSessionEventProcessor::processIrcEvent266(IrcEvent *) {
  // TODO: save information in network object
}

/*
WHOIS-Message:
   Replies 311 - 313, 317 - 319 are all replies generated in response to a WHOIS message.
  and 301 (RPL_AWAY)
              "<nick> :<away message>"
WHO-Message:
   Replies 352 and 315 paired are used to answer a WHO message.

WHOWAS-Message:
   Replies 314 and 369 are responses to a WHOWAS message.

*/

/* RPL_AWAY - "<nick> :<away message>" */
void CoreSessionEventProcessor::processIrcEvent301(IrcEvent *e) {
  if(!checkParamCount(e, 2))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser) {
    ircuser->setAway(true);
    ircuser->setAwayMessage(e->params().at(1));
    //ircuser->setLastAwayMessage(now);
  }
}

/* RPL_UNAWAY - ":You are no longer marked as being away" */
void CoreSessionEventProcessor::processIrcEvent305(IrcEvent *e) {
  IrcUser *me = e->network()->me();
  if(me)
    me->setAway(false);

  if(e->network()->autoAwayActive()) {
    e->network()->setAutoAwayActive(false);
    e->setFlag(EventManager::Silent);
  }
}

/* RPL_NOWAWAY - ":You have been marked as being away" */
void CoreSessionEventProcessor::processIrcEvent306(IrcEvent *e) {
  IrcUser *me = e->network()->me();
  if(me)
    me->setAway(true);
}

/* template
void CoreSessionEventProcessor::processIrcEvent(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

}
*/
