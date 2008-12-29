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

#include "syncableobject.h"
#include "types.h"

class BufferSyncer : public SyncableObject {
  Q_OBJECT

public:
  explicit BufferSyncer(QObject *parent);
  inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

  MsgId lastSeenMsg(BufferId buffer) const;

public slots:
  QVariantList initLastSeenMsg() const;
  void initSetLastSeenMsg(const QVariantList &);

  virtual inline void requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId) { emit setLastSeenMsgRequested(buffer, msgId); }

  virtual inline void requestRemoveBuffer(BufferId buffer) { emit removeBufferRequested(buffer); }
  virtual void removeBuffer(BufferId buffer);

  virtual inline void requestRenameBuffer(BufferId buffer, QString newName) { emit renameBufferRequested(buffer, newName); }
  virtual inline void renameBuffer(BufferId buffer, QString newName) { emit bufferRenamed(buffer, newName); }

  virtual inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) { emit mergeBuffersPermanentlyRequested(buffer1, buffer2); }
  virtual void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2);

signals:
  void lastSeenMsgSet(BufferId buffer, const MsgId &msgId);
  void setLastSeenMsgRequested(BufferId buffer, const MsgId &msgId);

  void removeBufferRequested(BufferId buffer);
  void bufferRemoved(BufferId buffer);

  void renameBufferRequested(BufferId buffer, QString newName);
  void bufferRenamed(BufferId buffer, QString newName);

  void mergeBuffersPermanentlyRequested(BufferId buffer1, BufferId buffer2);
  void buffersPermanentlyMerged(BufferId buffer1, BufferId buffer2);

protected slots:
  bool setLastSeenMsg(BufferId buffer, const MsgId &msgId);

private:
  QHash<BufferId, MsgId> _lastSeenMsg;
};

#endif
