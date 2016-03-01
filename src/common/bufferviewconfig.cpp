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

#include "bufferviewconfig.h"

#include "bufferinfo.h"

INIT_SYNCABLE_OBJECT(BufferViewConfig)
BufferViewConfig::BufferViewConfig(int bufferViewId, QObject *parent)
    : SyncableObject(parent),
    _bufferViewId(bufferViewId),
    _addNewBuffersAutomatically(true),
    _sortAlphabetically(true),
    _hideInactiveBuffers(false),
    _hideInactiveNetworks(false),
    _disableDecoration(false),
    _allowedBufferTypes(BufferInfo::StatusBuffer | BufferInfo::ChannelBuffer | BufferInfo::QueryBuffer | BufferInfo::GroupBuffer),
    _minimumActivity(0),
    _showSearch(false)
{
    setObjectName(QString::number(bufferViewId));
}


BufferViewConfig::BufferViewConfig(int bufferViewId, const QVariantMap &properties, QObject *parent)
    : SyncableObject(parent),
    _bufferViewId(bufferViewId)
{
    fromVariantMap(properties);
    setObjectName(QString::number(bufferViewId));
}


void BufferViewConfig::setBufferViewName(const QString &bufferViewName)
{
    if (_bufferViewName == bufferViewName)
        return;

    _bufferViewName = bufferViewName;
    SYNC(ARG(bufferViewName))
    emit bufferViewNameSet(bufferViewName);
}


void BufferViewConfig::setNetworkId(const NetworkId &networkId)
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

void BufferViewConfig::setShowSearch(bool showSearch) {
    if (_showSearch == showSearch) {
        return;
    }

    _showSearch = showSearch;
    SYNC(ARG(showSearch))
    emit configChanged();
}


QVariantList BufferViewConfig::initBufferList() const
{
    QVariantList buffers;

    foreach(BufferId bufferId, _buffers) {
        buffers << qVariantFromValue(bufferId);
    }

    return buffers;
}


void BufferViewConfig::initSetBufferList(const QVariantList &buffers)
{
    _buffers.clear();

    foreach(QVariant buffer, buffers) {
        _buffers << buffer.value<BufferId>();
    }

    emit configChanged(); // used to track changes in the settingspage
}


void BufferViewConfig::initSetBufferList(const QList<BufferId> &buffers)
{
    _buffers.clear();

    foreach(BufferId bufferId, buffers) {
        _buffers << bufferId;
    }

    emit configChanged(); // used to track changes in the settingspage
}


QVariantList BufferViewConfig::initRemovedBuffers() const
{
    QVariantList removedBuffers;

    foreach(BufferId bufferId, _removedBuffers) {
        removedBuffers << qVariantFromValue(bufferId);
    }

    return removedBuffers;
}


void BufferViewConfig::initSetRemovedBuffers(const QVariantList &buffers)
{
    _removedBuffers.clear();

    foreach(QVariant buffer, buffers) {
        _removedBuffers << buffer.value<BufferId>();
    }
}


QVariantList BufferViewConfig::initTemporarilyRemovedBuffers() const
{
    QVariantList temporarilyRemovedBuffers;

    foreach(BufferId bufferId, _temporarilyRemovedBuffers) {
        temporarilyRemovedBuffers << qVariantFromValue(bufferId);
    }

    return temporarilyRemovedBuffers;
}


void BufferViewConfig::initSetTemporarilyRemovedBuffers(const QVariantList &buffers)
{
    _temporarilyRemovedBuffers.clear();

    foreach(QVariant buffer, buffers) {
        _temporarilyRemovedBuffers << buffer.value<BufferId>();
    }
}


void BufferViewConfig::addBuffer(const BufferId &bufferId, int pos)
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


void BufferViewConfig::moveBuffer(const BufferId &bufferId, int pos)
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


void BufferViewConfig::removeBuffer(const BufferId &bufferId)
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


void BufferViewConfig::removeBufferPermanently(const BufferId &bufferId)
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
