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

#ifndef CLIENTBUFFERVIEWMANAGER_H
#define CLIENTBUFFERVIEWMANAGER_H

#include "bufferviewmanager.h"

class ClientBufferViewConfig;
class BufferViewOverlay;

class ClientBufferViewManager : public BufferViewManager {
  Q_OBJECT

public:
  ClientBufferViewManager(SignalProxy *proxy, QObject *parent = 0);

  QList<ClientBufferViewConfig *> clientBufferViewConfigs() const;
  ClientBufferViewConfig *clientBufferViewConfig(int bufferViewId) const;

protected:
  virtual BufferViewConfig *bufferViewConfigFactory(int bufferViewConfigId);
};

#endif //CLIENTBUFFERVIEWMANAGER_H
