/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "clientbacklogmanager.h"

#include "abstractmessageprocessor.h"
#include "backlogrequester.h"
#include "client.h"

#include <QDebug>

ClientBacklogManager::ClientBacklogManager(QObject *parent)
  : BacklogManager(parent),
    _buffer(true)
{
}

void ClientBacklogManager::receiveBacklog(BufferId bufferId, int lastMsgs, int offset, QVariantList msgs) {
  Q_UNUSED(lastMsgs)
  Q_UNUSED(offset)

  if(msgs.isEmpty())
    return;

  //QTime start = QTime::currentTime();
  QList<Message> msglist;
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    msg.setFlags(msg.flags() | Message::Backlog);
    msglist << msg;
  }

  if(_buffer) {
    _messageBuffer << msglist;
    _buffersWaiting.remove(bufferId);
    if(_buffersWaiting.isEmpty()) {
      _buffer = false;
      qSort(_messageBuffer);
      Client::messageProcessor()->process(_messageBuffer);
      _messageBuffer.clear();
    }
  } else {
    Client::messageProcessor()->process(msglist);
  }
  //qDebug() << "processed" << msgs.count() << "backlog lines in" << start.msecsTo(QTime::currentTime());
}

QVariantList ClientBacklogManager::requestBacklog(BufferId bufferId, int lastMsgs, int offset) {
  if(_buffer)
    _buffersWaiting << bufferId;

  return BacklogManager::requestBacklog(bufferId, lastMsgs, offset);
}

void ClientBacklogManager::requestInitialBacklog() {
  FixedBacklogRequester backlogRequester(this);
  backlogRequester.requestBacklog();
}
