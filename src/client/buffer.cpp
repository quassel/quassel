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

void Buffer::setVisible(bool visible) {
  _isVisible = visible;
  setActivityLevel(NoActivity);
  if(_lastRcvdMsg.msgId() > 0) setLastSeenMsg(_lastRcvdMsg.msgId());
}

void Buffer::setLastSeenMsg(const MsgId &msgId) {
  const MsgId oldLastSeen = lastSeenMsg();
  if(!oldLastSeen.isValid() || (msgId.isValid() && msgId > oldLastSeen)) {
    _lastSeenMsg = msgId;
    Client::setBufferLastSeenMsg(bufferInfo().bufferId(), msgId);
    setActivityLevel(NoActivity);
  }
}

void Buffer::setActivityLevel(ActivityLevel level) {
  _activityLevel = level;
  if(bufferInfo().bufferId() > 0) {
    Client::networkModel()->setBufferActivity(bufferInfo(), level);
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
