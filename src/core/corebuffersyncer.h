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

#ifndef COREBUFFERSYNCER_H
#define COREBUFFERSYNCER_H

#include "buffersyncer.h"

class CoreSession;

class CoreBufferSyncer : public BufferSyncer
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    explicit CoreBufferSyncer(CoreSession *parent);

public slots:
    void requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId) override;
    void requestSetMarkerLine(BufferId buffer, const MsgId &msgId) override;

    inline void requestRemoveBuffer(BufferId buffer) override { removeBuffer(buffer); }
    void removeBuffer(BufferId bufferId) override;

    void addBufferActivity(const Message &message) {
        auto oldActivity = activity(message.bufferId());
        if (!oldActivity.testFlag(message.type())) {
            setBufferActivity(message.bufferId(), (int) (oldActivity | message.type()));
        }
    }

    void addCoreHighlight(const Message &message) {
        auto oldHighlightCount = highlightCount(message.bufferId());
        if (message.flags().testFlag(Message::Flag::Highlight) && !message.flags().testFlag(Message::Flag::Self)) {
            setHighlightCount(message.bufferId(), oldHighlightCount + 1);
        }
    }

    void setBufferActivity(BufferId buffer, int activity) override;

    void setHighlightCount(BufferId buffer, int highlightCount) override;

    inline void requestRenameBuffer(BufferId buffer, QString newName) override { renameBuffer(buffer, newName); }
    void renameBuffer(BufferId buffer, QString newName) override;

    inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) override { mergeBuffersPermanently(buffer1, buffer2); }
    void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2) override;

    void requestPurgeBufferIds() override;

    inline void requestMarkBufferAsRead(BufferId buffer) override {
        int activity = Message::Types();
        setBufferActivity(buffer, activity);
        setHighlightCount(buffer, 0);
        markBufferAsRead(buffer);
    }

    void storeDirtyIds();

protected:
    void customEvent(QEvent *event) override;

private:
    CoreSession *_coreSession;
    bool _purgeBuffers;

    QSet<BufferId> dirtyLastSeenBuffers;
    QSet<BufferId> dirtyMarkerLineBuffers;
    QSet<BufferId> dirtyActivities;
    QSet<BufferId> dirtyHighlights;

    void purgeBufferIds();
};


#endif //COREBUFFERSYNCER_H
