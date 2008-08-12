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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <QDateTime>

class AbstractUiMsg;
class IrcChannel;
class NickModel;

struct BufferState;

#include "message.h"
#include "bufferinfo.h"

/**
 */
class Buffer : public QObject {
  Q_OBJECT

public:
  enum Activity {
    NoActivity = 0x00,
    OtherActivity = 0x01,
    NewMessage = 0x02,
    Highlight = 0x40
  };
  Q_DECLARE_FLAGS(ActivityLevel, Activity)

  Buffer(BufferInfo, QObject *parent = 0);

  BufferInfo bufferInfo() const;

private:
  BufferInfo _bufferInfo;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Buffer::ActivityLevel)

#endif
