/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <QString>
#include <QDateTime>

#include "bufferinfo.h"
#include "types.h"

class Message {
  Q_DECLARE_TR_FUNCTIONS(Message);

  public:
    /** The different types a message can have for display */
    enum Type { Plain, Notice, Action, Nick, Mode, Join, Part, Quit, Kick, Kill, Server, Info, Error };
    enum Flags { None = 0, Self = 1, PrivMsg = 2, Highlight = 4 };

    Message(BufferInfo buffer = BufferInfo(), Type type = Plain, QString text = "", QString sender = "", quint8 flags = None);

    Message(QDateTime ts, BufferInfo buffer = BufferInfo(), Type type = Plain, QString text = "", QString sender = "", quint8 flags = None);

    MsgId msgId() const;
    void setMsgId(MsgId id);

    BufferInfo buffer() const;
    QString text() const;
    QString sender() const;
    Type type() const;
    quint8 flags() const;
    QDateTime timestamp() const;

    QString formattedTimestamp();
    QString formattedSender();
    QString formattedText();

    //static QString formattedToHtml(const QString &);

    void format();

  private:
    QDateTime _timestamp;
    MsgId _msgId;
    BufferInfo _buffer;
    QString _text;
    QString _sender;
    Type _type;
    quint8 _flags;

    QString _formattedTimestamp, _formattedSender, _formattedText; // cache

    /** Convert mIRC control codes to our own */
    QString mircToInternal(QString);

    friend QDataStream &operator>>(QDataStream &in, Message &msg);
};

QDataStream &operator<<(QDataStream &out, const Message &msg);
QDataStream &operator>>(QDataStream &in, Message &msg);

Q_DECLARE_METATYPE(Message);

#endif
