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

#include <algorithm>
#include <iterator>

#include "bufferviewconfig.h"

BufferViewConfig::BufferViewConfig(int bufferViewId, QObject* parent)
    : SyncableObject(parent)
    , _bufferViewId(bufferViewId)
{
    setObjectName(QString::number(bufferViewId));
}

BufferViewConfig::BufferViewConfig(int bufferViewId, const QVariantMap& properties, QObject* parent)
    : SyncableObject(parent)
    , _bufferViewId(bufferViewId)
{
    fromVariantMap(properties);
    setObjectName(QString::number(bufferViewId));
}

int BufferViewConfig::bufferViewId() const
{
    return _bufferViewId;
}

QString BufferViewConfig::bufferViewName() const
{
    return _bufferViewName;
}

NetworkId BufferViewConfig::networkId() const
{
    return _networkId;
}

bool BufferViewConfig::addNewBuffersAutomatically() const
{
    return _addNewBuffersAutomatically;
}

bool BufferViewConfig::sortAlphabetically() const
{
    return _sortAlphabetically;
}

bool BufferViewConfig::disableDecoration() const
{
    return _disableDecoration;
}

int BufferViewConfig::allowedBufferTypes() const
{
    return _allowedBufferTypes;
}

int BufferViewConfig::minimumActivity() const
{
    return _minimumActivity;
}

bool BufferViewConfig::hideInactiveBuffers() const
{
    return _hideInactiveBuffers;
}

bool BufferViewConfig::hideInactiveNetworks() const
{
    return _hideInactiveNetworks;
}

bool BufferViewConfig::showSearch() const
{
    return _showSearch;
}

QList<BufferId> BufferViewConfig::bufferList() const
{
    return _buffers;
}

QSet<BufferId> BufferViewConfig::removedBuffers() const
{
    return _removedBuffers;
}

QSet<BufferId> BufferViewConfig::temporarilyRemovedBuffers() const
{
    return _temporarilyRemovedBuffers;
}

QVariantList BufferViewConfig::buffersToList() const
{
    QVariantList result;
    std::transform(_buffers.cbegin(), _buffers.cend(), std::back_inserter(result), [](auto bufferId) {
        return QVariant::fromValue(bufferId);
    });
    return result;
}

void BufferViewConfig::buffersFromList(const QVariantList& buffers)
{
    _buffers.clear();
    std::transform(buffers.cbegin(), buffers.cend(), std::back_inserter(_buffers), [](const QVariant& v) {
        return v.value<BufferId>();
    });

    emit configChanged();  // used to track changes in the settingspage
}

QVariantList BufferViewConfig::removedBuffersToList() const
{
    QVariantList result;
    std::transform(_removedBuffers.cbegin(), _removedBuffers.cend(), std::back_inserter(result), [](auto bufferId) {
        return QVariant::fromValue(bufferId);
    });
    return result;
}

void BufferViewConfig::removedBuffersFromList(const QVariantList& buffers)
{
    _removedBuffers.clear();
    for (auto&& v : buffers) {
        _removedBuffers.insert(v.value<BufferId>());
    };
}

QVariantList BufferViewConfig::tempRemovedBuffersToList() const
{
    QVariantList result;
    std::transform(_temporarilyRemovedBuffers.cbegin(), _temporarilyRemovedBuffers.cend(), std::back_inserter(result), [](auto bufferId) {
        return QVariant::fromValue(bufferId);
    });
    return result;
}

void BufferViewConfig::tempRemovedBuffersFromList(const QVariantList& buffers)
{
    _temporarilyRemovedBuffers.clear();
    for (auto&& v : buffers) {
        _temporarilyRemovedBuffers.insert(v.value<BufferId>());
    }
}

void BufferViewConfig::setBufferViewName(const QString& bufferViewName)
{
    if (_bufferViewName != bufferViewName) {
        _bufferViewName = bufferViewName;
        emit bufferViewNameSet(bufferViewName);
    }
}

void BufferViewConfig::setNetworkId(NetworkId networkId)
{
    if (_networkId != networkId) {
        _networkId = networkId;
        emit networkIdSet(networkId);
        emit configChanged();
    }
}

void BufferViewConfig::setAddNewBuffersAutomatically(bool addNewBuffersAutomatically)
{
    if (_addNewBuffersAutomatically != addNewBuffersAutomatically) {
        _addNewBuffersAutomatically = addNewBuffersAutomatically;
        emit addNewBuffersAutomaticallySet(addNewBuffersAutomatically);
        emit configChanged();
    }
}

void BufferViewConfig::setSortAlphabetically(bool sortAlphabetically)
{
    if (_sortAlphabetically != sortAlphabetically) {
        _sortAlphabetically = sortAlphabetically;
        emit sortAlphabeticallySet(sortAlphabetically);
        emit configChanged();
    }
}

void BufferViewConfig::setDisableDecoration(bool disableDecoration)
{
    if (_disableDecoration != disableDecoration) {
        _disableDecoration = disableDecoration;
        emit disableDecorationSet(disableDecoration);
    }
}

void BufferViewConfig::setAllowedBufferTypes(int bufferTypes)
{
    if (_allowedBufferTypes != bufferTypes) {
        _allowedBufferTypes = bufferTypes;
        emit allowedBufferTypesSet(bufferTypes);
        emit configChanged();
    }
}

void BufferViewConfig::setMinimumActivity(int activity)
{
    if (_minimumActivity != activity) {
        _minimumActivity = activity;
        emit minimumActivitySet(activity);
        emit configChanged();
    }
}

void BufferViewConfig::setHideInactiveBuffers(bool hideInactiveBuffers)
{
    if (_hideInactiveBuffers != hideInactiveBuffers) {
        _hideInactiveBuffers = hideInactiveBuffers;
        emit hideInactiveBuffersSet(hideInactiveBuffers);
        emit configChanged();
    }
}

void BufferViewConfig::setHideInactiveNetworks(bool hideInactiveNetworks)
{
    if (_hideInactiveNetworks != hideInactiveNetworks) {
        _hideInactiveNetworks = hideInactiveNetworks;
        emit hideInactiveNetworksSet(hideInactiveNetworks);
        emit configChanged();
    }
}

void BufferViewConfig::setShowSearch(bool showSearch)
{
    if (_showSearch != showSearch) {
        _showSearch = showSearch;
        emit showSearchSet(showSearch);
        emit configChanged();
    }
}

void BufferViewConfig::setBufferList(const QList<BufferId>& buffers)
{
    _buffers = buffers;
    emit configChanged();
}

void BufferViewConfig::addBuffer(BufferId bufferId, int pos)
{
    if (_buffers.contains(bufferId))
        return;

    if (pos < 0)
        pos = 0;
    if (pos > _buffers.count())
        pos = _buffers.count();

    if (_removedBuffers.contains(bufferId))
        _removedBuffers.remove(bufferId);

    if (_temporarilyRemovedBuffers.contains(bufferId))
        _temporarilyRemovedBuffers.remove(bufferId);

    _buffers.insert(pos, bufferId);
    SYNC(ARG(bufferId), ARG(pos))
    emit bufferAdded(bufferId, pos);
    emit configChanged();
}

void BufferViewConfig::moveBuffer(BufferId bufferId, int pos)
{
    if (!_buffers.contains(bufferId))
        return;

    if (pos < 0)
        pos = 0;
    if (pos >= _buffers.count())
        pos = _buffers.count() - 1;

    _buffers.move(_buffers.indexOf(bufferId), pos);
    SYNC(ARG(bufferId), ARG(pos))
    emit bufferMoved(bufferId, pos);
    emit configChanged();
}

void BufferViewConfig::removeBuffer(BufferId bufferId)
{
    if (_buffers.contains(bufferId))
        _buffers.removeAt(_buffers.indexOf(bufferId));

    if (_removedBuffers.contains(bufferId))
        _removedBuffers.remove(bufferId);

    _temporarilyRemovedBuffers << bufferId;
    SYNC(ARG(bufferId))
    emit bufferRemoved(bufferId);
    emit configChanged();
}

void BufferViewConfig::removeBufferPermanently(BufferId bufferId)
{
    if (_buffers.contains(bufferId))
        _buffers.removeAt(_buffers.indexOf(bufferId));

    if (_temporarilyRemovedBuffers.contains(bufferId))
        _temporarilyRemovedBuffers.remove(bufferId);

    _removedBuffers << bufferId;

    SYNC(ARG(bufferId))
    emit bufferPermanentlyRemoved(bufferId);
    emit configChanged();
}

void BufferViewConfig::requestSetBufferViewName(const QString& bufferViewName)
{
    REQUEST(ARG(bufferViewName))
}

void BufferViewConfig::requestAddBuffer(BufferId bufferId, int pos)
{
    REQUEST(ARG(bufferId), ARG(pos))
}

void BufferViewConfig::requestMoveBuffer(BufferId bufferId, int pos)
{
    REQUEST(ARG(bufferId), ARG(pos))
}

void BufferViewConfig::requestRemoveBuffer(BufferId bufferId)
{
    REQUEST(ARG(bufferId))
}

void BufferViewConfig::requestRemoveBufferPermanently(BufferId bufferId)
{
    REQUEST(ARG(bufferId))
}
