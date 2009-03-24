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

#ifndef BUFFERVIEWOVERLAY_H
#define BUFFERVIEWOVERLAY_H

#include <QObject>

#include "types.h"

class BufferViewConfig;
class ClientBufferViewConfig;

class BufferViewOverlay : public QObject {
  Q_OBJECT

public:
  BufferViewOverlay(QObject *parent = 0);

  inline bool allNetworks() const { return _networkIds.contains(NetworkId()); }

  inline const QSet<NetworkId> &networkIds() const { return _networkIds; }
  inline const QSet<BufferId> &bufferIds() const { return _buffers; }
  inline const QSet<BufferId> &removedBufferIds() const { return _removedBuffers; }
  inline const QSet<BufferId> &tempRemovedBufferIds() const { return _tempRemovedBuffers; }

  inline bool addBuffersAutomatically() const { return _addBuffersAutomatically; }
  inline bool hideInactiveBuffers() const { return _hideInactiveBuffers; }
  inline int allowedBufferTypes() const { return _allowedBufferTypes; }
  inline int minimumActivity() const { return _minimumActivity; }

public slots:
  void addView(int viewId);
  void removeView(int viewId);

  // updates propagated from the actual views
  void update();

signals:
  void hasChanged();

protected:
  virtual void customEvent(QEvent *event);

private slots:
  void viewInitialized();
  void viewInitialized(BufferViewConfig *config);

private:
  void updateHelper();
  bool _aboutToUpdate;

  QSet<int> _bufferViewIds;

  QSet<NetworkId> _networkIds;

  bool _addBuffersAutomatically;
  bool _hideInactiveBuffers;
  int _allowedBufferTypes;
  int _minimumActivity;

  QSet<BufferId> _buffers;
  QSet<BufferId> _removedBuffers;
  QSet<BufferId> _tempRemovedBuffers;

  static const int _updateEventId;
};

#endif //BUFFERVIEWOVERLAY_H
