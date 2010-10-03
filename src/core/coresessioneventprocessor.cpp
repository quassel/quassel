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
