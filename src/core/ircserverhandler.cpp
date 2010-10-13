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

}

IrcServerHandler::~IrcServerHandler() {

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
