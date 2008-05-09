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

#include "chatlinemodelitem.h"
#include "chatlinemodel.h"
#include "qtui.h"
#include "uistyle.h"

ChatlineModelItem::ChatlineModelItem(const Message &msg) : MessageModelItem(msg) {
  QtUiStyle::StyledMessage m = QtUi::style()->styleMessage(msg);

  _timestamp.plainText = m.timestamp.plainText;
  _sender.plainText = m.sender.plainText;
  _contents.plainText = m.contents.plainText;

  _timestamp.formatList = m.timestamp.formatList;
  _sender.formatList = m.sender.formatList;
  _contents.formatList = m.contents.formatList;

}


QVariant ChatlineModelItem::data(int column, int role) const {
  const ChatlinePart *part;

  switch(column) {
    case ChatlineModel::TimestampColumn: part = &_timestamp; break;
    case ChatlineModel::SenderColumn:    part = &_sender; break;
    case ChatlineModel::TextColumn:      part = &_contents; break;
    default: return MessageModelItem::data(column, role);
  }

  switch(role) {
    case ChatlineModel::DisplayRole: return part->plainText;
    case ChatlineModel::FormatRole:  return QVariant::fromValue<UiStyle::FormatList>(part->formatList);
  }

  return MessageModelItem::data(column, role);
}

bool ChatlineModelItem::setData(int column, const QVariant &value, int role) {
  return false;
}
