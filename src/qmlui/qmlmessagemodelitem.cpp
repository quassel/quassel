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

#include "graphicalui.h"
#include "qmlchatline.h"
#include "qmlmessagemodel.h"
#include "qmlmessagemodelitem.h"
#include "uistyle.h"

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
  case QmlMessageModel::MsgLabelRole:
    return messageLabel();

  case QmlMessageModel::RenderDataRole: {
    QmlChatLine::RenderData data;
    data[QmlChatLine::TimestampColumn].text = _styledMsg.decoratedTimestamp();
    data[QmlChatLine::TimestampColumn].formats = UiStyle::FormatList() << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Timestamp);
    data[QmlChatLine::TimestampColumn].background = backgroundBrush(UiStyle::Timestamp).value<QBrush>();
    data[QmlChatLine::TimestampColumn].selectedBackground = backgroundBrush(UiStyle::Timestamp, true).value<QBrush>();

    data[QmlChatLine::SenderColumn].text = _styledMsg.decoratedSender();
    data[QmlChatLine::SenderColumn].formats = UiStyle::FormatList() << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Sender);
    data[QmlChatLine::SenderColumn].background = backgroundBrush(UiStyle::Sender).value<QBrush>();
    data[QmlChatLine::SenderColumn].selectedBackground = backgroundBrush(UiStyle::Sender, true).value<QBrush>();

    data[QmlChatLine::ContentsColumn].text = _styledMsg.plainContents();
    data[QmlChatLine::ContentsColumn].formats = _styledMsg.contentsFormatList();
    data[QmlChatLine::ContentsColumn].background = backgroundBrush(UiStyle::Contents).value<QBrush>();
    data[QmlChatLine::ContentsColumn].selectedBackground = backgroundBrush(UiStyle::Contents, true).value<QBrush>();

    data.isValid = true;
    return QVariant::fromValue<QmlChatLine::RenderData>(data);
  }

  default:
    break;
  }

  MessageModel::ColumnType col = (MessageModel::ColumnType)column;
  switch(col) {
  case QmlMessageModel::TimestampColumn:
    variant = timestampData(role);
    break;
  case QmlMessageModel::SenderColumn:
    variant = senderData(role);
    break;
  case QmlMessageModel::ContentsColumn:
    variant = contentsData(role);
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
  case QmlMessageModel::DisplayRole:
    return _styledMsg.decoratedTimestamp();
  case QmlMessageModel::EditRole:
    return _styledMsg.timestamp();
  case QmlMessageModel::BackgroundRole:
    return backgroundBrush(UiStyle::Timestamp);
  case QmlMessageModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Timestamp, true);
  case QmlMessageModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                      << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Timestamp));
  }
  return QVariant();
}

QVariant QmlMessageModelItem::senderData(int role) const {
  switch(role) {
  case QmlMessageModel::DisplayRole:
    return _styledMsg.decoratedSender();
  case QmlMessageModel::EditRole:
    return _styledMsg.plainSender();
  case QmlMessageModel::BackgroundRole:
    return backgroundBrush(UiStyle::Sender);
  case QmlMessageModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Sender, true);
  case QmlMessageModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(UiStyle::FormatList()
                      << qMakePair((quint16)0, (quint32)UiStyle::formatType(_styledMsg.type()) | UiStyle::Sender));
  }
  return QVariant();
}

QVariant QmlMessageModelItem::contentsData(int role) const {
  switch(role) {
  case QmlMessageModel::DisplayRole:
  case QmlMessageModel::EditRole:
    return _styledMsg.plainContents();
  case QmlMessageModel::BackgroundRole:
    return backgroundBrush(UiStyle::Contents);
  case QmlMessageModel::SelectedBackgroundRole:
    return backgroundBrush(UiStyle::Contents, true);
  case QmlMessageModel::FormatRole:
    return QVariant::fromValue<UiStyle::FormatList>(_styledMsg.contentsFormatList());
  }
  return QVariant();
}

quint32 QmlMessageModelItem::messageLabel() const {
  quint32 label = _styledMsg.senderHash() << 16;
  if(_styledMsg.flags() & Message::Self)
    label |= UiStyle::OwnMsg;
  if(_styledMsg.flags() & Message::Highlight)
    label |= UiStyle::Highlight;
  return label;
}

QVariant QmlMessageModelItem::backgroundBrush(UiStyle::FormatType subelement, bool selected) const {
  QTextCharFormat fmt = GraphicalUi::uiStyle()->format(UiStyle::formatType(_styledMsg.type()) | subelement, messageLabel() | (selected ? UiStyle::Selected : 0));
  if(fmt.hasProperty(QTextFormat::BackgroundBrush))
    return QVariant::fromValue<QBrush>(fmt.background());
  return QVariant();
}
