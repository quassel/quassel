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

#include "bufferviewconfig.h"

BufferViewConfig::BufferViewConfig(int bufferViewId, QObject *parent)
  : SyncableObject(parent),
    _bufferViewId(bufferViewId)
{
  setObjectName(QString::number(bufferViewId));
}

void BufferViewConfig::setBufferViewName(const QString &bufferViewName) {
  if(_bufferViewName == bufferViewName)
    return;

  _bufferViewName = bufferViewName;
  emit bufferViewNameSet(bufferViewName);
}

void BufferViewConfig::setNetworkId(const NetworkId &networkId) {
  if(_networkId == networkId)
    return;

  _networkId = networkId;
  emit networkIdSet(networkId);
}

void BufferViewConfig::setAddNewBuffersAutomatically(bool addNewBuffersAutomatically) {
  if(_addNewBuffersAutomatically == addNewBuffersAutomatically)
    return;

  _addNewBuffersAutomatically = addNewBuffersAutomatically;
  emit addNewBuffersAutomaticallySet(addNewBuffersAutomatically);
}

void BufferViewConfig::setSortAlphabetically(bool sortAlphabetically) {
  if(_sortAlphabetically == sortAlphabetically)
    return;

  _sortAlphabetically = sortAlphabetically;
  emit sortAlphabeticallySet(sortAlphabetically);
}
