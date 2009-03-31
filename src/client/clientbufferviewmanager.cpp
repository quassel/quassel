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

#include "clientbufferviewmanager.h"

#include "clientbufferviewconfig.h"

ClientBufferViewManager::ClientBufferViewManager(SignalProxy *proxy, QObject *parent)
  : BufferViewManager(proxy, parent)
{
  connect(this, SIGNAL(initDone()), this, SLOT(waitForConfigInit()));
}

BufferViewConfig *ClientBufferViewManager::bufferViewConfigFactory(int bufferViewConfigId) {
  return new ClientBufferViewConfig(bufferViewConfigId, this);
}

QList<ClientBufferViewConfig *> ClientBufferViewManager::clientBufferViewConfigs() const {
  QList<ClientBufferViewConfig *> clientConfigs;
  foreach(BufferViewConfig *config, bufferViewConfigs()) {
    clientConfigs << static_cast<ClientBufferViewConfig *>(config);
  }
  return clientConfigs;
}

ClientBufferViewConfig *ClientBufferViewManager::clientBufferViewConfig(int bufferViewId) const {
  return static_cast<ClientBufferViewConfig *>(bufferViewConfig(bufferViewId));
}

void ClientBufferViewManager::waitForConfigInit() {
  bool initialized = true;
  foreach(BufferViewConfig *config, bufferViewConfigs()) {
    initialized &= config->isInitialized();
    if(config->isInitialized())
      continue;
    connect(config, SIGNAL(initDone()), this, SLOT(configInitBarrier()));
  }
  if(initialized)
    QMetaObject::invokeMethod(this, "viewsInitialized", Qt::QueuedConnection);
}

void ClientBufferViewManager::configInitBarrier() {
  BufferViewConfig *config = qobject_cast<BufferViewConfig *>(sender());
  Q_ASSERT(config);
  disconnect(config, SIGNAL(initDone()), this, SLOT(configInitBarrier()));

  bool initialized = true;
  foreach(BufferViewConfig *config, bufferViewConfigs()) {
    initialized &= config->isInitialized();
  }
  if(initialized)
    QMetaObject::invokeMethod(this, "viewsInitialized", Qt::QueuedConnection);
}
