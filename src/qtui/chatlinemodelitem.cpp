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

#include "chatlinemodelitem.h"
#include "chatlinemodel.h"
#include "qtui.h"
#include "qtuistyle.h"

// ****************************************
// the actual ChatLineModelItem
// ****************************************
ChatLineModelItem::ChatLineModelItem(const Message &msg)
  : MessageModelItem(),
    _styledMsg(msg)
{
  if(!msg.sender().contains('!'))
    _styledMsg.setFlags(msg.flags() |= Message::ServerMsg);
}

QVariant ChatLineModelItem::data(int column, int role) const {
  if(role == ChatLineModel::MsgLabelRole)
    return messageLabel();

  QVariant variant;
  MessageModel::ColumnType col = (MessageModel::ColumnType)column;
  switch(col) {
  case ChatLineModel::TimestampColumn:
    variant = timestampData(role);
    break;
  case ChatLineModel::SenderColumn:
    variant = senderData(role);
    break;
  case ChatLineModel::ContentsColumn:
    variant = contentsData(role);
    break;
  default:
    break;
  }
  if(!variant.isValid())
    return MessageModelItem::data(column, role);
  return variant;
}

QVariant ChatLineModelItem::timestampData(int role) const {
  switch(role) {
  case ChatLineModel::DisplayRole:
    return _styledMsg.decoratedTimestamp();
  case ChatLineModel::EditRole:
    return _styledMsg.timestamp();
  case ChatLineModel::BackgroundRole:
    return backgroundBrush(UiStyle::Timestamp);
  case ChatLineModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Timestamp, true);
  case ChatLineModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                      << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Timestamp));
  }
  return QVariant();
}

QVariant ChatLineModelItem::senderData(int role) const {
  switch(role) {
  case ChatLineModel::DisplayRole:
    return _styledMsg.decoratedSender();
  case ChatLineModel::EditRole:
    return _styledMsg.plainSender();
  case ChatLineModel::BackgroundRole:
    return backgroundBrush(UiStyle::Sender);
  case ChatLineModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Sender, true);
  case ChatLineModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                      << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Sender));
  }
  return QVariant();
}

QVariant ChatLineModelItem::contentsData(int role) const {
  switch(role) {
  case ChatLineModel::DisplayRole:
  case ChatLineModel::EditRole:
    return _styledMsg.plainContents();
  case ChatLineModel::BackgroundRole:
    return backgroundBrush(UiStyle::Contents);
  case ChatLineModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Contents, true);
  case ChatLineModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(_styledMsg.contentsFormatList());
  }
  return QVariant();
}

quint32 ChatLineModelItem::messageLabel() const {
  quint32 label = _styledMsg.senderHash() << 16;
  if(_styledMsg.flags() & Message::Self)
    label |= UiStyle::OwnMsg;
  if(_styledMsg.flags() & Message::Highlight)
    label |= UiStyle::Highlight;
  return label;
}

QVariant ChatLineModelItem::backgroundBrush(UiStyle::FormatType subelement, bool selected) const {
  QTextCharFormat fmt = QtUi::style()->format(UiStyle::formatType(_styledMsg.type()) | subelement, messageLabel() | (selected ? UiStyle::Selected : 0));
  if(fmt.hasProperty(QTextFormat::BackgroundBrush))
    return QVariant::fromValue<QBrush>(fmt.background());
  return QVariant();
}
