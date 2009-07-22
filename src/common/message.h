/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <QString>
#include <QDateTime>

#include "bufferinfo.h"
#include "types.h"

class Message {
  Q_DECLARE_TR_FUNCTIONS(Message)

public:
  /** The different types a message can have for display */
  enum Type {
    Plain     = 0x0001,
    Notice    = 0x0002,
    Action    = 0x0004,
    Nick      = 0x0008,
    Mode      = 0x0010,
    Join      = 0x0020,
    Part      = 0x0040,
    Quit      = 0x0080,
    Kick      = 0x0100,
    Kill      = 0x0200,
    Server    = 0x0400,
    Info      = 0x0800,
    Error     = 0x1000,
    DayChange = 0x2000,
    Topic     = 0x4000
  };

  // DO NOT CHANGE without knowing what you do, some of these flags are stored in the database
  enum Flag {
    None = 0x00,
    Self = 0x01,
    Highlight = 0x02,
    Redirected = 0x04,
    ServerMsg = 0x08,
    Backlog = 0x80
  };
  Q_DECLARE_FLAGS(Flags, Flag)


  Message(const BufferInfo &bufferInfo = BufferInfo(), Type type = Plain, const QString &contents = "", const QString &sender = "", Flags flags = None);
  Message(const QDateTime &ts, const BufferInfo &buffer = BufferInfo(), Type type = Plain,
          const QString &contents = "", const QString &sender = "", Flags flags = None);

  inline static Message ChangeOfDay(const QDateTime &day) { return Message(day, BufferInfo(), DayChange, tr("Day changed to %1").arg(day.toString("dddd MMMM d yyyy"))); }
  inline const MsgId &msgId() const { return _msgId; }
  inline void setMsgId(MsgId id) { _msgId = id; }

  inline const BufferInfo &bufferInfo() const { return _bufferInfo; }
  inline const BufferId &bufferId() const { return _bufferInfo.bufferId(); }
  inline void setBufferId(BufferId id) { _bufferInfo.setBufferId(id); }
  inline const QString &contents() const { return _contents; }
  inline const QString &sender() const { return _sender; }
  inline Type type() const { return _type; }
  inline Flags flags() const { return _flags; }
  inline void setFlags(Flags flags) { _flags = flags; }
  inline const QDateTime &timestamp() const { return _timestamp; }

  inline bool isValid() const { return _msgId.isValid(); }

  inline bool operator<(const Message &other) const { return _msgId < other._msgId; }

private:
  QDateTime _timestamp;
  MsgId _msgId;
  BufferInfo _bufferInfo;
  QString _contents;
  QString _sender;
  Type _type;
  Flags _flags;

  friend QDataStream &operator>>(QDataStream &in, Message &msg);
};

typedef QList<Message> MessageList;

QDataStream &operator<<(QDataStream &out, const Message &msg);
QDataStream &operator>>(QDataStream &in, Message &msg);
QDebug operator<<(QDebug dbg, const Message &msg);

Q_DECLARE_METATYPE(Message)
Q_DECLARE_OPERATORS_FOR_FLAGS(Message::Flags)

#endif
