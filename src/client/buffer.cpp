/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
#include <QDebug>

#include "buffer.h"

#include "buffersyncer.h"
#include "client.h"
#include "networkmodel.h"
#include "quasselui.h"
#include "util.h"


Buffer::Buffer(BufferInfo bufferid, QObject *parent)
  : QObject(parent),
    _bufferInfo(bufferid),
    _isVisible(false),
    _activityLevel(NoActivity)
{

}

BufferInfo Buffer::bufferInfo() const {
  // still needed by the gui *sigh* to request the backlogs *sigh*
  return _bufferInfo;
}

const QList<AbstractUiMsg *> &Buffer::contents() const {
  return layoutedMsgs;
}

void Buffer::appendMsg(const Message &msg) {
  updateActivityLevel(msg);
  AbstractUiMsg *m = Client::layoutMsg(msg);
  layoutedMsgs.append(m);
  emit msgAppended(m);
}

void Buffer::prependMsg(const Message &msg) {
  // check for duplicate first
  if(!layoutedMsgs.isEmpty()  && msg.msgId() >= layoutedMsgs.first()->msgId()) {
    return;
  }
  updateActivityLevel(msg);
  layoutQueue.append(msg);
}

bool Buffer::layoutMsg() {
  if(layoutQueue.isEmpty())
    return false;

  AbstractUiMsg *m = Client::layoutMsg(layoutQueue.takeFirst());
  layoutedMsgs.prepend(m);
  emit msgPrepended(m);

  return !layoutQueue.isEmpty();
}

void Buffer::setVisible(bool visible) {
  _isVisible = visible;
  setActivityLevel(NoActivity);
  //if(layoutedMsgs.isEmpty())
  //  return;
  //setLastSeenMsg(layoutedMsgs.last()->msgId());
  if(_lastRcvdMsg.msgId() > 0) setLastSeenMsg(_lastRcvdMsg.msgId());
  //qDebug() << "setting last seen" << _lastRcvdMsg.msgId();
}

void Buffer::setLastSeenMsg(const MsgId &msgId) {
  // qDebug() << "want to set lastSeen:" << bufferInfo() << seen << lastSeen();
  const MsgId oldLastSeen = lastSeenMsg();
  if(!oldLastSeen.isValid() || (msgId.isValid() && msgId > oldLastSeen)) {
    //qDebug() << "setting:" << bufferInfo().bufferName() << seen;
    _lastSeenMsg = msgId;
    Client::setBufferLastSeenMsg(bufferInfo().bufferId(), msgId);
    //qDebug() << "setting lastSeen:" << bufferInfo() << lastSeen();
    setActivityLevel(NoActivity);
  }
}

void Buffer::setActivityLevel(ActivityLevel level) {
  _activityLevel = level;
  if(bufferInfo().bufferId() > 0) {
    Client::networkModel()->setBufferActivity(bufferInfo(), level);
    //qDebug() << "setting level:" << bufferInfo() << lastSeen() << level;
  }
}

void Buffer::updateActivityLevel(const Message &msg) {
  // FIXME dirty hack to allow the lastSeen stuff to continue to work
  //       will be made much nicer once Buffer dies, I hope...
  if(msg.msgId() > _lastRcvdMsg.msgId()) _lastRcvdMsg = msg;

  if(isVisible())
    return;

  if(msg.flags() & Message::Self)	// don't update activity for our own messages
    return;

  if(lastSeenMsg().isValid() && lastSeenMsg() >= msg.msgId())
    return;

  ActivityLevel level = activityLevel() | OtherActivity;
  if(msg.type() & (Message::Plain | Message::Notice | Message::Action))
    level |= NewMessage;

  if(msg.flags() & Message::Highlight)
    level |= Highlight;

  if(level != activityLevel())
    setActivityLevel(level);
}
