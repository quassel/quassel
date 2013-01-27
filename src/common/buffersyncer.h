/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef BUFFERSYNCER_H_
#define BUFFERSYNCER_H_

#include "syncableobject.h"
#include "types.h"

class BufferSyncer : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    explicit BufferSyncer(QObject *parent);
    explicit BufferSyncer(const QHash<BufferId, MsgId> &lastSeenMsg, const QHash<BufferId, MsgId> &markerLines, QObject *parent);

    inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

    MsgId lastSeenMsg(BufferId buffer) const;
    MsgId markerLine(BufferId buffer) const;

public slots:
    QVariantList initLastSeenMsg() const;
    void initSetLastSeenMsg(const QVariantList &);

    QVariantList initMarkerLines() const;
    void initSetMarkerLines(const QVariantList &);

    virtual inline void requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId) { REQUEST(ARG(buffer), ARG(msgId)) }
    virtual inline void requestSetMarkerLine(BufferId buffer, const MsgId &msgId) { REQUEST(ARG(buffer), ARG(msgId)) setMarkerLine(buffer, msgId); }

    virtual inline void requestRemoveBuffer(BufferId buffer) { REQUEST(ARG(buffer)) }
    virtual void removeBuffer(BufferId buffer);

    virtual inline void requestRenameBuffer(BufferId buffer, QString newName) { REQUEST(ARG(buffer), ARG(newName)) }
    virtual inline void renameBuffer(BufferId buffer, QString newName) { SYNC(ARG(buffer), ARG(newName)) emit bufferRenamed(buffer, newName); }

    virtual inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) { emit REQUEST(ARG(buffer1), ARG(buffer2)) }
    virtual void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2);

    virtual inline void requestPurgeBufferIds() { REQUEST(NO_ARG); }

    virtual inline void requestMarkBufferAsRead(BufferId buffer) { REQUEST(ARG(buffer)) emit bufferMarkedAsRead(buffer); }
    virtual inline void markBufferAsRead(BufferId buffer) { SYNC(ARG(buffer)) emit bufferMarkedAsRead(buffer); }

signals:
    void lastSeenMsgSet(BufferId buffer, const MsgId &msgId);
    void markerLineSet(BufferId buffer, const MsgId &msgId);
    void bufferRemoved(BufferId buffer);
    void bufferRenamed(BufferId buffer, QString newName);
    void buffersPermanentlyMerged(BufferId buffer1, BufferId buffer2);
    void bufferMarkedAsRead(BufferId buffer);

protected slots:
    bool setLastSeenMsg(BufferId buffer, const MsgId &msgId);
    bool setMarkerLine(BufferId buffer, const MsgId &msgId);

protected:
    inline QList<BufferId> lastSeenBufferIds() const { return _lastSeenMsg.keys(); }
    inline QList<BufferId> markerLineBufferIds() const { return _markerLines.keys(); }
    inline QHash<BufferId, MsgId> markerLines() const { return _markerLines; }

private:
    QHash<BufferId, MsgId> _lastSeenMsg;
    QHash<BufferId, MsgId> _markerLines;
};


#endif
