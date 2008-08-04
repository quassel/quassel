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

#include "chatmonitorfilter.h"

#include "buffer.h"
#include "client.h"
#include "chatlinemodel.h"
#include "networkmodel.h"

ChatMonitorFilter::ChatMonitorFilter(MessageModel *model, QObject *parent)
: MessageFilter(model, QList<BufferId>(), parent)
{
  _initTime = QDateTime::currentDateTime();

}

bool ChatMonitorFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  Q_UNUSED(sourceParent)
  Message::Type type = (Message::Type)sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::TypeRole).toInt();
  Message::Flags flags = (Message::Flags)sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::FlagsRole).toInt();
  if(!((type & (Message::Plain | Message::Notice | Message::Action)) || flags & Message::Self))
    return false;
  QDateTime msgTime = sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::TimestampRole).toDateTime();
  return msgTime > _initTime;
}

QString ChatMonitorFilter::idString() const {
  return "ChatMonitor";
}

// override this to inject display of network and channel
QVariant ChatMonitorFilter::data(const QModelIndex &index, int role) const {
  if(index.column() != ChatLineModel::SenderColumn) return MessageFilter::data(index, role);
  if(role == ChatLineModel::DisplayRole) {
    BufferId bufid = data(index, ChatLineModel::BufferIdRole).value<BufferId>();
    if(bufid.isValid()) {
      Buffer *buf = Client::buffer(bufid);
      if(!buf) {
        qDebug() << "invalid buffer!";
        return QVariant();
      }
      const Network *net = Client::networkModel()->networkByIndex(Client::networkModel()->bufferIndex(bufid));
      if(!net) {
        qDebug() << "invalid net!";
        return QVariant();
      }
      QString result = QString("<%1:%2:%3").arg(net->networkName())
                                            .arg(buf->bufferInfo().bufferName())
                                            .arg(MessageFilter::data(index, role).toString().mid(1));
      return result;
    }

  }
  return MessageFilter::data(index, role);

}
