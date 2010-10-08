/***************************************************************************
 *   Copyright (C) 2005-10 by the Quassel Project                          *
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
#include "ircserverhandler.h"

#include "util.h"

#include "coresession.h"
#include "coreirclisthelper.h"
#include "coreidentity.h"
#include "ctcphandler.h"

#include "ircuser.h"
#include "coreircchannel.h"
#include "logger.h"

#include <QDebug>

#ifdef HAVE_QCA2
#  include "cipher.h"
#endif

IrcServerHandler::IrcServerHandler(CoreNetwork *parent)
  : CoreBasicHandler(parent),
    _whois(false)
{
  connect(parent, SIGNAL(disconnected(NetworkId)), this, SLOT(destroyNetsplits()));
}

IrcServerHandler::~IrcServerHandler() {
  destroyNetsplits();
}

/*! Handle a raw message string sent by the server. We try to find a suitable handler, otherwise we call a default handler. */
void IrcServerHandler::handleServerMsg(QByteArray msg) {
  if(msg.isEmpty()) {
    qWarning() << "Received empty string from server!";
    return;
  }

  // Now we split the raw message into its various parts...
  QString prefix = "";
  QByteArray trailing;
  QString cmd;

  // First, check for a trailing parameter introduced by " :", since this might screw up splitting the msg
  // NOTE: This assumes that this is true in raw encoding, but well, hopefully there are no servers running in japanese on protocol level...
  int idx = msg.indexOf(" :");
  if(idx >= 0) {
    if(msg.length() > idx + 2)
      trailing = msg.mid(idx + 2);
    msg = msg.left(idx);
  }
  // OK, now it is safe to split...
  QList<QByteArray> params = msg.split(' ');

  // This could still contain empty elements due to (faulty?) ircds sending multiple spaces in a row
  // Also, QByteArray is not nearly as convenient to work with as QString for such things :)
  QList<QByteArray>::iterator iter = params.begin();
  while(iter != params.end()) {
    if(iter->isEmpty())
      iter = params.erase(iter);
    else
      ++iter;
  }

  if(!trailing.isEmpty()) params << trailing;
  if(params.count() < 1) {
    qWarning() << "Received invalid string from server!";
    return;
  }

  QString foo = serverDecode(params.takeFirst());

  // with SASL, the command is 'AUTHENTICATE +' and we should check for this here.
  /* obsolete because of events
  if(foo == QString("AUTHENTICATE +")) {
    handleAuthenticate();
    return;
  }
  */
  // a colon as the first chars indicates the existence of a prefix
  if(foo[0] == ':') {
    foo.remove(0, 1);
    prefix = foo;
    if(params.count() < 1) {
      qWarning() << "Received invalid string from server!";
      return;
    }
    foo = serverDecode(params.takeFirst());
  }

  // next string without a whitespace is the command
  cmd = foo.trimmed().toUpper();

  // numeric replies have the target as first param (RFC 2812 - 2.4). this is usually our own nick. Remove this!
  uint num = cmd.toUInt();
  if(num > 0) {
    if(params.count() == 0) {
      qWarning() << "Message received from server violates RFC and is ignored!" << msg;
      return;
    }
    _target = serverDecode(params.takeFirst());
  } else {
    _target = QString();
  }

  // note that the IRC server is still alive
  network()->resetPingTimeout();

  // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
  handle(cmd, Q_ARG(QString, prefix), Q_ARG(QList<QByteArray>, params));
}


void IrcServerHandler::defaultHandler(QString cmd, const QString &prefix, const QList<QByteArray> &rawparams) {
  // many commands are handled by the event system now
  Q_UNUSED(cmd)
  Q_UNUSED(prefix)
  Q_UNUSED(rawparams)
}

//******************************/
// IRC SERVER HANDLER
//******************************/

void IrcServerHandler::handleJoin(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleJoin()", params, 1))
    return;

  QString channel = serverDecode(params[0]);
  IrcUser *ircuser = network()->updateNickFromMask(prefix);

  bool handledByNetsplit = false;
  if(!_netsplits.empty()) {
    foreach(Netsplit* n, _netsplits) {
      handledByNetsplit = n->userJoined(prefix, channel);
      if(handledByNetsplit)
        break;
    }
  }

  // normal join
  if(!handledByNetsplit) {
    emit displayMsg(Message::Join, BufferInfo::ChannelBuffer, channel, channel, prefix);
    ircuser->joinChannel(channel);
  }
  //qDebug() << "IrcServerHandler::handleJoin()" << prefix << params;

  if(network()->isMe(ircuser)) {
    network()->setChannelJoined(channel);
    putCmd("MODE", params[0]); // we want to know the modes of the channel we just joined, so we ask politely
  }
}

void IrcServerHandler::handleMode(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleMode()", params, 2))
    return;

  if(network()->isChannelName(serverDecode(params[0]))) {
    // Channel Modes
    emit displayMsg(Message::Mode, BufferInfo::ChannelBuffer, serverDecode(params[0]), serverDecode(params).join(" "), prefix);

    IrcChannel *channel = network()->ircChannel(params[0]);
    if(!channel) {
      // we received mode information for a channel we're not in. that means probably we've just been kicked out or something like that
      // anyways: we don't have a place to store the data --> discard the info.
      return;
    }

    QString modes = params[1];
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

      if(network()->prefixModes().contains(modes[c])) {
        // user channel modes (op, voice, etc...)
        if(paramOffset < params.count()) {
          IrcUser *ircUser = network()->ircUser(params[paramOffset]);
          if(!ircUser) {
            qWarning() << Q_FUNC_INFO << "Unknown IrcUser:" << params[paramOffset];
          } else {
            if(add) {
              bool handledByNetsplit = false;
              if(!_netsplits.empty()) {
                foreach(Netsplit* n, _netsplits) {
                  handledByNetsplit = n->userAlreadyJoined(ircUser->hostmask(), channel->name());
                  if(handledByNetsplit) {
                    n->addMode(ircUser->hostmask(), channel->name(), QString(modes[c]));
                    break;
                  }
                }
              }
              if(!handledByNetsplit)
                channel->addUserMode(ircUser, QString(modes[c]));
            }
            else
              channel->removeUserMode(ircUser, QString(modes[c]));
          }
        } else {
          qWarning() << "Received MODE with too few parameters:" << serverDecode(params);
        }
        paramOffset++;
      } else {
        // regular channel modes
        QString value;
        Network::ChannelModeType modeType = network()->channelModeType(modes[c]);
        if(modeType == Network::A_CHANMODE || modeType == Network::B_CHANMODE || (modeType == Network::C_CHANMODE && add)) {
          if(paramOffset < params.count()) {
            value = params[paramOffset];
          } else {
            qWarning() << "Received MODE with too few parameters:" << serverDecode(params);
          }
          paramOffset++;
        }

        if(add)
          channel->addChannelMode(modes[c], value);
        else
          channel->removeChannelMode(modes[c], value);
      }
    }

  } else {
    // pure User Modes
    IrcUser *ircUser = network()->newIrcUser(params[0]);
    QString modeString(serverDecode(params[1]));
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

    if(network()->isMe(ircUser)) {
      network()->updatePersistentModes(addModes, removeModes);
    }

    // FIXME: redirect
    emit displayMsg(Message::Mode, BufferInfo::StatusBuffer, "", serverDecode(params).join(" "), prefix);
  }
}

void IrcServerHandler::handleNotice(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleNotice()", params, 2))
    return;


  QStringList targets = serverDecode(params[0]).split(',', QString::SkipEmptyParts);
  QStringList::const_iterator targetIter;
  for(targetIter = targets.constBegin(); targetIter != targets.constEnd(); targetIter++) {
    QString target = *targetIter;

    // special treatment for welcome messages like:
    // :ChanServ!ChanServ@services. NOTICE egst :[#apache] Welcome, this is #apache. Please read the in-channel topic message. This channel is being logged by IRSeekBot. If you have any question please see http://blog.freenode.net/?p=68
    if(!network()->isChannelName(target)) {
      QString msg = serverDecode(params[1]);
      QRegExp welcomeRegExp("^\\[([^\\]]+)\\] ");
      if(welcomeRegExp.indexIn(msg) != -1) {
        QString channelname = welcomeRegExp.cap(1);
        msg = msg.mid(welcomeRegExp.matchedLength());
        CoreIrcChannel *chan = static_cast<CoreIrcChannel *>(network()->ircChannel(channelname)); // we only have CoreIrcChannels in the core, so this cast is safe
        if(chan && !chan->receivedWelcomeMsg()) {
          chan->setReceivedWelcomeMsg();
          emit displayMsg(Message::Notice, BufferInfo::ChannelBuffer, channelname, msg, prefix);
          continue;
        }
      }
    }

    if(prefix.isEmpty() || target == "AUTH") {
      target = "";
    } else {
      if(!target.isEmpty() && network()->prefixes().contains(target[0]))
        target = target.mid(1);
      if(!network()->isChannelName(target))
        target = nickFromMask(prefix);
    }

    network()->ctcpHandler()->parse(Message::Notice, prefix, target, params[1]);
  }

}

void IrcServerHandler::handlePing(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  putCmd("PONG", params);
}

void IrcServerHandler::handlePrivmsg(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handlePrivmsg()", params, 1))
    return;

  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser) {
    qWarning() << "IrcServerHandler::handlePrivmsg(): Unknown IrcUser!";
    return;
  }

  if(params.isEmpty()) {
    qWarning() << "IrcServerHandler::handlePrivmsg(): received PRIVMSG without target or message from:" << prefix;
    return;
  }

  QString senderNick = nickFromMask(prefix);

  QByteArray msg = params.count() < 2
    ? QByteArray("")
    : params[1];

  QStringList targets = serverDecode(params[0]).split(',', QString::SkipEmptyParts);
  QStringList::const_iterator targetIter;
  for(targetIter = targets.constBegin(); targetIter != targets.constEnd(); targetIter++) {
    const QString &target = network()->isChannelName(*targetIter)
      ? *targetIter
      : senderNick;

#ifdef HAVE_QCA2
    msg = decrypt(target, msg);
#endif
    // it's possible to pack multiple privmsgs into one param using ctcp
    // - > we let the ctcpHandler do the work
    network()->ctcpHandler()->parse(Message::Plain, prefix, target, msg);
  }
}

void IrcServerHandler::handleQuit(const QString &prefix, const QList<QByteArray> &params) {
  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser) return;

  QString msg;
  if(params.count() > 0)
    msg = userDecode(ircuser->nick(), params[0]);

  // check if netsplit
  if(Netsplit::isNetsplit(msg)) {
    Netsplit *n;
    if(!_netsplits.contains(msg)) {
      n = new Netsplit();
      connect(n, SIGNAL(finished()), this, SLOT(handleNetsplitFinished()));
      connect(n, SIGNAL(netsplitJoin(const QString&, const QStringList&, const QStringList&, const QString&)),
              this, SLOT(handleNetsplitJoin(const QString&, const QStringList&, const QStringList&, const QString&)));
      connect(n, SIGNAL(netsplitQuit(const QString&, const QStringList&, const QString&)),
              this, SLOT(handleNetsplitQuit(const QString&, const QStringList&, const QString&)));
      connect(n, SIGNAL(earlyJoin(const QString&, const QStringList&, const QStringList&)),
              this, SLOT(handleEarlyNetsplitJoin(const QString&, const QStringList&, const QStringList&)));
      _netsplits.insert(msg, n);
    }
    else {
      n = _netsplits[msg];
    }
    // add this user to the netsplit
    n->userQuit(prefix, ircuser->channels(),msg);
  }
  // normal quit
  else {
    foreach(QString channel, ircuser->channels())
      emit displayMsg(Message::Quit, BufferInfo::ChannelBuffer, channel, msg, prefix);
    ircuser->quit();
  }
}

/* RPL_CHANNELMODEIS - "<channel> <mode> <mode params>" */
void IrcServerHandler::handle324(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  handleMode(prefix, params);
}

/* RPL_INVITING - "<nick> <channel>*/
void IrcServerHandler::handle341(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle341()", params, 2))
    return;

  QString nick = serverDecode(params[0]);

  IrcChannel *channel = network()->ircChannel(serverDecode(params[1]));
  if(!channel) {
    qWarning() << "IrcServerHandler::handle341(): unknown channel:" << params[1];
    return;
  }

  emit displayMsg(Message::Server, BufferInfo::ChannelBuffer, channel->name(), tr("%1 has been invited to %2").arg(nick).arg(channel->name()));
}

/*  RPL_WHOREPLY: "<channel> <user> <host> <server> <nick>
              ( "H" / "G" > ["*"] [ ( "@" / "+" ) ] :<hopcount> <real name>" */
void IrcServerHandler::handle352(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle352()", params, 6))
    return;

  QString channel = serverDecode(params[0]);
  IrcUser *ircuser = network()->ircUser(serverDecode(params[4]));
  if(ircuser) {
    ircuser->setUser(serverDecode(params[1]));
    ircuser->setHost(serverDecode(params[2]));

    bool away = serverDecode(params[5]).startsWith("G") ? true : false;
    ircuser->setAway(away);
    ircuser->setServer(serverDecode(params[3]));
    ircuser->setRealName(serverDecode(params.last()).section(" ", 1));
  }

  if(!network()->isAutoWhoInProgress(channel)) {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Who] %1").arg(serverDecode(params).join(" ")));
  }
}

/* RPL_NAMREPLY */
void IrcServerHandler::handle353(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle353()", params, 3))
    return;

  // param[0] is either "=", "*" or "@" indicating a public, private or secret channel
  // we don't use this information at the time beeing
  QString channelname = serverDecode(params[1]);

  IrcChannel *channel = network()->ircChannel(channelname);
  if(!channel) {
    qWarning() << "IrcServerHandler::handle353(): received unknown target channel:" << channelname;
    return;
  }

  QStringList nicks;
  QStringList modes;

  foreach(QString nick, serverDecode(params[2]).split(' ')) {
    QString mode = QString();

    if(network()->prefixes().contains(nick[0])) {
      mode = network()->prefixToMode(nick[0]);
      nick = nick.mid(1);
    }

    nicks << nick;
    modes << mode;
  }

  channel->joinIrcUsers(nicks, modes);
}

/*  RPL_ENDOFWHOWAS - "<nick> :End of WHOWAS" */
void IrcServerHandler::handle369(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whowas] %1").arg(serverDecode(params).join(" ")));
}

/* ERR_ERRONEUSNICKNAME */
void IrcServerHandler::handle432(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);

  QString errnick;
  if(params.size() < 2) {
    // handle unreal-ircd bug, where unreal ircd doesnt supply a TARGET in ERR_ERRONEUSNICKNAME during registration phase:
    // nick @@@
    // :irc.scortum.moep.net 432  @@@ :Erroneous Nickname: Illegal characters
    // correct server reply:
    // :irc.scortum.moep.net 432 * @@@ :Erroneous Nickname: Illegal characters
    errnick = target();
  } else {
    errnick = params[0];
  }
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Nick %1 contains illegal characters").arg(errnick));
  tryNextNick(errnick, true /* erroneus */);
}

/* ERR_NICKNAMEINUSE */
void IrcServerHandler::handle433(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle433()", params, 1))
    return;

  QString errnick = serverDecode(params[0]);
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Nick already in use: %1").arg(errnick));

  // if there is a problem while connecting to the server -> we handle it
  // but only if our connection has not been finished yet...
  if(!network()->currentServer().isEmpty())
    return;

  tryNextNick(errnick);
}

/* ERR_UNAVAILRESOURCE */
void IrcServerHandler::handle437(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle437()", params, 1))
    return;

  QString errnick = serverDecode(params[0]);
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Nick/channel is temporarily unavailable: %1").arg(errnick));

  // if there is a problem while connecting to the server -> we handle it
  // but only if our connection has not been finished yet...
  if(!network()->currentServer().isEmpty())
    return;

  if(!network()->isChannelName(errnick))
    tryNextNick(errnick);
}

/* Handle signals from Netsplit objects  */

void IrcServerHandler::handleNetsplitJoin(const QString &channel, const QStringList &users, const QStringList &modes, const QString& quitMessage)
{
  IrcChannel *ircChannel = network()->ircChannel(channel);
  if(!ircChannel) {
    return;
  }
  QList<IrcUser *> ircUsers;
  QStringList newModes = modes;
  QStringList newUsers = users;

  foreach(QString user, users) {
    IrcUser *iu = network()->ircUser(nickFromMask(user));
    if(iu)
      ircUsers.append(iu);
    else { // the user already quit
      int idx = users.indexOf(user);
      newUsers.removeAt(idx);
      newModes.removeAt(idx);
    }
  }

  QString msg = newUsers.join("#:#").append("#:#").append(quitMessage);
  emit displayMsg(Message::NetsplitJoin, BufferInfo::ChannelBuffer, channel, msg);
  ircChannel->joinIrcUsers(ircUsers, newModes);
}

void IrcServerHandler::handleNetsplitQuit(const QString &channel, const QStringList &users, const QString& quitMessage)
{
  QString msg = users.join("#:#").append("#:#").append(quitMessage);
  emit displayMsg(Message::NetsplitQuit, BufferInfo::ChannelBuffer, channel, msg);
  foreach(QString user, users) {
    IrcUser *iu = network()->ircUser(nickFromMask(user));
    if(iu)
      iu->quit();
  }
}

void IrcServerHandler::handleEarlyNetsplitJoin(const QString &channel, const QStringList &users, const QStringList &modes) {
  IrcChannel *ircChannel = network()->ircChannel(channel);
  if(!ircChannel) {
    qDebug() << "handleEarlyNetsplitJoin(): channel " << channel << " invalid";
    return;
  }
  QList<IrcUser *> ircUsers;
  QStringList newModes = modes;

  foreach(QString user, users) {
    IrcUser *iu = network()->updateNickFromMask(user);
    if(iu) {
      ircUsers.append(iu);
      emit displayMsg(Message::Join, BufferInfo::ChannelBuffer, channel, channel, user);
    }
    else {
      newModes.removeAt(users.indexOf(user));
    }
  }
  ircChannel->joinIrcUsers(ircUsers, newModes);
}
void IrcServerHandler::handleNetsplitFinished()
{
  Netsplit* n = qobject_cast<Netsplit*>(sender());
  _netsplits.remove(_netsplits.key(n));
  n->deleteLater();
}

/* */

// FIXME networkConnection()->setChannelKey("") for all ERR replies indicating that a JOIN went wrong
//       mostly, these are codes in the 47x range

/* */

void IrcServerHandler::tryNextNick(const QString &errnick, bool erroneus) {
  QStringList desiredNicks = coreSession()->identity(network()->identity())->nicks();
  int nextNickIdx = desiredNicks.indexOf(errnick) + 1;
  QString nextNick;
  if(nextNickIdx > 0 && desiredNicks.size() > nextNickIdx) {
    nextNick = desiredNicks[nextNickIdx];
  } else {
    if(erroneus) {
      emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("No free and valid nicks in nicklist found. use: /nick <othernick> to continue"));
      return;
    } else {
      nextNick = errnick + "_";
    }
  }
  putCmd("NICK", serverEncode(nextNick));
}

bool IrcServerHandler::checkParamCount(const QString &methodName, const QList<QByteArray> &params, int minParams) {
  if(params.count() < minParams) {
    qWarning() << qPrintable(methodName) << "requires" << minParams << "parameters but received only" << params.count() << serverDecode(params);
    return false;
  } else {
    return true;
  }
}

void IrcServerHandler::destroyNetsplits() {
  qDeleteAll(_netsplits);
  _netsplits.clear();
}

#ifdef HAVE_QCA2
QByteArray IrcServerHandler::decrypt(const QString &bufferName, const QByteArray &message_, bool isTopic) {
  if(message_.isEmpty())
    return message_;

  Cipher *cipher = network()->cipher(bufferName);
  if(!cipher)
    return message_;

  QByteArray message = message_;
  message = isTopic? cipher->decryptTopic(message) : cipher->decrypt(message);
  return message;
}
#endif
