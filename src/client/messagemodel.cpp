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

#include "messagemodel.h"

#include <QEvent>

#include "backlogsettings.h"
#include "clientbacklogmanager.h"
#include "client.h"
#include "message.h"
#include "networkmodel.h"

class ProcessBufferEvent : public QEvent {
public:
  inline ProcessBufferEvent() : QEvent(QEvent::User) {}
};

MessageModel::MessageModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  QDateTime now = QDateTime::currentDateTime();
  now.setTimeSpec(Qt::UTC);
  _nextDayChange.setTimeSpec(Qt::UTC);
  _nextDayChange.setTime_t(((now.toTime_t() / 86400) + 1) * 86400);
  _nextDayChange.setTimeSpec(Qt::LocalTime);
  _dayChangeTimer.setInterval(QDateTime::currentDateTime().secsTo(_nextDayChange) * 1000);
  _dayChangeTimer.start();
  connect(&_dayChangeTimer, SIGNAL(timeout()), this, SLOT(changeOfDay()));
}

QVariant MessageModel::data(const QModelIndex &index, int role) const {
  int row = index.row(); int column = index.column();
  if(row < 0 || row >= _messageList.count() || column < 0)
    return QVariant();

  if(role == ColumnTypeRole)
    return column;

  return _messageList[row]->data(index.column(), role);
}

bool MessageModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  int row = index.row();
  if(row < 0 || row >= _messageList.count())
    return false;

  if(_messageList[row]->setData(index.column(), value, role)) {
    emit dataChanged(index, index);
    return true;
  }

  return false;
}

bool MessageModel::insertMessage(const Message &msg, bool fakeMsg) {
  MsgId id = msg.msgId();
  int idx = indexForId(id);
  if(!fakeMsg && idx < _messageList.count()) { // check for duplicate
    if(_messageList[idx]->msgId() == id)
      return false;
  }

  insertMessageGroup(QList<Message>() << msg);
  return true;
}

void MessageModel::insertMessages(const QList<Message> &msglist) {
  if(msglist.isEmpty())
    return;

  if(_messageBuffer.isEmpty()) {
    int processedMsgs = insertMessagesGracefully(msglist);
    int remainingMsgs = msglist.count() - processedMsgs;
    if(remainingMsgs > 0) {
      if(msglist.first().msgId() < msglist.last().msgId()) {
	// in Order - we have just successfully processed "processedMsg" messages from the end of the list
	_messageBuffer = msglist.mid(0, remainingMsgs);
      } else {
	_messageBuffer = msglist.mid(processedMsgs);
      }
      qSort(_messageBuffer);
      QCoreApplication::postEvent(this, new ProcessBufferEvent());
    }
  } else {
    _messageBuffer << msglist;
    qSort(_messageBuffer);
  }
}

void MessageModel::insertMessageGroup(const QList<Message> &msglist) {
  Q_ASSERT(!msglist.isEmpty()); // the msglist can be assumed to be non empty
//   int last = msglist.count() - 1;
//   Q_ASSERT(0 == last || msglist.at(0).msgId() != msglist.at(last).msgId() || msglist.at(last).type() == Message::DayChange);
  int start = indexForId(msglist.first().msgId());
  int end = start + msglist.count() - 1;
  MessageModelItem *dayChangeItem = 0;
  bool relocatedMsg = false;
  if(start > 0) {
    // check if the preceeding msg is a daychange message and if so if
    // we have to drop or relocate it at the end of this chunk
    int prevIdx = start - 1;
    if(_messageList.at(prevIdx)->msgType() == Message::DayChange
       && _messageList.at(prevIdx)->timeStamp() > msglist.at(0).timestamp()) {
      beginRemoveRows(QModelIndex(), prevIdx, prevIdx);
      MessageModelItem *oldItem = _messageList.takeAt(prevIdx);
      if(msglist.last().timestamp() < oldItem->timeStamp()) {
	// we have to reinsert it (with changed msgId -> thus we need to recreate it)
	Message dayChangeMsg = Message::ChangeOfDay(oldItem->timeStamp());
	dayChangeMsg.setMsgId(msglist.last().msgId());
	dayChangeItem = createMessageModelItem(dayChangeMsg);
      }
      delete oldItem;
      endRemoveRows();
      start--;
      end--;
      relocatedMsg = true;
    }
  }

  if(!dayChangeItem && start < _messageList.count()) {
    // check if we need to insert a daychange message at the end of the this group

    // if this assert triggers then indexForId() would have found a spot right before a DayChangeMsg
    // this should never happen as daychange messages share the msgId with the preceeding message
    Q_ASSERT(_messageList[start]->msgType() != Message::DayChange);
    QDateTime nextTs = _messageList[start]->timeStamp();
    QDateTime prevTs = msglist.last().timestamp();
    nextTs.setTimeSpec(Qt::UTC);
    prevTs.setTimeSpec(Qt::UTC);
    uint nextDay = nextTs.toTime_t() / 86400;
    uint prevDay = prevTs.toTime_t() / 86400;
    if(nextDay != prevDay) {
      nextTs.setTime_t(nextDay * 86400);
      nextTs.setTimeSpec(Qt::LocalTime);
      Message dayChangeMsg = Message::ChangeOfDay(nextTs);
      dayChangeMsg.setMsgId(msglist.last().msgId());
      dayChangeItem = createMessageModelItem(dayChangeMsg);
    }
  }

  if(dayChangeItem)
    end++;

  Q_ASSERT(start == 0 || _messageList[start - 1]->msgId() < msglist.first().msgId());
  Q_ASSERT(start == _messageList.count() || _messageList[start]->msgId() > msglist.last().msgId());
  beginInsertRows(QModelIndex(), start, end);
  int pos = start;
  foreach(Message msg, msglist) {
    _messageList.insert(pos, createMessageModelItem(msg));
    pos++;
  }
  if(dayChangeItem) {
    _messageList.insert(pos, dayChangeItem);
    pos++; // needed for the following assert
  }
  endInsertRows();

//   Q_ASSERT(start == end || _messageList.at(start)->msgId() != _messageList.at(end)->msgId() || _messageList.at(end)->msgType() == Message::DayChange);
  Q_ASSERT(start == 0 || _messageList[start - 1]->msgId() < _messageList[start]->msgId());
  Q_ASSERT(end + 1 == _messageList.count() || _messageList[end]->msgId() < _messageList[end + 1]->msgId());
  Q_ASSERT(pos - 1 == end);
}

int MessageModel::insertMessagesGracefully(const QList<Message> &msglist) {
  /* short description:
   * 1) first we check where the message with the highest msgId from msglist would be inserted
   * 2) check that position for dupe
   * 3) determine the messageId of the preceeding msg
   * 4) insert as many msgs from msglist with with msgId larger then the just determined id
   *    those messages are automatically less then the msg of the position we just determined in 1)
   */
  bool inOrder = (msglist.first().msgId() < msglist.last().msgId());
  // depending on the order we have to traverse from the front to the back or vice versa

  QList<Message> grouplist;
  MsgId minId;
  MsgId dupeId;
  int processedMsgs = 1; // we know the list isn't empty, so we at least process one message
  int idx;
  bool fastForward = false;
  QList<Message>::const_iterator iter;
  if(inOrder) {
    iter = msglist.constEnd();
    iter--; // this op is safe as we've allready passed an empty check
  } else {
    iter = msglist.constBegin();
  }

  idx = indexForId((*iter).msgId());
  if(idx < _messageList.count())
    dupeId = _messageList[idx]->msgId();

  // we always compare to the previous entry...
  // if there isn't, we can fastforward to the top
  if(idx - 1 >= 0)
    minId = _messageList[idx - 1]->msgId();
  else
    fastForward = true;

  if((*iter).msgId() != dupeId) {
    grouplist << *iter;
    dupeId = (*iter).msgId();
  }

  if(!inOrder)
    iter++;

  if(inOrder) {
    while(iter != msglist.constBegin()) {
      iter--;

      if(!fastForward && (*iter).msgId() < minId)
	break;
      processedMsgs++;

      if(grouplist.isEmpty()) { // as long as we don't have a starting point, we have to update the dupeId
	idx = indexForId((*iter).msgId());
	if(idx >= 0 && !_messageList.isEmpty())
	  dupeId = _messageList[idx]->msgId();
      }
      if((*iter).msgId() != dupeId) {
	if(!grouplist.isEmpty()) {
	  QDateTime nextTs = grouplist.value(0).timestamp();
	  QDateTime prevTs = (*iter).timestamp();
	  nextTs.setTimeSpec(Qt::UTC);
	  prevTs.setTimeSpec(Qt::UTC);
	  uint nextDay = nextTs.toTime_t() / 86400;
	  uint prevDay = prevTs.toTime_t() / 86400;
	  if(nextDay != prevDay) {
	    nextTs.setTime_t(nextDay * 86400);
	    nextTs.setTimeSpec(Qt::LocalTime);
	    Message dayChangeMsg = Message::ChangeOfDay(nextTs);
	    dayChangeMsg.setMsgId((*iter).msgId());
	    grouplist.prepend(dayChangeMsg);
	  }
	}
	dupeId = (*iter).msgId();
	grouplist.prepend(*iter);
      }
    }
  } else {
    while(iter != msglist.constEnd()) {
      if(!fastForward && (*iter).msgId() < minId)
	break;
      processedMsgs++;

      if(grouplist.isEmpty()) { // as long as we don't have a starting point, we have to update the dupeId
	idx = indexForId((*iter).msgId());
	if(idx >= 0 && !_messageList.isEmpty())
	  dupeId = _messageList[idx]->msgId();
      }
      if((*iter).msgId() != dupeId) {
	if(!grouplist.isEmpty()) {
	  QDateTime nextTs = grouplist.value(0).timestamp();
	  QDateTime prevTs = (*iter).timestamp();
	  nextTs.setTimeSpec(Qt::UTC);
	  prevTs.setTimeSpec(Qt::UTC);
	  uint nextDay = nextTs.toTime_t() / 86400;
	  uint prevDay = prevTs.toTime_t() / 86400;
	  if(nextDay != prevDay) {
	    nextTs.setTime_t(nextDay * 86400);
	    nextTs.setTimeSpec(Qt::LocalTime);
	    Message dayChangeMsg = Message::ChangeOfDay(nextTs);
	    dayChangeMsg.setMsgId((*iter).msgId());
	    grouplist.prepend(dayChangeMsg);
	  }
	}
	dupeId = (*iter).msgId();
	grouplist.prepend(*iter);
      }
      iter++;
    }
  }

  if(!grouplist.isEmpty())
    insertMessageGroup(grouplist);
  return processedMsgs;
}

void MessageModel::customEvent(QEvent *event) {
  if(event->type() != QEvent::User)
    return;

  event->accept();

  if(_messageBuffer.isEmpty())
    return;

  int processedMsgs = insertMessagesGracefully(_messageBuffer);
  int remainingMsgs = _messageBuffer.count() - processedMsgs;

  QList<Message>::iterator removeStart = _messageBuffer.begin() + remainingMsgs;
  QList<Message>::iterator removeEnd = _messageBuffer.end();
  _messageBuffer.erase(removeStart, removeEnd);
  if(!_messageBuffer.isEmpty())
    QCoreApplication::postEvent(this, new ProcessBufferEvent());
}

void MessageModel::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
  qDeleteAll(_messageList);
  _messageList.clear();
  endRemoveRows();
  _messagesWaiting.clear();
}

// returns index of msg with given Id or of the next message after that (i.e., the index where we'd insert this msg)
int MessageModel::indexForId(MsgId id) {
  if(_messageList.isEmpty() || id <= _messageList.value(0)->msgId())
    return 0;
  if(id > _messageList.last()->msgId())
    return _messageList.count();

  // binary search
  int start = 0; int end = _messageList.count()-1;
  while(1) {
    if(end - start == 1)
      return end;
    int pivot = (end + start) / 2;
    if(id <= _messageList.value(pivot)->msgId()) end = pivot;
    else start = pivot;
  }
}

void MessageModel::changeOfDay() {
  _dayChangeTimer.setInterval(86400000);
  if(!_messageList.isEmpty()) {
    int idx = _messageList.count();
    while(idx > 0 && _messageList[idx - 1]->timeStamp() > _nextDayChange) {
      idx--;
    }
    beginInsertRows(QModelIndex(), idx, idx);
    Message dayChangeMsg = Message::ChangeOfDay(_nextDayChange);
    dayChangeMsg.setMsgId(_messageList[idx - 1]->msgId());
    _messageList.insert(idx, createMessageModelItem(dayChangeMsg));
    endInsertRows();
  }
  _nextDayChange = _nextDayChange.addSecs(86400);
}

void MessageModel::requestBacklog(BufferId bufferId) {
  if(_messagesWaiting.contains(bufferId))
    return;

  BacklogSettings backlogSettings;
  int requestCount = backlogSettings.dynamicBacklogAmount();

  for(int i = 0; i < _messageList.count(); i++) {
    if(_messageList.at(i)->bufferId() == bufferId) {
      _messagesWaiting[bufferId] = requestCount;
      Client::backlogManager()->emitMessagesRequested(tr("Requesting %1 messages from backlog for buffer %2:%3")
						      .arg(requestCount)
						      .arg(Client::networkModel()->networkName(bufferId))
						      .arg(Client::networkModel()->bufferName(bufferId)));
      Client::backlogManager()->requestBacklog(bufferId, requestCount, _messageList.at(i)->msgId().toInt());
      return;
    }
  }
}

void MessageModel::messagesReceived(BufferId bufferId, int count) {
  if(!_messagesWaiting.contains(bufferId))
    return;

  _messagesWaiting[bufferId] -= count;
  if(_messagesWaiting[bufferId] <= 0)
    _messagesWaiting.remove(bufferId);
}

// ========================================
//  MessageModelItem
// ========================================
MessageModelItem::MessageModelItem(const Message &msg) :
  _timestamp(msg.timestamp()),
  _msgId(msg.msgId()),
  _bufferId(msg.bufferInfo().bufferId()),
  _type(msg.type()),
  _flags(msg.flags())
{
  if(!msg.sender().contains('!'))
    _flags |= Message::ServerMsg;
}

QVariant MessageModelItem::data(int column, int role) const {
  if(column < MessageModel::TimestampColumn || column > MessageModel::ContentsColumn)
    return QVariant();

  switch(role) {
  case MessageModel::MsgIdRole: return QVariant::fromValue<MsgId>(_msgId);
  case MessageModel::BufferIdRole: return QVariant::fromValue<BufferId>(_bufferId);
  case MessageModel::TypeRole: return _type;
  case MessageModel::FlagsRole: return (int)_flags;
  case MessageModel::TimestampRole: return _timestamp;
  case MessageModel::RedirectedToRole: return qVariantFromValue<BufferId>(_redirectedTo);
  default: return QVariant();
  }
}

bool MessageModelItem::setData(int column, const QVariant &value, int role) {
  Q_UNUSED(column);

  switch(role) {
  case MessageModel::RedirectedToRole:
    _redirectedTo = value.value<BufferId>();
    return true;
  default:
    return false;
  }
}


// Stuff for later
bool MessageModelItem::lessThan(const MessageModelItem *m1, const MessageModelItem *m2){
  return (*m1) < (*m2);
}

bool MessageModelItem::operator<(const MessageModelItem &other) const {
  return _msgId < other._msgId;
}

bool MessageModelItem::operator==(const MessageModelItem &other) const {
  return _msgId == other._msgId;
}

bool MessageModelItem::operator>(const MessageModelItem &other) const {
  return _msgId > other._msgId;
}

QDebug operator<<(QDebug dbg, const MessageModelItem &msgItem) {
  dbg.nospace() << qPrintable(QString("MessageModelItem(MsgId:")) << msgItem.msgId()
		<< qPrintable(QString(",")) << msgItem.timeStamp()
		<< qPrintable(QString(", Type:")) << msgItem.msgType()
		<< qPrintable(QString(", Flags:")) << msgItem.msgFlags() << qPrintable(QString(")"))
		<< msgItem.data(1, Qt::DisplayRole).toString() << ":" << msgItem.data(2, Qt::DisplayRole).toString();
  return dbg;
}
