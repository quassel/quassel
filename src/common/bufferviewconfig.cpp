/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

QVariantList BufferViewConfig::initBufferList() const
{
    QVariantList buffers;

    foreach (BufferId bufferId, _buffers) {
        buffers << QVariant::fromValue(bufferId);
    }

    return buffers;
}

void BufferViewConfig::initSetBufferList(const QVariantList& buffers)
{
    _buffers.clear();

    foreach (QVariant buffer, buffers) {
        _buffers << buffer.value<BufferId>();
    }

    emit configChanged();  // used to track changes in the settingspage
}

QVariantList BufferViewConfig::initRemovedBuffers() const
{
    QVariantList removedBuffers;

    foreach (BufferId bufferId, _removedBuffers) {
        removedBuffers << QVariant::fromValue(bufferId);
    }

    return removedBuffers;
}

void BufferViewConfig::initSetRemovedBuffers(const QVariantList& buffers)
{
    _removedBuffers.clear();

    foreach (QVariant buffer, buffers) {
        _removedBuffers << buffer.value<BufferId>();
    }
}

QVariantList BufferViewConfig::initTemporarilyRemovedBuffers() const
{
    QVariantList temporarilyRemovedBuffers;

    foreach (BufferId bufferId, _temporarilyRemovedBuffers) {
        temporarilyRemovedBuffers << QVariant::fromValue(bufferId);
    }

    return temporarilyRemovedBuffers;
}

void BufferViewConfig::initSetTemporarilyRemovedBuffers(const QVariantList& buffers)
{
    _temporarilyRemovedBuffers.clear();

    foreach (QVariant buffer, buffers) {
        _temporarilyRemovedBuffers << buffer.value<BufferId>();
    }
}

void BufferViewConfig::setBufferViewName(const QString& bufferViewName)
{
    if (_bufferViewName == bufferViewName)
        return;

    _bufferViewName = bufferViewName;
    SYNC(ARG(bufferViewName))
    emit bufferViewNameSet(bufferViewName);
}

void BufferViewConfig::setNetworkId(const NetworkId& networkId)
{
    if (_networkId == networkId)
        return;

    _networkId = networkId;
    SYNC(ARG(networkId))
    emit networkIdSet(networkId);
    emit configChanged();
}

void BufferViewConfig::setAddNewBuffersAutomatically(bool addNewBuffersAutomatically)
{
    if (_addNewBuffersAutomatically == addNewBuffersAutomatically)
        return;

    _addNewBuffersAutomatically = addNewBuffersAutomatically;
    SYNC(ARG(addNewBuffersAutomatically))
    emit configChanged();
}

void BufferViewConfig::setSortAlphabetically(bool sortAlphabetically)
{
    if (_sortAlphabetically == sortAlphabetically)
        return;

    _sortAlphabetically = sortAlphabetically;
    SYNC(ARG(sortAlphabetically))
    emit configChanged();
}

void BufferViewConfig::setDisableDecoration(bool disableDecoration)
{
    if (_disableDecoration == disableDecoration)
        return;

    _disableDecoration = disableDecoration;
    SYNC(ARG(disableDecoration))
}

void BufferViewConfig::setAllowedBufferTypes(int bufferTypes)
{
    if (_allowedBufferTypes == bufferTypes)
        return;

    _allowedBufferTypes = bufferTypes;
    SYNC(ARG(bufferTypes))
    emit configChanged();
}

void BufferViewConfig::setMinimumActivity(int activity)
{
    if (_minimumActivity == activity)
        return;

    _minimumActivity = activity;
    SYNC(ARG(activity))
    emit configChanged();
}

void BufferViewConfig::setHideInactiveBuffers(bool hideInactiveBuffers)
{
    if (_hideInactiveBuffers == hideInactiveBuffers)
        return;

    _hideInactiveBuffers = hideInactiveBuffers;
    SYNC(ARG(hideInactiveBuffers))
    emit configChanged();
}

void BufferViewConfig::setHideInactiveNetworks(bool hideInactiveNetworks)
{
    if (_hideInactiveNetworks == hideInactiveNetworks)
        return;

    _hideInactiveNetworks = hideInactiveNetworks;
    SYNC(ARG(hideInactiveNetworks))
    emit configChanged();
}

void BufferViewConfig::setShowSearch(bool showSearch)
{
    if (_showSearch == showSearch) {
        return;
    }

    _showSearch = showSearch;
    SYNC(ARG(showSearch))
    emit configChanged();
}

void BufferViewConfig::setBufferList(const QList<BufferId>& buffers)
{
    _buffers = buffers;
    emit configChanged();
}

void BufferViewConfig::addBuffer(const BufferId& bufferId, int pos)
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

void BufferViewConfig::moveBuffer(const BufferId& bufferId, int pos)
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

void BufferViewConfig::removeBuffer(const BufferId& bufferId)
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

void BufferViewConfig::removeBufferPermanently(const BufferId& bufferId)
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

void BufferViewConfig::requestAddBuffer(const BufferId& bufferId, int pos)
{
    REQUEST(ARG(bufferId), ARG(pos))
}

void BufferViewConfig::requestMoveBuffer(const BufferId& bufferId, int pos)
{
    REQUEST(ARG(bufferId), ARG(pos))
}

void BufferViewConfig::requestRemoveBuffer(const BufferId& bufferId)
{
    REQUEST(ARG(bufferId))
}

void BufferViewConfig::requestRemoveBufferPermanently(const BufferId& bufferId)
{
    REQUEST(ARG(bufferId))
}
