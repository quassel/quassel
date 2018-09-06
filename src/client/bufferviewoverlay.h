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

#include "client-export.h"

#include <QObject>

#include "types.h"

class BufferViewConfig;
class ClientBufferViewConfig;

class CLIENT_EXPORT BufferViewOverlay : public QObject
{
    Q_OBJECT

public:
    BufferViewOverlay(QObject *parent = nullptr);

    inline const QSet<int> &bufferViewIds() { return _bufferViewIds; }
    bool allNetworks();

    const QSet<NetworkId> &networkIds();
    const QSet<BufferId> &bufferIds();
    const QSet<BufferId> &removedBufferIds();
    const QSet<BufferId> &tempRemovedBufferIds();

    int allowedBufferTypes();
    int minimumActivity();

    inline bool isInitialized() { return _uninitializedViewCount == 0; }

public slots:
    void addView(int viewId);
    void removeView(int viewId);

    void reset();
    void save();
    void restore();

    // updates propagated from the actual views
    void update();

signals:
    void hasChanged();
    void initDone();

protected:
    void customEvent(QEvent *event) override;

private slots:
    void viewInitialized();
    void viewInitialized(BufferViewConfig *config);

private:
    void updateHelper();
    QSet<BufferId> filterBuffersByConfig(const QList<BufferId> &buffers, const BufferViewConfig *config);

    bool _aboutToUpdate{false};

    QSet<int> _bufferViewIds;
    int _uninitializedViewCount{0};

    QSet<NetworkId> _networkIds;
    int _allowedBufferTypes{0};
    int _minimumActivity{0};

    QSet<BufferId> _buffers;
    QSet<BufferId> _removedBuffers;
    QSet<BufferId> _tempRemovedBuffers;

    static const int _updateEventId;
};
