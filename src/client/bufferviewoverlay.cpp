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

#include "bufferviewoverlay.h"

#include <QEvent>

#include "bufferviewconfig.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "clientbufferviewmanager.h"
#include "networkmodel.h"

const int BufferViewOverlay::_updateEventId = QEvent::registerEventType();

BufferViewOverlay::BufferViewOverlay(QObject *parent)
    : QObject(parent),
    _aboutToUpdate(false),
    _uninitializedViewCount(0),
    _allowedBufferTypes(0),
    _minimumActivity(0)
{
}


void BufferViewOverlay::reset()
{
    _aboutToUpdate = false;

    _bufferViewIds.clear();
    _uninitializedViewCount = 0;

    _networkIds.clear();
    _allowedBufferTypes = 0;
    _minimumActivity = 0;

    _buffers.clear();
    _removedBuffers.clear();
    _tempRemovedBuffers.clear();
}


void BufferViewOverlay::save()
{
    CoreAccountSettings().setBufferViewOverlay(_bufferViewIds);
}


void BufferViewOverlay::restore()
{
    QSet<int> currentIds = _bufferViewIds;
    reset();
    currentIds += CoreAccountSettings().bufferViewOverlay();

    QSet<int>::const_iterator iter;
    for (iter = currentIds.constBegin(); iter != currentIds.constEnd(); iter++) {
        addView(*iter);
    }
}


void BufferViewOverlay::addView(int viewId)
{
    if (_bufferViewIds.contains(viewId))
        return;

    BufferViewConfig *config = Client::bufferViewManager()->bufferViewConfig(viewId);
    if (!config) {
        qDebug() << "BufferViewOverlay::addView(): no such buffer view:" << viewId;
        return;
    }

    _bufferViewIds << viewId;
    bool wasInitialized = isInitialized();
    _uninitializedViewCount++;

    if (config->isInitialized()) {
        viewInitialized(config);

        if (wasInitialized) {
            BufferIdList buffers;
            if (config->networkId().isValid()) {
                foreach(BufferId bufferId, config->bufferList()) {
                    if (Client::networkModel()->networkId(bufferId) == config->networkId())
                        buffers << bufferId;
                }
                foreach(BufferId bufferId, config->temporarilyRemovedBuffers().toList()) {
                    if (Client::networkModel()->networkId(bufferId) == config->networkId())
                        buffers << bufferId;
                }
            }
            else {
                buffers = BufferIdList::fromSet(config->bufferList().toSet() + config->temporarilyRemovedBuffers());
            }
            Client::backlogManager()->checkForBacklog(buffers);
        }
    }
    else {
        disconnect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()));
        // we use a queued connection here since manipulating the connection list of a sending object
        // doesn't seem to be such a good idea while executing a connected slots.
        connect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()), Qt::QueuedConnection);
    }
    save();
}


void BufferViewOverlay::removeView(int viewId)
{
    if (!_bufferViewIds.contains(viewId))
        return;

    _bufferViewIds.remove(viewId);
    BufferViewConfig *config = Client::bufferViewManager()->bufferViewConfig(viewId);
    if (config)
        disconnect(config, 0, this, 0);

    // update initialized State:
    bool wasInitialized = isInitialized();
    _uninitializedViewCount = 0;
    QSet<int>::iterator viewIter = _bufferViewIds.begin();
    while (viewIter != _bufferViewIds.end()) {
        config = Client::bufferViewManager()->bufferViewConfig(*viewIter);
        if (!config) {
            viewIter = _bufferViewIds.erase(viewIter);
        }
        else {
            if (!config->isInitialized())
                _uninitializedViewCount++;
            viewIter++;
        }
    }

    update();
    if (!wasInitialized && isInitialized())
        emit initDone();
    save();
}


void BufferViewOverlay::viewInitialized(BufferViewConfig *config)
{
    if (!config) {
        qWarning() << "BufferViewOverlay::viewInitialized() received invalid view!";
        return;
    }
    disconnect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()));

    connect(config, SIGNAL(configChanged()), this, SLOT(update()));

    // check if the view was removed in the meantime...
    if (_bufferViewIds.contains(config->bufferViewId()))
        update();

    _uninitializedViewCount--;
    if (isInitialized())
        emit initDone();
}


void BufferViewOverlay::viewInitialized()
{
    BufferViewConfig *config = qobject_cast<BufferViewConfig *>(sender());
    Q_ASSERT(config);

    viewInitialized(config);
}


void BufferViewOverlay::update()
{
    if (_aboutToUpdate) {
        return;
    }
    _aboutToUpdate = true;
    QCoreApplication::postEvent(this, new QEvent((QEvent::Type)_updateEventId));
}


void BufferViewOverlay::updateHelper()
{
    if (!_aboutToUpdate)
        return;

    bool changed = false;

    int allowedBufferTypes = 0;
    int minimumActivity = -1;
    QSet<NetworkId> networkIds;
    QSet<BufferId> buffers;
    QSet<BufferId> removedBuffers;
    QSet<BufferId> tempRemovedBuffers;

    if (Client::bufferViewManager()) {
        BufferViewConfig *config = 0;
        QSet<int>::const_iterator viewIter;
        for (viewIter = _bufferViewIds.constBegin(); viewIter != _bufferViewIds.constEnd(); viewIter++) {
            config = Client::bufferViewManager()->bufferViewConfig(*viewIter);
            if (!config)
                continue;

            allowedBufferTypes |= config->allowedBufferTypes();
            if (minimumActivity == -1 || config->minimumActivity() < minimumActivity)
                minimumActivity = config->minimumActivity();

            networkIds << config->networkId();

            // we have to apply several filters before we can add a buffer to a category (visible, removed, ...)
            buffers += filterBuffersByConfig(config->bufferList(), config);
            tempRemovedBuffers += filterBuffersByConfig(config->temporarilyRemovedBuffers().toList(), config);
            removedBuffers += config->removedBuffers();
        }

        // prune the sets from overlap
        QSet<BufferId> availableBuffers = Client::networkModel()->allBufferIds().toSet();

        buffers.intersect(availableBuffers);

        tempRemovedBuffers.intersect(availableBuffers);
        tempRemovedBuffers.subtract(buffers);

        removedBuffers.intersect(availableBuffers);
        removedBuffers.subtract(tempRemovedBuffers);
        removedBuffers.subtract(buffers);
    }

    changed |= (allowedBufferTypes != _allowedBufferTypes);
    changed |= (minimumActivity != _minimumActivity);
    changed |= (networkIds != _networkIds);
    changed |= (buffers != _buffers);
    changed |= (removedBuffers != _removedBuffers);
    changed |= (tempRemovedBuffers != _tempRemovedBuffers);

    _allowedBufferTypes = allowedBufferTypes;
    _minimumActivity = minimumActivity;
    _networkIds = networkIds;
    _buffers = buffers;
    _removedBuffers = removedBuffers;
    _tempRemovedBuffers = tempRemovedBuffers;

    _aboutToUpdate = false;

    if (changed)
        emit hasChanged();
}


QSet<BufferId> BufferViewOverlay::filterBuffersByConfig(const QList<BufferId> &buffers, const BufferViewConfig *config)
{
    Q_ASSERT(config);

    QSet<BufferId> bufferIds;
    BufferInfo bufferInfo;
    foreach(BufferId bufferId, buffers) {
        bufferInfo = Client::networkModel()->bufferInfo(bufferId);
        if (!(bufferInfo.type() & config->allowedBufferTypes()))
            continue;
        if (config->networkId().isValid() && bufferInfo.networkId() != config->networkId())
            continue;
        bufferIds << bufferId;
    }

    return bufferIds;
}


void BufferViewOverlay::customEvent(QEvent *event)
{
    if (event->type() == _updateEventId) {
        updateHelper();
    }
}


bool BufferViewOverlay::allNetworks()
{
    updateHelper();
    return _networkIds.contains(NetworkId());
}


const QSet<NetworkId> &BufferViewOverlay::networkIds()
{
    updateHelper();
    return _networkIds;
}


const QSet<BufferId> &BufferViewOverlay::bufferIds()
{
    updateHelper();
    return _buffers;
}


const QSet<BufferId> &BufferViewOverlay::removedBufferIds()
{
    updateHelper();
    return _removedBuffers;
}


const QSet<BufferId> &BufferViewOverlay::tempRemovedBufferIds()
{
    updateHelper();
    return _tempRemovedBuffers;
}


int BufferViewOverlay::allowedBufferTypes()
{
    updateHelper();
    return _allowedBufferTypes;
}


int BufferViewOverlay::minimumActivity()
{
    updateHelper();
    return _minimumActivity;
}
