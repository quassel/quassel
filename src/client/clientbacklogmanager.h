/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef CLIENTBACKLOGMANAGER_H
#define CLIENTBACKLOGMANAGER_H

#include "backlogmanager.h"
#include "message.h"

class ClientBacklogManager : public BacklogManager {
  Q_OBJECT

public:
  ClientBacklogManager(QObject *parent = 0);

  virtual const QMetaObject *syncMetaObject() const { return &BacklogManager::staticMetaObject; }

public slots:
  virtual void receiveBacklog(BufferId bufferId, int lastMsgs, int offset, QVariantList msgs);
  virtual QVariantList requestBacklog(BufferId bufferId, int lastMsgs = -1, int offset = -1);
  void requestInitialBacklog();

private:
  bool _buffer;
  QList<Message> _messageBuffer;
  QSet<BufferId> _buffersWaiting;
};

#endif // CLIENTBACKLOGMANAGER_H
