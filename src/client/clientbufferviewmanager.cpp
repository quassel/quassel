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

#include "clientbufferviewmanager.h"

#include "clientbufferviewconfig.h"
#include "client.h"
#include "networkmodel.h"

INIT_SYNCABLE_OBJECT(ClientBufferViewManager)
ClientBufferViewManager::ClientBufferViewManager(SignalProxy *proxy, QObject *parent)
    : BufferViewManager(proxy, parent)
{
}


BufferViewConfig *ClientBufferViewManager::bufferViewConfigFactory(int bufferViewConfigId)
{
    return new ClientBufferViewConfig(bufferViewConfigId, this);
}


QList<ClientBufferViewConfig *> ClientBufferViewManager::clientBufferViewConfigs() const
{
    QList<ClientBufferViewConfig *> clientConfigs;
    foreach(BufferViewConfig *config, bufferViewConfigs()) {
        clientConfigs << static_cast<ClientBufferViewConfig *>(config);
    }
    return clientConfigs;
}


ClientBufferViewConfig *ClientBufferViewManager::clientBufferViewConfig(int bufferViewId) const
{
    return static_cast<ClientBufferViewConfig *>(bufferViewConfig(bufferViewId));
}


void ClientBufferViewManager::setInitialized()
{
    if (bufferViewConfigs().isEmpty()) {
        BufferViewConfig config(-1);
        config.setBufferViewName(tr("All Chats"));
        config.initSetBufferList(Client::networkModel()->allBufferIdsSorted());
        requestCreateBufferView(config.toVariantMap());
    }
    BufferViewManager::setInitialized();
}
