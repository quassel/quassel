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

#ifndef BUFFERVIEWCONFIG_H
#define BUFFERVIEWCONFIG_H

#include "syncableobject.h"

#include "types.h"

class BufferViewConfig : public SyncableObject {
  Q_OBJECT
  Q_PROPERTY(QString bufferViewName READ bufferViewName WRITE setBufferViewName)
  Q_PROPERTY(NetworkId networkId READ networkId WRITE setNetworkId)
  Q_PROPERTY(bool addNewBuffersAutomatically READ addNewBuffersAutomatically WRITE setAddNewBuffersAutomatically)
  Q_PROPERTY(bool sortAlphabetically READ sortAlphabetically WRITE setSortAlphabetically)

public:
  BufferViewConfig(int bufferViewId, QObject *parent = 0);

public slots:
  inline int bufferViewId() const { return _bufferViewId; }

  inline const QString &bufferViewName() const { return _bufferViewName; }
  void setBufferViewName(const QString &bufferViewName);

  inline const NetworkId &networkId() const { return _networkId; }
  void setNetworkId(const NetworkId &networkId);

  inline bool addNewBuffersAutomatically() const { return _addNewBuffersAutomatically; }
  void setAddNewBuffersAutomatically(bool addNewBuffersAutomatically);

  inline bool sortAlphabetically() const { return _sortAlphabetically; }
  void setSortAlphabetically(bool sortAlphabetically);

  virtual inline void requestSetBufferViewName(const QString &bufferViewName) { emit setBufferViewNameRequested(bufferViewName); }

signals:
  void bufferViewNameSet(const QString &bufferViewName);
  void networkIdSet(const NetworkId &networkId);
  void addNewBuffersAutomaticallySet(bool addNewBuffersAutomatically);
  void sortAlphabeticallySet(bool sortAlphabetically);  

  void setBufferViewNameRequested(const QString &bufferViewName);

private:
  int _bufferViewId;
  QString _bufferViewName;
  NetworkId _networkId;
  bool _addNewBuffersAutomatically;
  bool _sortAlphabetically;
};

#endif // BUFFERVIEWCONFIG_H
