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

#include "messagefilter.h"

#include "buffersettings.h"
#include "client.h"
#include "messagemodel.h"
#include "networkmodel.h"

MessageFilter::MessageFilter(QAbstractItemModel *source, QObject *parent)
  : QSortFilterProxyModel(parent),
    _messageTypeFilter(0),
    _bufferType(BufferInfo::InvalidBuffer)
{
  init();
  setSourceModel(source);
}

MessageFilter::MessageFilter(MessageModel *source, const QList<BufferId> &buffers, QObject *parent)
  : QSortFilterProxyModel(parent),
    _validBuffers(buffers.toSet()),
    _messageTypeFilter(0),
    _bufferType(BufferInfo::InvalidBuffer)
{
  init();
  setSourceModel(source);
}

void MessageFilter::init() {
  BufferSettings defaultSettings;
  _messageTypeFilter = defaultSettings.messageFilter();
  defaultSettings.notify("MessageTypeFilter", this, SLOT(messageTypeFilterChanged()));

  BufferSettings mySettings(idString());
  if(mySettings.hasFilter())
    _messageTypeFilter = mySettings.messageFilter();
  mySettings.notify("MessageTypeFilter", this, SLOT(messageTypeFilterChanged()));
}

void MessageFilter::messageTypeFilterChanged() {
  int newFilter;
  BufferSettings defaultSettings;
  newFilter = BufferSettings().messageFilter();

  BufferSettings mySettings(idString());
  if(mySettings.hasFilter())
    newFilter = mySettings.messageFilter();

  if(_messageTypeFilter != newFilter) {
    _messageTypeFilter = newFilter;
    _filteredQuitMsgs.clear();
    invalidateFilter();
  }
}

QString MessageFilter::idString() const {
  if(_validBuffers.isEmpty())
    return "*";

  QList<BufferId> bufferIds = _validBuffers.toList();;
  qSort(bufferIds);
  
  QStringList bufferIdStrings;
  foreach(BufferId id, bufferIds)
    bufferIdStrings << QString::number(id.toInt());

  return bufferIdStrings.join("|");
}

bool MessageFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  Q_UNUSED(sourceParent);
  QModelIndex sourceIdx = sourceModel()->index(sourceRow, 2);
  Message::Type messageType = (Message::Type)sourceModel()->data(sourceIdx, MessageModel::TypeRole).toInt();

  // apply message type filter
  if(_messageTypeFilter & messageType)
    return false;

  if(_validBuffers.isEmpty())
    return true;

  BufferId id = sourceModel()->data(sourceIdx, MessageModel::BufferIdRole).value<BufferId>();
  if(!id.isValid()) {
    return true;
  }

  if(_validBuffers.contains(id)) {
    return true;
  } else {
    // show Quit messages in Query buffers:
    if(bufferType() != BufferInfo::QueryBuffer)
      return false;
    if(!(messageType & Message::Quit))
      return false;

    uint messageTimestamp = sourceModel()->data(sourceIdx, MessageModel::TimestampRole).value<QDateTime>().toTime_t();
    if(_filteredQuitMsgs.contains(messageTimestamp))
      return false;

    QString quiter = sourceModel()->data(sourceIdx, Qt::DisplayRole).toString().section(' ', 0, 0, QString::SectionSkipEmpty);
    if(quiter.toLower() == bufferName().toLower()) {
      MessageFilter *that = const_cast<MessageFilter *>(this);
      that->_filteredQuitMsgs << messageTimestamp;
      return true;
    } else {
      return false;
    }
  }
}

void MessageFilter::requestBacklog() {
  QSet<BufferId>::const_iterator bufferIdIter = _validBuffers.constBegin();
  while(bufferIdIter != _validBuffers.constEnd()) {
    Client::messageModel()->requestBacklog(*bufferIdIter);
    bufferIdIter++;
  }
}

const QString &MessageFilter::bufferName() const {
  if(_bufferName.isEmpty()) {
    MessageFilter *that = const_cast<MessageFilter *>(this);
    that->_bufferName = Client::networkModel()->bufferName(singleBufferId());
    return that->_bufferName;
  }
  return _bufferName;
}

BufferInfo::Type MessageFilter::bufferType() const {
  if(_bufferType == BufferInfo::InvalidBuffer) {
    MessageFilter *that = const_cast<MessageFilter *>(this);
    that->_bufferType = Client::networkModel()->bufferType(singleBufferId());
    return that->_bufferType;
  }
  return _bufferType;
}
