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

#include "message.h"

#include <QEvent>

class ProcessBufferEvent : public QEvent {
public:
  inline ProcessBufferEvent() : QEvent(QEvent::User) {}
};

MessageModel::MessageModel(QObject *parent)
  : QAbstractItemModel(parent)
{
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

  MessageModelItem *item = createMessageModelItem(msg);
  beginInsertRows(QModelIndex(), idx, idx);
  _messageList.insert(idx, item);
  endInsertRows();
  return true;
}

void MessageModel::insertMessages(const QList<Message> &msglist) {
  if(msglist.isEmpty())
    return;

  if(_messageList.isEmpty()) {
    insertMessageGroup(msglist);
  } else {
    int processedMsgs = insertMessagesGracefully(msglist);
    int remainingMsgs = msglist.count() - processedMsgs;
    if(remainingMsgs > 0) {
      if(msglist.first().msgId() < msglist.last().msgId()) {
	// in Order
	_messageBuffer << msglist.mid(0, remainingMsgs);
      } else {
	_messageBuffer << msglist.mid(processedMsgs);
      }
      qSort(_messageBuffer);
      QCoreApplication::postEvent(this, new ProcessBufferEvent());
    }
  }
}

void MessageModel::insertMessageGroup(const QList<Message> &msglist) {
  int idx = indexForId(msglist.first().msgId());
  beginInsertRows(QModelIndex(), idx, idx+msglist.count()-1);

  foreach(Message msg, msglist) {
    _messageList.insert(idx, createMessageModelItem(msg));
    idx++;
  }

  endInsertRows();
}

int MessageModel::insertMessagesGracefully(const QList<Message> &msglist) {
  bool inOrder = (msglist.first().msgId() < msglist.last().msgId());
  // depending on the order we have to traverse from the front to the back or vice versa

  QList<Message> grouplist;
  MsgId id;
  MsgId dupeId;
  int dupeCount = 0;
  bool fastForward = false;
  QList<Message>::const_iterator iter;
  if(inOrder) {
    iter = msglist.constEnd();
    iter--; // this op is safe as we've allready passed an empty check
  } else {
    iter = msglist.constBegin();
  }

  int idx = indexForId((*iter).msgId());
  if(idx >= 0)
    dupeId = _messageList[idx]->msgId();

  // we always compare to the previous entry...
  // if there isn't, we can fastforward to the top
  if(idx - 1 >= 0)
    id = _messageList[idx - 1]->msgId();
  else
    fastForward = true;

  if((*iter).msgId() != dupeId)
    grouplist << *iter;
  else
    dupeCount++;

  if(!inOrder)
    iter++;

  if(inOrder) {
    while(iter != msglist.constBegin()) {
      iter--;

      if(!fastForward && (*iter).msgId() < id)
	break;

      if((*iter).msgId() != dupeId)
	grouplist.prepend(*iter);
      else
	dupeCount++;
    }
  } else {
    while(iter != msglist.constEnd()) {
      if(!fastForward && (*iter).msgId() < id)
	break;

      if((*iter).msgId() != dupeId)
	grouplist.prepend(*iter);
      else
	dupeCount++;

      iter++;
    }
  }

  insertMessageGroup(grouplist);
  return grouplist.count() + dupeCount;
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
}



// returns index of msg with given Id or of the next message after that (i.e., the index where we'd insert this msg)
int MessageModel::indexForId(MsgId id) {
  if(_messageList.isEmpty() || id <= _messageList.value(0)->data(0, MsgIdRole).value<MsgId>()) return 0;
  if(id > _messageList.last()->data(0, MsgIdRole).value<MsgId>()) return _messageList.count();
  // binary search
  int start = 0; int end = _messageList.count()-1;
  while(1) {
    if(end - start == 1) return end;
    int pivot = (end + start) / 2;
    if(id <= _messageList.value(pivot)->data(0, MsgIdRole).value<MsgId>()) end = pivot;
    else start = pivot;
  }
}

/**********************************************************************************/

MessageModelItem::MessageModelItem(const Message &msg) :
  _timestamp(msg.timestamp()),
  _msgId(msg.msgId()),
  _bufferId(msg.bufferInfo().bufferId()),
  _type(msg.type()),
  _flags(msg.flags())
{
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
  default: return QVariant();
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
