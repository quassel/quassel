/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#pragma once

#include "common-export.h"

#include "message.h"
#include "syncableobject.h"
#include "types.h"

class COMMON_EXPORT BufferSyncer : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT(BufferSyncer,
        requestSetLastSeenMsg,
        setLastSeenMsg,
        requestSetMarkerLine,
        setMarkerLine,
        requestRemoveBuffer,
        removeBuffer,
        requestMergeBuffersPermanently,
        mergeBuffersPermanently,
        setBufferActivity,
        setHighlightCount,
        requestRenameBuffer,
        renameBuffer,
        requestMarkBufferAsRead,
        markBufferAsRead,
        requestPurgeBufferIds
    )

    Q_PROPERTY(QVariantList Activities READ activities WRITE setActivities)
    Q_PROPERTY(QVariantList HighlightCounts READ highlightCounts WRITE setHighlightCounts)
    Q_PROPERTY(QVariantList LastSeenMsg READ lastSeenMsgs WRITE setLastSeenMsgs)
    Q_PROPERTY(QVariantList MarkerLines READ markerLines WRITE setMarkerLines)

public:
    explicit BufferSyncer(QObject* parent);
    explicit BufferSyncer(QHash<BufferId, MsgId> lastSeenMsg,
                          QHash<BufferId, MsgId> markerLines,
                          QHash<BufferId, Message::Types> activities,
                          QHash<BufferId, int> highlightCounts,
                          QObject* parent);

    MsgId lastSeenMsg(BufferId buffer) const;
    MsgId markerLine(BufferId buffer) const;
    Message::Types activity(BufferId buffer) const;
    int highlightCount(BufferId buffer) const;

    void markActivitiesChanged()
    {
        for (auto buffer : _bufferActivities.keys()) {
            emit bufferActivityChanged(buffer, activity(buffer));
        }
    }

    void markHighlightCountsChanged()
    {
        for (auto buffer : _highlightCounts.keys()) {
            emit highlightCountChanged(buffer, highlightCount(buffer));
        }
    }

public slots:
    virtual inline void requestSetLastSeenMsg(BufferId buffer, const MsgId& msgId) { REQUEST(ARG(buffer), ARG(msgId)) }
    virtual inline void requestSetMarkerLine(BufferId buffer, const MsgId& msgId)
    {
        REQUEST(ARG(buffer), ARG(msgId)) setMarkerLine(buffer, msgId);
    }

    virtual inline void setBufferActivity(BufferId buffer, int activity)
    {
        auto flags = Message::Types(activity);
        SYNC(ARG(buffer), ARG(activity));
        _bufferActivities[buffer] = flags;
        emit bufferActivityChanged(buffer, flags);
    }

    virtual inline void setHighlightCount(BufferId buffer, int count)
    {
        SYNC(ARG(buffer), ARG(count));
        _highlightCounts[buffer] = count;
        emit highlightCountChanged(buffer, count);
    }

    virtual inline void requestRemoveBuffer(BufferId buffer) { REQUEST(ARG(buffer)) }
    virtual void removeBuffer(BufferId buffer);

    virtual inline void requestRenameBuffer(BufferId buffer, QString newName) { REQUEST(ARG(buffer), ARG(newName)) }
    virtual inline void renameBuffer(BufferId buffer, QString newName)
    {
        SYNC(ARG(buffer), ARG(newName)) emit bufferRenamed(buffer, newName);
    }

    virtual inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) { emit REQUEST(ARG(buffer1), ARG(buffer2)) }
    virtual void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2);

    virtual inline void requestPurgeBufferIds() { REQUEST(NO_ARG); }

    virtual inline void requestMarkBufferAsRead(BufferId buffer) { REQUEST(ARG(buffer)) emit bufferMarkedAsRead(buffer); }
    virtual inline void markBufferAsRead(BufferId buffer) { SYNC(ARG(buffer)) emit bufferMarkedAsRead(buffer); }

signals:
    void lastSeenMsgSet(BufferId buffer, const MsgId& msgId);
    void markerLineSet(BufferId buffer, const MsgId& msgId);
    void bufferRemoved(BufferId buffer);
    void bufferRenamed(BufferId buffer, QString newName);
    void buffersPermanentlyMerged(BufferId buffer1, BufferId buffer2);
    void bufferMarkedAsRead(BufferId buffer);
    void bufferActivityChanged(BufferId, Message::Types);
    void highlightCountChanged(BufferId, int);

protected slots:
    bool setLastSeenMsg(BufferId buffer, const MsgId& msgId);
    bool setMarkerLine(BufferId buffer, const MsgId& msgId);

protected:
    inline QList<BufferId> lastSeenBufferIds() const { return _lastSeenMsg.keys(); }
    inline QList<BufferId> markerLineBufferIds() const { return _markerLines.keys(); }

private:
    QVariantList lastSeenMsgs() const;
    void setLastSeenMsgs(const QVariantList&);

    QVariantList markerLines() const;
    void setMarkerLines(const QVariantList&);

    QVariantList activities() const;
    void setActivities(const QVariantList&);

    QVariantList highlightCounts() const;
    void setHighlightCounts(const QVariantList&);

private:
    QHash<BufferId, MsgId> _lastSeenMsg;
    QHash<BufferId, MsgId> _markerLines;
    QHash<BufferId, Message::Types> _bufferActivities;
    QHash<BufferId, int> _highlightCounts;
};
