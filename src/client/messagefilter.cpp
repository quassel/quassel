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
#include "buffermodel.h"
#include "messagemodel.h"
#include "networkmodel.h"

MessageFilter::MessageFilter(QAbstractItemModel *source, QObject *parent)
  : QSortFilterProxyModel(parent),
    _messageTypeFilter(0)
{
  init();
  setSourceModel(source);
}

MessageFilter::MessageFilter(MessageModel *source, const QList<BufferId> &buffers, QObject *parent)
  : QSortFilterProxyModel(parent),
    _validBuffers(buffers.toSet()),
    _messageTypeFilter(0)
{
  init();
  setSourceModel(source);
}

void MessageFilter::init() {
  setDynamicSortFilter(true);

  BufferSettings defaultSettings;
  _messageTypeFilter = defaultSettings.messageFilter();
  defaultSettings.notify("MessageTypeFilter", this, SLOT(messageTypeFilterChanged()));
  defaultSettings.notify("UserNoticesInDefaultBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("UserNoticesInStatusBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("UserNoticesInCurrentBuffer", this, SLOT(messageRedirectionChanged()));

  defaultSettings.notify("serverNoticesInDefaultBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("serverNoticesInStatusBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("serverNoticesInCurrentBuffer", this, SLOT(messageRedirectionChanged()));

  defaultSettings.notify("ErrorMsgsInDefaultBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("ErrorMsgsInStatusBuffer", this, SLOT(messageRedirectionChanged()));
  defaultSettings.notify("ErrorMsgsInCurrentBuffer", this, SLOT(messageRedirectionChanged()));
  messageRedirectionChanged();

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

void MessageFilter::messageRedirectionChanged() {
  BufferSettings bufferSettings;
  _userNoticesInDefaultBuffer = bufferSettings.value("UserNoticesInDefaultBuffer", QVariant(true)).toBool();
  _userNoticesInStatusBuffer = bufferSettings.value("UserNoticesInStatusBuffer", QVariant(false)).toBool();
  _userNoticesInCurrentBuffer = bufferSettings.value("UserNoticesInCurrentBuffer", QVariant(false)).toBool();

  _serverNoticesInDefaultBuffer = bufferSettings.value("ServerNoticesInDefaultBuffer", QVariant(false)).toBool();
  _serverNoticesInStatusBuffer = bufferSettings.value("ServerNoticesInStatusBuffer", QVariant(true)).toBool();
  _serverNoticesInCurrentBuffer = bufferSettings.value("ServerNoticesInCurrentBuffer", QVariant(false)).toBool();

  _errorMsgsInDefaultBuffer = bufferSettings.value("ErrorMsgsInDefaultBuffer", QVariant(true)).toBool();
  _errorMsgsInStatusBuffer = bufferSettings.value("ErrorMsgsInStatusBuffer", QVariant(false)).toBool();
  _errorMsgsInCurrentBuffer = bufferSettings.value("ErrorMsgsInCurrentBuffer", QVariant(false)).toBool();

  invalidateFilter();
}

QString MessageFilter::idString() const {
  if(_validBuffers.isEmpty())
    return "*";

  QList<BufferId> bufferIds = _validBuffers.toList();
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

  BufferId bufferId = sourceModel()->data(sourceIdx, MessageModel::BufferIdRole).value<BufferId>();
  if(!bufferId.isValid()) {
    return true;
  }

  MsgId msgId = sourceModel()->data(sourceIdx, MessageModel::MsgIdRole).value<MsgId>();
  Message::Flags flags = (Message::Flags)sourceModel()->data(sourceIdx, MessageModel::FlagsRole).toInt();

  NetworkId myNetworkId = networkId();
  NetworkId msgNetworkId = Client::networkModel()->networkId(bufferId);
  if(myNetworkId != msgNetworkId)
    return false;

  bool redirect = false;
  bool inDefaultBuffer;
  bool inStatusBuffer;
  bool inCurrentBuffer;

  switch(messageType) {
  case Message::Notice:
    if(Client::networkModel()->bufferType(bufferId) != BufferInfo::ChannelBuffer) {
      redirect = true;
      if(flags & Message::ServerMsg) {
	// server notice
	inDefaultBuffer = _serverNoticesInDefaultBuffer;
	inStatusBuffer = _serverNoticesInStatusBuffer;
	inCurrentBuffer = _serverNoticesInCurrentBuffer;
      } else {
	inDefaultBuffer = _userNoticesInDefaultBuffer;
	inStatusBuffer = _userNoticesInStatusBuffer;
	inCurrentBuffer = _userNoticesInCurrentBuffer;
      }
    }
    break;
  case Message::Error:
    redirect = true;
    inDefaultBuffer = _errorMsgsInDefaultBuffer;
    inStatusBuffer = _errorMsgsInStatusBuffer;
    inCurrentBuffer = _errorMsgsInCurrentBuffer;
    break;
  default:
    break;
  }

  if(redirect) {
    if(_redirectedMsgs.contains(msgId))
      return true;

    if(inDefaultBuffer && _validBuffers.contains(bufferId))
      return true;

    if(inCurrentBuffer && !(flags & Message::Backlog) && _validBuffers.contains(Client::bufferModel()->currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>())) {
      BufferId redirectedTo = sourceModel()->data(sourceIdx, MessageModel::RedirectedToRole).value<BufferId>();
      if(!redirectedTo.isValid()) {
	sourceModel()->setData(sourceIdx, QVariant::fromValue<BufferId>(singleBufferId()), MessageModel::RedirectedToRole);
	_redirectedMsgs << msgId;
	return true;
      } else if(_validBuffers.contains(redirectedTo)) {
	return true;
      }
    }

    QSet<BufferId>::const_iterator idIter = _validBuffers.constBegin();
    while(idIter != _validBuffers.constEnd()) {
      if(inStatusBuffer && Client::networkModel()->bufferType(*idIter) == BufferInfo::StatusBuffer)
	return true;
      idIter++;
    }

    return false;
  }


  if(_validBuffers.contains(bufferId)) {
    return true;
  } else {
    // show Quit messages in Query buffers:
    if(bufferType() != BufferInfo::QueryBuffer)
      return false;
    if(!(messageType & Message::Quit))
      return false;

    if(myNetworkId != msgNetworkId)
      return false;

    uint messageTimestamp = sourceModel()->data(sourceIdx, MessageModel::TimestampRole).value<QDateTime>().toTime_t();
    QString quiter = sourceModel()->data(sourceIdx, Qt::DisplayRole).toString().section(' ', 0, 0, QString::SectionSkipEmpty).toLower();
    if(quiter != bufferName().toLower())
      return false;

    if(_filteredQuitMsgs.contains(quiter, messageTimestamp))
      return false;

    MessageFilter *that = const_cast<MessageFilter *>(this);
    that->_filteredQuitMsgs.insert(quiter,  messageTimestamp);
    return true;
  }
}

void MessageFilter::requestBacklog() {
  QSet<BufferId>::const_iterator bufferIdIter = _validBuffers.constBegin();
  while(bufferIdIter != _validBuffers.constEnd()) {
    Client::messageModel()->requestBacklog(*bufferIdIter);
    bufferIdIter++;
  }
}
