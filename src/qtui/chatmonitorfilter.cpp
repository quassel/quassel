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

#include "chatlinemodel.h"

ChatMonitorFilter::ChatMonitorFilter(MessageModel *model, QObject *parent)
: MessageFilter(model, QList<BufferId>(), parent)
{
  _initTime = QDateTime::currentDateTime();

}

bool ChatMonitorFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
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
    /*
    BufferId bufid = data(index, ChatLineModel::BufferIdRole);
    if(bufid.isValid) {
      const Network *net = Client::networkModel()->networkByIndex(Client::networkModel()->bufferIndex(bufid));
      if(!net) {
        qDebug() << "invalid net!";
        return QVariant();
      }

  */
  }
  return MessageFilter::data(index, role);

}
