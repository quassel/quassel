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

#include "corebufferviewmanager.h"

#include "corebufferviewconfig.h"

CoreBufferViewManager::CoreBufferViewManager(SignalProxy *proxy, QObject *parent)
  : BufferViewManager(proxy, parent)
{
  return;
  // fill in some demo views
  CoreBufferViewConfig *config = 0;
  for(int i = 0; i < 10; i++) {
    config = new CoreBufferViewConfig(i);
    config->setBufferViewName(QString("asdf%1").arg(i));
    addBufferViewConfig(config);
  }
}

void CoreBufferViewManager::requestCreateBufferView(const QString &bufferViewName) {
  // FIXME retreive new Id from database or whereever this stuff will be stored
  int maxId = -1;
  BufferViewConfigHash::const_iterator iter = bufferViewConfigHash().constBegin();
  BufferViewConfigHash::const_iterator iterEnd = bufferViewConfigHash().constEnd();
  while(iter != iterEnd) {
    if((*iter)->bufferViewName() == bufferViewName)
      return;
    
    if((*iter)->bufferViewId() > maxId)
      maxId = (*iter)->bufferViewId();
    
    iter++;
  }
  maxId++;

  CoreBufferViewConfig *config = new CoreBufferViewConfig(maxId);
  config->setBufferViewName(bufferViewName);
  addBufferViewConfig(config);
}
