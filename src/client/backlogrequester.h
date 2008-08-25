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

#ifndef BACKLOGREQUESTER_H
#define BACKLOGREQUESTER_H

#include <QList>

#include "client.h"
#include "networkmodel.h"
#include "types.h"

class BacklogManager;

class BacklogRequester {
public:
  BacklogRequester(BacklogManager *backlogManger);
  virtual inline ~BacklogRequester() {}

  virtual void requestBacklog() = 0;

protected:
  inline QList<BufferId> allBufferIds() const { return Client::networkModel()->allBufferIds(); }
  BacklogManager *backlogManager;
};


class FixedBacklogRequester : public BacklogRequester {
public:
  FixedBacklogRequester(BacklogManager *backlogManager);

  virtual void requestBacklog();

private:
  static const int backlogCount;
};


#endif //BACKLOGREQUESTER_H
