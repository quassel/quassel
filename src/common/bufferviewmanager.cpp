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

#include "bufferviewmanager.h"

#include "bufferviewconfig.h"
#include "signalproxy.h"

BufferViewManager::BufferViewManager(SignalProxy *proxy, QObject *parent)
    : SyncableObject(parent),
    _proxy(proxy)
{
    _proxy->synchronize(this);
}


BufferViewConfig *BufferViewManager::bufferViewConfig(int bufferViewId) const
{
    if (_bufferViewConfigs.contains(bufferViewId))
        return _bufferViewConfigs[bufferViewId];
    else
        return 0;
}


BufferViewConfig *BufferViewManager::bufferViewConfigFactory(int bufferViewConfigId)
{
    return new BufferViewConfig(bufferViewConfigId, this);
}


void BufferViewManager::addBufferViewConfig(int bufferViewConfigId)
{
    if (_bufferViewConfigs.contains(bufferViewConfigId)) {
        return;
    }

    addBufferViewConfig(bufferViewConfigFactory(bufferViewConfigId));
}


void BufferViewManager::addBufferViewConfig(BufferViewConfig *config)
{
    if (_bufferViewConfigs.contains(config->bufferViewId())) {
        delete config;
        return;
    }

    _proxy->synchronize(config);
    _bufferViewConfigs[config->bufferViewId()] = config;
    int bufferViewId = config->bufferViewId();
    SYNC_OTHER(addBufferViewConfig, ARG(bufferViewId))
    emit bufferViewConfigAdded(bufferViewId);
}


void BufferViewManager::deleteBufferViewConfig(int bufferViewConfigId)
{
    if (!_bufferViewConfigs.contains(bufferViewConfigId))
        return;

    _bufferViewConfigs[bufferViewConfigId]->deleteLater();
    _bufferViewConfigs.remove(bufferViewConfigId);
    SYNC(ARG(bufferViewConfigId))
    emit bufferViewConfigDeleted(bufferViewConfigId);
}


QVariantList BufferViewManager::initBufferViewIds() const
{
    QVariantList bufferViewIds;
    BufferViewConfigHash::const_iterator iter = _bufferViewConfigs.constBegin();
    BufferViewConfigHash::const_iterator iterEnd = _bufferViewConfigs.constEnd();
    while (iter != iterEnd) {
        bufferViewIds << (*iter)->bufferViewId();
        ++iter;
    }
    return bufferViewIds;
}


void BufferViewManager::initSetBufferViewIds(const QVariantList bufferViewIds)
{
    for (auto &&id : bufferViewIds) {
        addBufferViewConfig(id.value<int>());
    }
}
