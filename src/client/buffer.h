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

class AbstractUiMsg;
class IrcChannel;
class NickModel;

struct BufferState;

#include "message.h"
#include "bufferinfo.h"

//!\brief Encapsulates the contents of a single channel, query or server status context.
/** A Buffer maintains a list of existing nicks and their status.
 */
class Buffer : public QObject {
  Q_OBJECT

public:
  Buffer(BufferInfo, QObject *parent = 0);

  BufferInfo bufferInfo() const;

  QList<AbstractUiMsg *> contents() const;
  
signals:
  void userInput(const BufferInfo &, QString);

  void msgAppended(AbstractUiMsg *);
  void msgPrepended(AbstractUiMsg *);
  void layoutQueueEmpty();

public slots:
  void appendMsg(const Message &);
  void prependMsg(const Message &);
  bool layoutMsg();

  void processUserInput(QString);

private:
  BufferInfo _bufferInfo;

  QList<Message> layoutQueue;
  QList<AbstractUiMsg *> layoutedMsgs;

};

#endif
