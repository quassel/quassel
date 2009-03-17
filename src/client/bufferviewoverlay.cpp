/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "bufferviewoverlay.h"

#include <QEvent>

#include "bufferviewconfig.h"
#include "client.h"
#include "clientbufferviewmanager.h"
#include "networkmodel.h"

const int BufferViewOverlay::_updateEventId = QEvent::registerEventType();

BufferViewOverlay::BufferViewOverlay(QObject *parent)
  : QObject(parent),
    _aboutToUpdate(false),
    _addBuffersAutomatically(false),
    _hideInactiveBuffers(false),
    _allowedBufferTypes(0),
    _minimumActivity(0)
{
}

void BufferViewOverlay::addView(int viewId) {
  if(_bufferViewIds.contains(viewId))
    return;

  BufferViewConfig *config = Client::bufferViewManager()->bufferViewConfig(viewId);
  if(!config) {
    qDebug() << "BufferViewOverlay::addView(): no such buffer view:" << viewId;
    return;
  }

  _bufferViewIds << viewId;
  if(config->isInitialized()) {
    viewInitialized(config);
  } else {
    disconnect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()));
    connect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()));
  }
}

void BufferViewOverlay::removeView(int viewId) {
  if(!_bufferViewIds.contains(viewId))
    return;

  _bufferViewIds.remove(viewId);
  BufferViewConfig *config = qobject_cast<BufferViewConfig *>(sender());
  if(config)
    disconnect(config, 0, this, 0);
  update();
}

void BufferViewOverlay::viewInitialized(BufferViewConfig *config) {
  if(!config) {
    qWarning() << "BufferViewOverlay::viewInitialized() received invalid view!";
    return;
  }
  disconnect(config, SIGNAL(initDone()), this, SLOT(viewInitialized()));

  connect(config, SIGNAL(networkIdSet(const NetworkId &)), this, SLOT(update()));
  connect(config, SIGNAL(addNewBuffersAutomaticallySet(bool)), this, SLOT(update()));
  connect(config, SIGNAL(sortAlphabeticallySet(bool)), this, SLOT(update()));
  connect(config, SIGNAL(hideInactiveBuffersSet(bool)), this, SLOT(update()));
  connect(config, SIGNAL(allowedBufferTypesSet(int)), this, SLOT(update()));
  connect(config, SIGNAL(minimumActivitySet(int)), this, SLOT(update()));
  connect(config, SIGNAL(bufferListSet()), this, SLOT(update()));
  connect(config, SIGNAL(bufferAdded(const BufferId &, int)), this, SLOT(update()));
  connect(config, SIGNAL(bufferRemoved(const BufferId &)), this, SLOT(update()));
  connect(config, SIGNAL(bufferPermanentlyRemoved(const BufferId &)), this, SLOT(update()));

  // check if the view was removed in the meantime...
  if(_bufferViewIds.contains(config->bufferViewId()))
    update();
}

void BufferViewOverlay::viewInitialized() {
  BufferViewConfig *config = qobject_cast<BufferViewConfig *>(sender());
  Q_ASSERT(config);

  viewInitialized(config);
}

void BufferViewOverlay::update() {
  if(_aboutToUpdate) {
    return;
  }
  _aboutToUpdate = true;
  QCoreApplication::postEvent(this, new QEvent((QEvent::Type)_updateEventId));
}

void BufferViewOverlay::updateHelper() {
  bool changed = false;

  bool addBuffersAutomatically = false;
  bool hideInactiveBuffers = true;
  int allowedBufferTypes = 0;
  int minimumActivity = -1;
  QSet<NetworkId> networkIds;
  QSet<BufferId> buffers;
  QSet<BufferId> removedBuffers;
  QSet<BufferId> tempRemovedBuffers;

  if(Client::bufferViewManager()) {
    BufferViewConfig *config = 0;
    QSet<int>::const_iterator viewIter;
    for(viewIter = _bufferViewIds.constBegin(); viewIter != _bufferViewIds.constEnd(); viewIter++) {
      config = Client::bufferViewManager()->bufferViewConfig(*viewIter);
      if(!config)
        continue;

      networkIds << config->networkId();
      if(config->networkId().isValid()) {
        NetworkId networkId = config->networkId();
        // we have to filter out all the buffers that don't belong to this net... :/
        QSet<BufferId> bufferIds;
        foreach(BufferId bufferId, config->bufferList()) {
          if(Client::networkModel()->networkId(bufferId) == networkId)
            bufferIds << bufferId;
        }
        buffers += bufferIds;

        bufferIds.clear();
        foreach(BufferId bufferId, config->temporarilyRemovedBuffers()) {
          if(Client::networkModel()->networkId(bufferId) == networkId)
            bufferIds << bufferId;
        }
        tempRemovedBuffers += bufferIds;
      } else {
        buffers += config->bufferList().toSet();
        tempRemovedBuffers += config->temporarilyRemovedBuffers();
      }

      // in the overlay a buffer is removed it is removed from all views
      if(removedBuffers.isEmpty())
        removedBuffers = config->removedBuffers();
      else
        removedBuffers.intersect(config->removedBuffers());


      addBuffersAutomatically |= config->addNewBuffersAutomatically();
      hideInactiveBuffers &= config->hideInactiveBuffers();
      allowedBufferTypes |= config->allowedBufferTypes();
      if(minimumActivity == -1 || config->minimumActivity() < minimumActivity)
        minimumActivity = config->minimumActivity();
    }
  }

  changed |= (addBuffersAutomatically != _addBuffersAutomatically);
  changed |= (hideInactiveBuffers != _hideInactiveBuffers);
  changed |= (allowedBufferTypes != _allowedBufferTypes);
  changed |= (minimumActivity != _minimumActivity);
  changed |= (networkIds != _networkIds);
  changed |= (buffers != _buffers);
  changed |= (removedBuffers != _removedBuffers);
  changed |= (tempRemovedBuffers != _tempRemovedBuffers);

  _addBuffersAutomatically = addBuffersAutomatically;
  _hideInactiveBuffers = hideInactiveBuffers;
  _allowedBufferTypes = allowedBufferTypes;
  _minimumActivity = minimumActivity;
  _networkIds = networkIds;
  _buffers = buffers;
  _removedBuffers = removedBuffers;
  _tempRemovedBuffers = tempRemovedBuffers;

  if(changed)
    emit hasChanged();
}

void BufferViewOverlay::customEvent(QEvent *event) {
  if(event->type() == _updateEventId) {
    updateHelper();
    _aboutToUpdate = false;
  }
}
