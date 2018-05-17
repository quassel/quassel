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

#include "buffersyncer.h"

INIT_SYNCABLE_OBJECT(BufferSyncer)
BufferSyncer::BufferSyncer(QObject *parent)
    : SyncableObject(parent)
{
}


BufferSyncer::BufferSyncer(
        const QHash<BufferId, MsgId> &lastSeenMsg,
        const QHash<BufferId, MsgId> &markerLines,
        const QHash<BufferId, Message::Types> &activities,
        const QHash<BufferId, int> &highlightCounts,
        QObject *parent
) : SyncableObject(parent),
    _lastSeenMsg(lastSeenMsg),
    _markerLines(markerLines),
    _bufferActivities(activities),
    _highlightCounts(highlightCounts)
{
}


MsgId BufferSyncer::lastSeenMsg(BufferId buffer) const
{
    return _lastSeenMsg.value(buffer, MsgId());
}


bool BufferSyncer::setLastSeenMsg(BufferId buffer, const MsgId &msgId)
{
    if (!msgId.isValid())
        return false;

    const MsgId oldLastSeenMsg = lastSeenMsg(buffer);
    if (!oldLastSeenMsg.isValid() || oldLastSeenMsg < msgId) {
        _lastSeenMsg[buffer] = msgId;
        SYNC(ARG(buffer), ARG(msgId))
        emit lastSeenMsgSet(buffer, msgId);
        return true;
    }
    return false;
}


MsgId BufferSyncer::markerLine(BufferId buffer) const
{
    return _markerLines.value(buffer, MsgId());
}


bool BufferSyncer::setMarkerLine(BufferId buffer, const MsgId &msgId)
{
    if (!msgId.isValid())
        return false;

    if (_markerLines.value(buffer) == msgId)
        return false;

    _markerLines[buffer] = msgId;
    SYNC(ARG(buffer), ARG(msgId))
    emit markerLineSet(buffer, msgId);
    return true;
}


QVariantList BufferSyncer::initLastSeenMsg() const
{
    QVariantList list;
    QHash<BufferId, MsgId>::const_iterator iter = _lastSeenMsg.constBegin();
    while (iter != _lastSeenMsg.constEnd()) {
        list << QVariant::fromValue<BufferId>(iter.key())
             << QVariant::fromValue<MsgId>(iter.value());
        ++iter;
    }
    return list;
}


void BufferSyncer::initSetLastSeenMsg(const QVariantList &list)
{
    _lastSeenMsg.clear();
    Q_ASSERT(list.count() % 2 == 0);
    for (int i = 0; i < list.count(); i += 2) {
        setLastSeenMsg(list.at(i).value<BufferId>(), list.at(i+1).value<MsgId>());
    }
}


QVariantList BufferSyncer::initMarkerLines() const
{
    QVariantList list;
    QHash<BufferId, MsgId>::const_iterator iter = _markerLines.constBegin();
    while (iter != _markerLines.constEnd()) {
        list << QVariant::fromValue<BufferId>(iter.key())
             << QVariant::fromValue<MsgId>(iter.value());
        ++iter;
    }
    return list;
}


void BufferSyncer::initSetMarkerLines(const QVariantList &list)
{
    _markerLines.clear();
    Q_ASSERT(list.count() % 2 == 0);
    for (int i = 0; i < list.count(); i += 2) {
        setMarkerLine(list.at(i).value<BufferId>(), list.at(i+1).value<MsgId>());
    }
}


QVariantList BufferSyncer::initActivities() const
{
    QVariantList list;
    auto iter = _bufferActivities.constBegin();
    while (iter != _bufferActivities.constEnd()) {
        list << QVariant::fromValue<BufferId>(iter.key())
             << QVariant::fromValue<int>((int) iter.value());
        ++iter;
    }
    return list;
}


void BufferSyncer::initSetActivities(const QVariantList &list)
{
    _bufferActivities.clear();
    Q_ASSERT(list.count() % 2 == 0);
    for (int i = 0; i < list.count(); i += 2) {
        setBufferActivity(list.at(i).value<BufferId>(), list.at(i+1).value<int>());
    }
}


Message::Types BufferSyncer::activity(BufferId buffer) const
{
    return _bufferActivities.value(buffer, Message::Types());
}


void BufferSyncer::removeBuffer(BufferId buffer)
{
    if (_lastSeenMsg.contains(buffer))
        _lastSeenMsg.remove(buffer);
    if (_markerLines.contains(buffer))
        _markerLines.remove(buffer);
    if (_bufferActivities.contains(buffer))
        _bufferActivities.remove(buffer);
    if (_highlightCounts.contains(buffer))
        _highlightCounts.remove(buffer);
    SYNC(ARG(buffer))
    emit bufferRemoved(buffer);
}


void BufferSyncer::mergeBuffersPermanently(BufferId buffer1, BufferId buffer2)
{
    if (_lastSeenMsg.contains(buffer2))
        _lastSeenMsg.remove(buffer2);
    if (_markerLines.contains(buffer2))
        _markerLines.remove(buffer2);
    if (_bufferActivities.contains(buffer2))
        _bufferActivities.remove(buffer2);
    if (_highlightCounts.contains(buffer2))
        _highlightCounts.remove(buffer2);
    SYNC(ARG(buffer1), ARG(buffer2))
    emit buffersPermanentlyMerged(buffer1, buffer2);
}

int BufferSyncer::highlightCount(BufferId buffer) const {
    return _highlightCounts.value(buffer, 0);
}

QVariantList BufferSyncer::initHighlightCounts() const {
    QVariantList list;
    auto iter = _highlightCounts.constBegin();
    while (iter != _highlightCounts.constEnd()) {
        list << QVariant::fromValue<BufferId>(iter.key())
             << QVariant::fromValue<int>((int) iter.value());
        ++iter;
    }
    return list;
}

void BufferSyncer::initSetHighlightCounts(const QVariantList &list) {
    _highlightCounts.clear();
    Q_ASSERT(list.count() % 2 == 0);
    for (int i = 0; i < list.count(); i += 2) {
        setHighlightCount(list.at(i).value<BufferId>(), list.at(i+1).value<int>());
    }
}
