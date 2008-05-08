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
  _msg = QtUi::style()->styleMessage(msg);

}


QVariant ChatlineModelItem::data(int column, int role) const {
  switch(role) {
    case ChatlineModel::DisplayRole:
      switch(column) {
        case ChatlineModel::TimestampColumn: return _msg.timestamp.text;
        case ChatlineModel::SenderColumn:    return _msg.sender.text;
        case ChatlineModel::TextColumn:      return _msg.text.text;
      }
      break;
    case ChatlineModel::FormatRole:
      switch(column) {
        case ChatlineModel::TimestampColumn: return QVariant::fromValue<UiStyle::FormatList>(_msg.timestamp.formats);
        case ChatlineModel::SenderColumn:    return QVariant::fromValue<UiStyle::FormatList>(_msg.sender.formats);
        case ChatlineModel::TextColumn:      return QVariant::fromValue<UiStyle::FormatList>(_msg.text.formats);
      }
      break;
  }
  return MessageModelItem::data(column, role);
}

bool ChatlineModelItem::setData(int column, const QVariant &value, int role) {
  return false;
}
