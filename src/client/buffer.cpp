/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "client.h"
#include "buffer.h"
#include "util.h"

/*
Buffer::Buffer(QString netname, QString bufname) {
  Buffer(BufferId(0, netname, bufname));


}
*/

Buffer::Buffer(BufferId bufid) {
  id = bufid;
  _networkName = bufid.network();
  _bufferName = bufid.buffer();

  if(_bufferName.isEmpty()) type = ServerBuffer;
  else if(isChannelName(_bufferName)) type = ChannelBuffer;
  else type = QueryBuffer;

  active = false;
/*
  QSettings s;
  s.beginGroup(QString("GUI/BufferStates/%1/%2").arg(netname).arg(bufname));
  state->splitterState = s.value("Splitter").toByteArray();
  s.endGroup();
  */
  emit bufferUpdated(this);
}

Buffer::~Buffer() {
  //delete widget;
  /*
  QSettings s;
  s.beginGroup(QString("GUI/BufferStates/%1/%2").arg(networkName).arg(bufferName));
  s.setValue("Splitter", state->splitterState);
  s.endGroup();
*/
  //delete state;
  emit bufferDestroyed(this);
}

Buffer::Type Buffer::bufferType() const {
   return type;
}

bool Buffer::isActive() const {
   return active;
}

QString Buffer::networkName() const {
   return _networkName;
}

QString Buffer::bufferName() const {
   return _bufferName;
}

QString Buffer::displayName() const {
  if(bufferType() == ServerBuffer)
    return tr("Status Buffer");
  else
    return bufferName();
}

BufferId Buffer::bufferId() const {
   return id;
}

QList<AbstractUiMsg *> Buffer::contents() const {
   return layoutedMsgs;
}

VarMap Buffer::nickList() const {
   return nicks;
}

QString Buffer::topic() const {
   return _topic;
}

QString Buffer::ownNick() const {
   return _ownNick;
}

bool Buffer::isStatusBuffer() const {
   return bufferType() == ServerBuffer;
}

void Buffer::setActive(bool a) {
  if(a != active) {
    active = a;
    emit bufferUpdated(this);
  }
}

void Buffer::appendMsg(const Message &msg) {
  AbstractUiMsg *m = Client::layoutMsg(msg);
  layoutedMsgs.append(m);
  emit msgAppended(m);
}

void Buffer::prependMsg(const Message &msg) {
  layoutQueue.append(msg);
}

bool Buffer::layoutMsg() {
  if(layoutQueue.count()) {
    AbstractUiMsg *m = Client::layoutMsg(layoutQueue.takeFirst());
    layoutedMsgs.prepend(m);
    emit msgPrepended(m);
  }
  return layoutQueue.count();
}

void Buffer::processUserInput(QString msg) {
  // TODO User Input processing (plugins) -> well, this goes through MainWin into Core for processing... so...
  emit userInput(id, msg);
}

void Buffer::setTopic(QString t) {
  _topic = t;
  emit topicSet(t);
  emit bufferUpdated(this);
}

void Buffer::addNick(QString nick, VarMap props) {
  if(nick == ownNick()) setActive(true);
  nicks[nick] = props;
  emit nickListChanged(nicks);
}

void Buffer::updateNick(QString nick, VarMap props) {
  nicks[nick] = props;
  emit nickListChanged(nicks);
}

void Buffer::renameNick(QString oldnick, QString newnick) {
  QVariant v = nicks.take(oldnick);
  nicks[newnick] = v;
  emit nickListChanged(nicks);
}

void Buffer::removeNick(QString nick) {
  if(nick == ownNick()) setActive(false);
  nicks.remove(nick);
  emit nickListChanged(nicks);
}

void Buffer::setOwnNick(QString nick) {
  _ownNick = nick;
  emit ownNickSet(nick);
}

/****************************************************************************************/


/****************************************************************************************/

