/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

struct BufferState;

#include "message.h"
#include "bufferinfo.h"

#include <QVariantMap>

//!\brief Encapsulates the contents of a single channel, query or server status context.
/** A Buffer maintains a list of existing nicks and their status.
 */
class Buffer : public QObject {
  Q_OBJECT

public:
  Buffer(BufferInfo, QObject *parent = 0);
  virtual ~Buffer();

  enum Type {
    StatusType,
    ChannelType,
    QueryType
  };

  enum Activity {
    NoActivity = 0x00,
    OtherActivity = 0x01,
    NewMessage = 0x02,
    Highlight = 0x40
  };
  Q_DECLARE_FLAGS(ActivityLevel, Activity)

  bool isStatusBuffer() const;
  Type bufferType() const;
  bool isActive() const;

  BufferInfo bufferInfo() const;
  void updateBufferInfo(BufferInfo bufferid);

  uint uid() const;
  uint networkId() const;
  
  QString networkName() const;
  QString bufferName() const;
  QString displayName() const;
  
  QList<AbstractUiMsg *> contents() const;
  
  QVariantMap nickList() const;
  QString topic() const;
  QString ownNick() const;

signals:
  void userInput(const BufferInfo &, QString);
  void nickListChanged(QVariantMap nicks);
  void topicSet(QString topic);
  void ownNickSet(QString ownNick);
  void bufferUpdated(Buffer *);

  void msgAppended(AbstractUiMsg *);
  void msgPrepended(AbstractUiMsg *);
  void layoutQueueEmpty();

public slots:
  void setActive(bool active = true);
  void appendMsg(const Message &);
  void prependMsg(const Message &);
  bool layoutMsg();

  // no longer needed
//   void setTopic(QString);
//   //void setNicks(QStringList);
//   void addNick(QString nick, QVariantMap props);
//   void renameNick(QString oldnick, QString newnick);
//   void removeNick(QString nick);
//   void updateNick(QString nick, QVariantMap props);
//  void setOwnNick(QString nick);

  void processUserInput(QString);

private:
  BufferInfo _bufferInfo;
  bool _active;
  Type _type;
  BufferState *state;

  QList<Message> layoutQueue;
  QList<AbstractUiMsg *> layoutedMsgs;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(Buffer::ActivityLevel)

#endif
