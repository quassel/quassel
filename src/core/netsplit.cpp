/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "netsplit.h"

#include <QRegExp>

Netsplit::Netsplit()
    : _quitMsg(""), _sentQuit(false)
{
  _discardTimer.setSingleShot(true);
  _joinTimer.setSingleShot(true);
  _quitTimer.setSingleShot(true);

  connect(&_discardTimer, SIGNAL(timeout()), this, SIGNAL(finished()));

  connect(&_joinTimer, SIGNAL(timeout()), this, SLOT(joinTimeout()));
  connect(&_quitTimer, SIGNAL(timeout()), this, SLOT(quitTimeout()));

  // wait for a maximum of 1 hour until we discard the netsplit
  _discardTimer.start(3600000);
}

void Netsplit::userQuit(const QString &sender, const QStringList &channels, const QString &msg)
{
  if(_quitMsg.isEmpty())
    _quitMsg = msg;
  foreach(QString channel, channels) {
    _quits[channel].append(sender);
  }
  // now let's wait 5s to finish the netsplit-quit
  _quitTimer.start(5000);
}

bool Netsplit::userJoined(const QString &sender, const QString &channel) {
  if(!_quits.contains(channel))
    return false;

  QStringList &users = _quits[channel];
  int idx = users.indexOf(sender);
  if(idx == -1)
    return false;

  _joins[channel].append(users.takeAt(idx));
  if(users.empty())
    _quits.remove(channel);

  // now let's wait 5s to finish the netsplit-join
  _joinTimer.start(5000);
  return true;
}

bool Netsplit::isNetsplit(const QString &quitMessage)
{
  // check if we find some common chars that disqualify the netsplit as such
  if(quitMessage.contains(':') || quitMessage.contains('/'))
    return false;

  // now test if message consists only of two dns names as the RFC requests
  // but also allow the commonly used "*.net *.split"
  QRegExp hostRx("^(?:[\\w\\d-.]+|\\*)\\.[\\w\\d-]+\\s(?:[\\w\\d-.]+|\\*)\\.[\\w\\d-]+$");
  if(hostRx.exactMatch(quitMessage))
    return true;

  return false;
}

void Netsplit::joinTimeout()
{
  if(!_sentQuit) {
    _quitTimer.stop();
    quitTimeout();
  }
  QHash<QString, QStringList>::iterator it;
  for(it = _joins.begin(); it != _joins.end(); ++it)
    emit netsplitJoin(it.key(), it.value(),_quitMsg);
  _joins.clear();
  _discardTimer.stop();
  emit finished();
}

void Netsplit::quitTimeout()
{
  QHash<QString, QStringList>::iterator it;
  for(it = _quits.begin(); it != _quits.end(); ++it)
    emit netsplitQuit(it.key(), it.value(),_quitMsg);
  _sentQuit = true;
}
