/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "corebuffersyncer.h"

#include "core.h"
#include "coresession.h"
#include "corenetwork.h"
#include "ircchannel.h"

class PurgeEvent : public QEvent
{
public:
    PurgeEvent() : QEvent(QEvent::User) {}
};


INIT_SYNCABLE_OBJECT(CoreBufferSyncer)
CoreBufferSyncer::CoreBufferSyncer(CoreSession *parent)
    : BufferSyncer(Core::bufferLastSeenMsgIds(parent->user()), Core::bufferMarkerLineMsgIds(parent->user()), parent),
    _coreSession(parent),
    _purgeBuffers(false)
{
}


void CoreBufferSyncer::requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId)
{
    if (setLastSeenMsg(buffer, msgId))
        dirtyLastSeenBuffers << buffer;
}


void CoreBufferSyncer::requestSetMarkerLine(BufferId buffer, const MsgId &msgId)
{
    if (setMarkerLine(buffer, msgId))
        dirtyMarkerLineBuffers << buffer;
}


void CoreBufferSyncer::storeDirtyIds()
{
    UserId userId = _coreSession->user();
    MsgId msgId;
    foreach(BufferId bufferId, dirtyLastSeenBuffers) {
        msgId = lastSeenMsg(bufferId);
        if (msgId.isValid())
            Core::setBufferLastSeenMsg(userId, bufferId, msgId);
    }

    foreach(BufferId bufferId, dirtyMarkerLineBuffers) {
        msgId = markerLine(bufferId);
        if (msgId.isValid())
            Core::setBufferMarkerLineMsg(userId, bufferId, msgId);
    }

    dirtyLastSeenBuffers.clear();
    dirtyMarkerLineBuffers.clear();
}


void CoreBufferSyncer::removeBuffer(BufferId bufferId)
{
    BufferInfo bufferInfo = Core::getBufferInfo(_coreSession->user(), bufferId);
    if (!bufferInfo.isValid()) {
        qWarning() << "CoreBufferSyncer::removeBuffer(): invalid BufferId:" << bufferId << "for User:" << _coreSession->user();
        return;
    }

    if (bufferInfo.type() == BufferInfo::StatusBuffer) {
        qWarning() << "CoreBufferSyncer::removeBuffer(): Status Buffers cannot be removed!";
        return;
    }

    if (bufferInfo.type() == BufferInfo::ChannelBuffer) {
        CoreNetwork *net = _coreSession->network(bufferInfo.networkId());
        if (!net) {
            qWarning() << "CoreBufferSyncer::removeBuffer(): Received BufferInfo with unknown networkId!";
            return;
        }
        IrcChannel *chan = net->ircChannel(bufferInfo.bufferName());
        if (chan) {
            qWarning() << "CoreBufferSyncer::removeBuffer(): Unable to remove Buffer for joined Channel:" << bufferInfo.bufferName();
            return;
        }
    }
    if (Core::removeBuffer(_coreSession->user(), bufferId))
        BufferSyncer::removeBuffer(bufferId);
}


void CoreBufferSyncer::renameBuffer(BufferId bufferId, QString newName)
{
    BufferInfo bufferInfo = Core::getBufferInfo(_coreSession->user(), bufferId);
    if (!bufferInfo.isValid()) {
        qWarning() << "CoreBufferSyncer::renameBuffer(): invalid BufferId:" << bufferId << "for User:" << _coreSession->user();
        return;
    }

    if (bufferInfo.type() != BufferInfo::QueryBuffer) {
        qWarning() << "CoreBufferSyncer::renameBuffer(): only QueryBuffers can be renamed" << bufferId;
        return;
    }

    if (Core::renameBuffer(_coreSession->user(), bufferId, newName))
        BufferSyncer::renameBuffer(bufferId, newName);
}


void CoreBufferSyncer::mergeBuffersPermanently(BufferId bufferId1, BufferId bufferId2)
{
    BufferInfo bufferInfo1 = Core::getBufferInfo(_coreSession->user(), bufferId1);
    BufferInfo bufferInfo2 = Core::getBufferInfo(_coreSession->user(), bufferId2);
    if (!bufferInfo1.isValid() || !bufferInfo2.isValid()) {
        qWarning() << "CoreBufferSyncer::mergeBuffersPermanently(): invalid BufferIds:" << bufferId1 << bufferId2 << "for User:" << _coreSession->user();
        return;
    }

    if((bufferInfo1.type() != BufferInfo::QueryBuffer && bufferInfo1.type() != BufferInfo::ChannelBuffer) || (bufferInfo2.type() != BufferInfo::QueryBuffer && bufferInfo2.type() != BufferInfo::ChannelBuffer)) {
        qWarning() << "CoreBufferSyncer::mergeBuffersPermanently(): only QueryBuffers and/or ChannelBuffers can be merged!" << bufferId1 << bufferId2;
        return;
    }

    if (Core::mergeBuffersPermanently(_coreSession->user(), bufferId1, bufferId2)) {
        BufferSyncer::mergeBuffersPermanently(bufferId1, bufferId2);
    }
}


void CoreBufferSyncer::customEvent(QEvent *event)
{
    if (event->type() != QEvent::User)
        return;

    purgeBufferIds();
    event->accept();
}


void CoreBufferSyncer::requestPurgeBufferIds()
{
    if (_purgeBuffers)
        return;

    _purgeBuffers = true;
    QCoreApplication::postEvent(this, new PurgeEvent());
}


void CoreBufferSyncer::purgeBufferIds()
{
    _purgeBuffers = false;
    QList<BufferInfo> bufferInfos = Core::requestBuffers(_coreSession->user());
    QSet<BufferId> actualBuffers;
    foreach(BufferInfo bufferInfo, bufferInfos) {
        actualBuffers << bufferInfo.bufferId();
    }

    QSet<BufferId> storedIds = lastSeenBufferIds().toSet() + markerLineBufferIds().toSet();
    foreach(BufferId bufferId, storedIds) {
        if (!actualBuffers.contains(bufferId)) {
            BufferSyncer::removeBuffer(bufferId);
        }
    }
}
