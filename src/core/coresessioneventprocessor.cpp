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

#include "coreirclisthelper.h"
#include "corenetwork.h"
#include "coresession.h"
#include "ctcpevent.h"
#include "ircevent.h"
#include "ircuser.h"
#include "messageevent.h"
#include "netsplit.h"
#include "quassel.h"

CoreSessionEventProcessor::CoreSessionEventProcessor(CoreSession *session)
  : BasicHandler("handleCtcp", session),
  _coreSession(session)
{
  connect(coreSession(), SIGNAL(networkDisconnected(NetworkId)), this, SLOT(destroyNetsplits(NetworkId)));
  connect(this, SIGNAL(newEvent(Event *)), coreSession()->eventManager(), SLOT(postEvent(Event *)));
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

void CoreSessionEventProcessor::tryNextNick(NetworkEvent *e, const QString &errnick, bool erroneus) {
  QStringList desiredNicks = coreSession()->identity(e->network()->identity())->nicks();
  int nextNickIdx = desiredNicks.indexOf(errnick) + 1;
  QString nextNick;
  if(nextNickIdx > 0 && desiredNicks.size() > nextNickIdx) {
    nextNick = desiredNicks[nextNickIdx];
  } else {
    if(erroneus) {
      // FIXME Make this an ErrorEvent or something like that, so it's translated in the client
      MessageEvent *msgEvent = new MessageEvent(Message::Error, e->network(),
                                                tr("No free and valid nicks in nicklist found. use: /nick <othernick> to continue"),
                                                QString(), QString(), Message::None, e->timestamp());
      emit newEvent(msgEvent);
      return;
    } else {
      nextNick = errnick + "_";
    }
  }
  // FIXME Use a proper output event for this
  coreNetwork(e)->putRawLine("NICK " + coreNetwork(e)->encodeServerString(nextNick));
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

void CoreSessionEventProcessor::processIrcEventJoin(IrcEvent *e) {
  if(e->testFlag(EventManager::Fake)) // generated by handleEarlyNetsplitJoin
    return;

  if(!checkParamCount(e, 1))
    return;

  CoreNetwork *net = coreNetwork(e);
  QString channel = e->params()[0];
  IrcUser *ircuser = net->updateNickFromMask(e->prefix());

  bool handledByNetsplit = false;
  foreach(Netsplit* n, _netsplits.value(e->network())) {
    handledByNetsplit = n->userJoined(e->prefix(), channel);
    if(handledByNetsplit)
      break;
  }

  if(!handledByNetsplit)
    ircuser->joinChannel(channel);
  else
    e->setFlag(EventManager::Netsplit);

  if(net->isMe(ircuser)) {
    net->setChannelJoined(channel);
     // FIXME use event
    net->putRawLine(net->serverEncode("MODE " + channel)); // we want to know the modes of the channel we just joined, so we ask politely
  }
}

void CoreSessionEventProcessor::lateProcessIrcEventKick(IrcEvent *e) {
  if(checkParamCount(e, 2)) {
    e->network()->updateNickFromMask(e->prefix());
    IrcUser *victim = e->network()->ircUser(e->params().at(1));
    if(victim) {
      victim->partChannel(e->params().at(0));
      //if(e->network()->isMe(victim)) e->network()->setKickedFromChannel(channel);
    }
  }
}

void CoreSessionEventProcessor::processIrcEventMode(IrcEvent *e) {
  if(!checkParamCount(e, 2))
    return;

  if(e->network()->isChannelName(e->params().first())) {
    // Channel Modes

    IrcChannel *channel = e->network()->ircChannel(e->params()[0]);
    if(!channel) {
      // we received mode information for a channel we're not in. that means probably we've just been kicked out or something like that
      // anyways: we don't have a place to store the data --> discard the info.
      return;
    }

    QString modes = e->params()[1];
    bool add = true;
    int paramOffset = 2;
    for(int c = 0; c < modes.length(); c++) {
      if(modes[c] == '+') {
        add = true;
        continue;
      }
      if(modes[c] == '-') {
        add = false;
        continue;
      }

      if(e->network()->prefixModes().contains(modes[c])) {
        // user channel modes (op, voice, etc...)
        if(paramOffset < e->params().count()) {
          IrcUser *ircUser = e->network()->ircUser(e->params()[paramOffset]);
          if(!ircUser) {
            qWarning() << Q_FUNC_INFO << "Unknown IrcUser:" << e->params()[paramOffset];
          } else {
            if(add) {
              bool handledByNetsplit = false;
              QHash<QString, Netsplit *> splits = _netsplits.value(e->network());
              foreach(Netsplit* n, _netsplits.value(e->network())) {
                handledByNetsplit = n->userAlreadyJoined(ircUser->hostmask(), channel->name());
                if(handledByNetsplit) {
                  n->addMode(ircUser->hostmask(), channel->name(), QString(modes[c]));
                  break;
                }
              }
              if(!handledByNetsplit)
                channel->addUserMode(ircUser, QString(modes[c]));
            }
            else
              channel->removeUserMode(ircUser, QString(modes[c]));
          }
        } else {
          qWarning() << "Received MODE with too few parameters:" << e->params();
        }
        ++paramOffset;
      } else {
        // regular channel modes
        QString value;
        Network::ChannelModeType modeType = e->network()->channelModeType(modes[c]);
        if(modeType == Network::A_CHANMODE || modeType == Network::B_CHANMODE || (modeType == Network::C_CHANMODE && add)) {
          if(paramOffset < e->params().count()) {
            value = e->params()[paramOffset];
          } else {
            qWarning() << "Received MODE with too few parameters:" << e->params();
          }
          ++paramOffset;
        }

        if(add)
          channel->addChannelMode(modes[c], value);
        else
          channel->removeChannelMode(modes[c], value);
      }
    }

  } else {
    // pure User Modes
    IrcUser *ircUser = e->network()->newIrcUser(e->params().first());
    QString modeString(e->params()[1]);
    QString addModes;
    QString removeModes;
    bool add = false;
    for(int c = 0; c < modeString.count(); c++) {
      if(modeString[c] == '+') {
        add = true;
        continue;
      }
      if(modeString[c] == '-') {
        add = false;
        continue;
      }
      if(add)
        addModes += modeString[c];
      else
        removeModes += modeString[c];
    }
    if(!addModes.isEmpty())
      ircUser->addUserModes(addModes);
    if(!removeModes.isEmpty())
      ircUser->removeUserModes(removeModes);

    if(e->network()->isMe(ircUser)) {
      coreNetwork(e)->updatePersistentModes(addModes, removeModes);
    }
  }
}

void CoreSessionEventProcessor::lateProcessIrcEventNick(IrcEvent *e) {
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

void CoreSessionEventProcessor::lateProcessIrcEventPart(IrcEvent *e) {
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

void CoreSessionEventProcessor::processIrcEventPing(IrcEvent *e) {
  QString param = e->params().count()? e->params().first() : QString();
  // FIXME use events
  coreNetwork(e)->putRawLine("PONG " + coreNetwork(e)->serverEncode(param));
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

void CoreSessionEventProcessor::processIrcEventQuit(IrcEvent *e) {
  IrcUser *ircuser = e->network()->updateNickFromMask(e->prefix());
  if(!ircuser)
    return;

  QString msg;
  if(e->params().count() > 0)
    msg = e->params()[0];

  // check if netsplit
  if(Netsplit::isNetsplit(msg)) {
    Netsplit *n;
    if(!_netsplits[e->network()].contains(msg)) {
      n = new Netsplit(e->network(), this);
      connect(n, SIGNAL(finished()), this, SLOT(handleNetsplitFinished()));
      connect(n, SIGNAL(netsplitJoin(Network*,QString,QStringList,QStringList,QString)),
              this, SLOT(handleNetsplitJoin(Network*,QString,QStringList,QStringList,QString)));
      connect(n, SIGNAL(netsplitQuit(Network*,QString,QStringList,QString)),
              this, SLOT(handleNetsplitQuit(Network*,QString,QStringList,QString)));
      connect(n, SIGNAL(earlyJoin(Network*,QString,QStringList,QStringList)),
              this, SLOT(handleEarlyNetsplitJoin(Network*,QString,QStringList,QStringList)));
      _netsplits[e->network()].insert(msg, n);
    }
    else {
      n = _netsplits[e->network()][msg];
    }
    // add this user to the netsplit
    n->userQuit(e->prefix(), ircuser->channels(), msg);
    e->setFlag(EventManager::Netsplit);
  }
  // normal quit is handled in lateProcessIrcEventQuit()
}

void CoreSessionEventProcessor::lateProcessIrcEventQuit(IrcEvent *e) {
  if(e->testFlag(EventManager::Netsplit))
    return;

  IrcUser *ircuser = e->network()->updateNickFromMask(e->prefix());
  if(!ircuser)
    return;

  ircuser->quit();
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

/* RPL_ISUPPORT */
// TODO Complete 005 handling, also use sensible defaults for non-sent stuff
void CoreSessionEventProcessor::processIrcEvent005(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  QString key, value;
  for(int i = 0; i < e->params().count() - 1; i++) {
    QString key = e->params()[i].section("=", 0, 0);
    QString value = e->params()[i].section("=", 1);
    e->network()->addSupport(key, value);
  }

  /* determine our prefixes here to get an accurate result */
  e->network()->determinePrefixes();
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

/* RPL_WHOISSERVICE - "<user> is registered nick" */
void CoreSessionEventProcessor::processIrcEvent307(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser)
    ircuser->setWhoisServiceReply(e->params().join(" "));
}

/* RPL_SUSERHOST - "<user> is available for help." */
void CoreSessionEventProcessor::processIrcEvent310(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser)
    ircuser->setSuserHost(e->params().join(" "));
}

/*  RPL_WHOISUSER - "<nick> <user> <host> * :<real name>" */
void CoreSessionEventProcessor::processIrcEvent311(IrcEvent *e) {
  if(!checkParamCount(e, 3))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser) {
    ircuser->setUser(e->params().at(1));
    ircuser->setHost(e->params().at(2));
    ircuser->setRealName(e->params().last());
  }
}

/*  RPL_WHOISSERVER -  "<nick> <server> :<server info>" */
void CoreSessionEventProcessor::processIrcEvent312(IrcEvent *e) {
  if(!checkParamCount(e, 2))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser)
    ircuser->setServer(e->params().at(1));
}

/*  RPL_WHOISOPERATOR - "<nick> :is an IRC operator" */
void CoreSessionEventProcessor::processIrcEvent313(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  IrcUser *ircuser = e->network()->ircUser(e->params().at(0));
  if(ircuser)
    ircuser->setIrcOperator(e->params().last());
}

/*  RPL_ENDOFWHO: "<name> :End of WHO list" */
void CoreSessionEventProcessor::processIrcEvent315(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  if(coreNetwork(e)->setAutoWhoDone(e->params()[0]))
    e->setFlag(EventManager::Silent);
}

/*  RPL_WHOISIDLE - "<nick> <integer> :seconds idle"
   (real life: "<nick> <integer> <integer> :seconds idle, signon time) */
void CoreSessionEventProcessor::processIrcEvent317(IrcEvent *e) {
  if(!checkParamCount(e, 2))
    return;

  QDateTime loginTime;

  int idleSecs = e->params()[1].toInt();
  if(e->params().count() > 3) { // if we have more then 3 params we have the above mentioned "real life" situation
    int logintime = e->params()[2].toInt();
    loginTime = QDateTime::fromTime_t(logintime);
  }

  IrcUser *ircuser = e->network()->ircUser(e->params()[0]);
  if(ircuser) {
    ircuser->setIdleTime(e->timestamp().addSecs(-idleSecs));
    if(loginTime.isValid())
      ircuser->setLoginTime(loginTime);
  }
}

/* RPL_LIST -  "<channel> <# visible> :<topic>" */
void CoreSessionEventProcessor::processIrcEvent322(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  QString channelName;
  quint32 userCount = 0;
  QString topic;

  switch(e->params().count()) {
  case 3:
    topic = e->params()[2];
  case 2:
    userCount = e->params()[1].toUInt();
  case 1:
    channelName = e->params()[0];
  default:
    break;
  }
  if(coreSession()->ircListHelper()->addChannel(e->networkId(), channelName, userCount, topic))
    e->stop(); // consumed by IrcListHelper, so don't further process/show this event
}

/* RPL_LISTEND ":End of LIST" */
void CoreSessionEventProcessor::processIrcEvent323(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  if(coreSession()->ircListHelper()->endOfChannelList(e->networkId()))
    e->stop(); // consumed by IrcListHelper, so don't further process/show this event
}

/* RPL_CHANNELMODEIS - "<channel> <mode> <mode params>" */
void CoreSessionEventProcessor::processIrcEvent324(IrcEvent *e) {
  processIrcEventMode(e);
}

/* RPL_NOTOPIC */
void CoreSessionEventProcessor::processIrcEvent331(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

  IrcChannel *chan = e->network()->ircChannel(e->params()[0]);
  if(chan)
    chan->setTopic(QString());
}

/* RPL_TOPIC */
void CoreSessionEventProcessor::processIrcEvent332(IrcEvent *e) {
  if(!checkParamCount(e, 2))
    return;

  IrcChannel *chan = e->network()->ircChannel(e->params()[0]);
  if(chan)
    chan->setTopic(e->params()[1]);
}

/*  RPL_WHOREPLY: "<channel> <user> <host> <server> <nick>
              ( "H" / "G" > ["*"] [ ( "@" / "+" ) ] :<hopcount> <real name>" */
void CoreSessionEventProcessor::processIrcEvent352(IrcEvent *e) {
  if(!checkParamCount(e, 6))
    return;

  QString channel = e->params()[0];
  IrcUser *ircuser = e->network()->ircUser(e->params()[4]);
  if(ircuser) {
    ircuser->setUser(e->params()[1]);
    ircuser->setHost(e->params()[2]);

    bool away = e->params()[5].startsWith("G");
    ircuser->setAway(away);
    ircuser->setServer(e->params()[3]);
    ircuser->setRealName(e->params().last().section(" ", 1));
  }

  if(coreNetwork(e)->isAutoWhoInProgress(channel))
    e->setFlag(EventManager::Silent);
}

/* RPL_NAMREPLY */
void CoreSessionEventProcessor::processIrcEvent353(IrcEvent *e) {
  if(!checkParamCount(e, 3))
    return;

  // param[0] is either "=", "*" or "@" indicating a public, private or secret channel
  // we don't use this information at the time beeing
  QString channelname = e->params()[1];

  IrcChannel *channel = e->network()->ircChannel(channelname);
  if(!channel) {
    qWarning() << Q_FUNC_INFO << "Received unknown target channel:" << channelname;
    return;
  }

  QStringList nicks;
  QStringList modes;

  foreach(QString nick, e->params()[2].split(' ')) {
    QString mode;

    if(e->network()->prefixes().contains(nick[0])) {
      mode = e->network()->prefixToMode(nick[0]);
      nick = nick.mid(1);
    }

    nicks << nick;
    modes << mode;
  }

  channel->joinIrcUsers(nicks, modes);
}

/* ERR_ERRONEUSNICKNAME */
void CoreSessionEventProcessor::processIrcEvent432(IrcEventNumeric *e) {
  QString errnick;
  if(e->params().count() < 2) {
    // handle unreal-ircd bug, where unreal ircd doesnt supply a TARGET in ERR_ERRONEUSNICKNAME during registration phase:
    // nick @@@
    // :irc.scortum.moep.net 432  @@@ :Erroneous Nickname: Illegal characters
    // correct server reply:
    // :irc.scortum.moep.net 432 * @@@ :Erroneous Nickname: Illegal characters
    e->params().prepend(e->target());
    e->setTarget("*");
  }
  errnick = e->params()[0];

  tryNextNick(e, errnick, true /* erroneus */);
}

/* ERR_NICKNAMEINUSE */
void CoreSessionEventProcessor::processIrcEvent433(IrcEventNumeric *e) {
  if(!checkParamCount(e, 1))
    return;

  QString errnick = e->params().first();

  // if there is a problem while connecting to the server -> we handle it
  // but only if our connection has not been finished yet...
  if(!e->network()->currentServer().isEmpty())
    return;

  tryNextNick(e, errnick);
}

/* ERR_UNAVAILRESOURCE */
void CoreSessionEventProcessor::processIrcEvent437(IrcEventNumeric *e) {
  if(!checkParamCount(e, 1))
    return;

  QString errnick = e->params().first();

  // if there is a problem while connecting to the server -> we handle it
  // but only if our connection has not been finished yet...
  if(!e->network()->currentServer().isEmpty())
    return;

  if(!e->network()->isChannelName(errnick))
    tryNextNick(e, errnick);
}

/* template
void CoreSessionEventProcessor::processIrcEvent(IrcEvent *e) {
  if(!checkParamCount(e, 1))
    return;

}
*/

/* Handle signals from Netsplit objects  */

void CoreSessionEventProcessor::handleNetsplitJoin(Network *net,
                                                   const QString &channel,
                                                   const QStringList &users,
                                                   const QStringList &modes,
                                                   const QString& quitMessage)
{
  IrcChannel *ircChannel = net->ircChannel(channel);
  if(!ircChannel) {
    return;
  }
  QList<IrcUser *> ircUsers;
  QStringList newModes = modes;
  QStringList newUsers = users;

  foreach(const QString &user, users) {
    IrcUser *iu = net->ircUser(nickFromMask(user));
    if(iu)
      ircUsers.append(iu);
    else { // the user already quit
      int idx = users.indexOf(user);
      newUsers.removeAt(idx);
      newModes.removeAt(idx);
    }
  }

  ircChannel->joinIrcUsers(ircUsers, newModes);
  NetworkSplitEvent *event = new NetworkSplitEvent(EventManager::NetworkSplitJoin, net, channel, newUsers, quitMessage);
  emit newEvent(event);
}

void CoreSessionEventProcessor::handleNetsplitQuit(Network *net, const QString &channel, const QStringList &users, const QString& quitMessage) {
  NetworkSplitEvent *event = new NetworkSplitEvent(EventManager::NetworkSplitQuit, net, channel, users, quitMessage);
  emit newEvent(event);
  foreach(QString user, users) {
    IrcUser *iu = net->ircUser(nickFromMask(user));
    if(iu)
      iu->quit();
  }
}

void CoreSessionEventProcessor::handleEarlyNetsplitJoin(Network *net, const QString &channel, const QStringList &users, const QStringList &modes) {
  IrcChannel *ircChannel = net->ircChannel(channel);
  if(!ircChannel) {
    qDebug() << "handleEarlyNetsplitJoin(): channel " << channel << " invalid";
    return;
  }
  QList<NetworkEvent *> events;
  QList<IrcUser *> ircUsers;
  QStringList newModes = modes;

  foreach(QString user, users) {
    IrcUser *iu = net->updateNickFromMask(user);
    if(iu) {
      ircUsers.append(iu);
      // fake event for scripts that consume join events
      events << new IrcEvent(EventManager::IrcEventJoin, net, iu->hostmask(), QStringList() << channel);
    }
    else {
      newModes.removeAt(users.indexOf(user));
    }
  }
  ircChannel->joinIrcUsers(ircUsers, newModes);
  foreach(NetworkEvent *event, events) {
    event->setFlag(EventManager::Fake); // ignore this in here!
    emit newEvent(event);
  }
}

void CoreSessionEventProcessor::handleNetsplitFinished() {
  Netsplit* n = qobject_cast<Netsplit*>(sender());
  Q_ASSERT(n);
  QHash<QString, Netsplit *> splithash  = _netsplits.take(n->network());
  splithash.remove(splithash.key(n));
  if(splithash.count())
    _netsplits[n->network()] = splithash;
  n->deleteLater();
}

void CoreSessionEventProcessor::destroyNetsplits(NetworkId netId) {
  Network *net = coreSession()->network(netId);
  if(!net)
    return;

  QHash<QString, Netsplit *> splits = _netsplits.take(net);
  qDeleteAll(splits);
}

/*******************************/
/******** CTCP HANDLING ********/
/*******************************/

void CoreSessionEventProcessor::processCtcpEvent(CtcpEvent *e) {
  if(e->testFlag(EventManager::Self))
    return; // ignore ctcp events generated by user input

  if(e->type() != EventManager::CtcpEvent || e->ctcpType() != CtcpEvent::Query)
    return;

  handle(e->ctcpCmd(), Q_ARG(CtcpEvent *, e));
}

void CoreSessionEventProcessor::defaultHandler(const QString &ctcpCmd, CtcpEvent *e) {
  // This handler is only there to avoid warnings for unknown CTCPs
  Q_UNUSED(e);
  Q_UNUSED(ctcpCmd);
}

void CoreSessionEventProcessor::handleCtcpAction(CtcpEvent *e) {
  // This handler is only there to feed CLIENTINFO
  Q_UNUSED(e);
}

void CoreSessionEventProcessor::handleCtcpClientinfo(CtcpEvent *e) {
  QStringList supportedHandlers;
  foreach(QString handler, providesHandlers())
    supportedHandlers << handler.toUpper();
  qSort(supportedHandlers);
  e->setReply(supportedHandlers.join(" "));
}

void CoreSessionEventProcessor::handleCtcpPing(CtcpEvent *e) {
  e->setReply(e->param());
}

void CoreSessionEventProcessor::handleCtcpTime(CtcpEvent *e) {
  e->setReply(QDateTime::currentDateTime().toString());
}

void CoreSessionEventProcessor::handleCtcpVersion(CtcpEvent *e) {
  e->setReply(QString("Quassel IRC %1 (built on %2) -- http://www.quassel-irc.org")
              .arg(Quassel::buildInfo().plainVersionString).arg(Quassel::buildInfo().buildDate));
}
