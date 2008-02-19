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

#ifndef BUFFERSYNCER_H_
#define BUFFERSYNCER_H_

#include <QDateTime>

#include "syncableobject.h"
#include "types.h"

class BufferSyncer : public SyncableObject {
  Q_OBJECT

  public:
    explicit BufferSyncer(QObject *parent);

    QDateTime lastSeen(BufferId buffer) const;

  public slots:
    QVariantList initLastSeen() const;
    void initSetLastSeen(const QVariantList &);

    void requestSetLastSeen(BufferId buffer, const QDateTime &time);
    void requestRemoveBuffer(BufferId buffer);
    void removeBuffer(BufferId buffer);
    void renameBuffer(BufferId buffer, QString newName);

  signals:
    void lastSeenSet(BufferId buffer, const QDateTime &time);
    void setLastSeenRequested(BufferId buffer, const QDateTime &time);
    void removeBufferRequested(BufferId buffer);
    void bufferRemoved(BufferId buffer);
    void bufferRenamed(BufferId buffer, QString newName);

  private slots:
    bool setLastSeen(BufferId buffer, const QDateTime &time);

  private:
    QMap<BufferId, QDateTime> _lastSeen;
};

#endif
