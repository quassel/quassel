/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#include "qmlmessagemodel.h"
#include "qmlmessagemodelitem.h"

QmlMessageModelItem::QmlMessageModelItem(const Message &msg)
  : MessageModelItem(),
    _styledMsg(msg)
{
  if(!msg.sender().contains('!'))
    _styledMsg.setFlags(msg.flags() |= Message::ServerMsg);
}

bool QmlMessageModelItem::setData(int column, const QVariant &value, int role) {
  switch(role) {
    case MessageModel::FlagsRole:
      _styledMsg.setFlags((Message::Flags)value.toUInt());
      return true;
    default:
      return MessageModelItem::setData(column, value, role);
  }
}

QVariant QmlMessageModelItem::data(int column, int role) const {
  QVariant variant;
  switch(role) {
  case QmlMessageModel::TimestampRole:
    variant = _styledMsg.timestamp();
    break;
  case QmlMessageModel::SenderRole:
    variant = _styledMsg.sender();
    break;
  case QmlMessageModel::ContentsRole:
    variant = _styledMsg.contents();
    break;
  default:
    break;
  }
  if(!variant.isValid())
    return MessageModelItem::data(column, role);
  return variant;
}

QVariant QmlMessageModelItem::timestampData(int role) const {
  switch(role) {
  case MessageModel::DisplayRole:
    return _styledMsg.timestamp();
  }
  return QVariant();
}

QVariant QmlMessageModelItem::senderData(int role) const {
  switch(role) {
  case MessageModel::DisplayRole:
    return _styledMsg.sender();
  }
  return QVariant();
}

QVariant QmlMessageModelItem::contentsData(int role) const {
  switch(role) {
  case MessageModel::DisplayRole:
    return _styledMsg.contents();
  }
  return QVariant();
}

