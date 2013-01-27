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

#include "clientbufferviewconfig.h"

INIT_SYNCABLE_OBJECT(ClientBufferViewConfig)
ClientBufferViewConfig::ClientBufferViewConfig(int bufferViewId, QObject *parent)
    : BufferViewConfig(bufferViewId, parent),
    _locked(false)
{
    connect(this, SIGNAL(initDone()), this, SLOT(ensureDecoration()));
}


// currently we don't have a possibility to configure disableDecoration
// if we have an old config this value can be true which is... bad.
// so we upgrade the core stored bufferViewConfig.
// This can be removed with the next release
void ClientBufferViewConfig::ensureDecoration()
{
    if (!disableDecoration())
        return;
    setDisableDecoration(false);
    requestUpdate(toVariantMap());
}
