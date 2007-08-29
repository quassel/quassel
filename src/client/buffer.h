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

#include "global.h"

class AbstractUiMsg;
class Message;
struct BufferState;

//!\brief Encapsulates the contents of a single channel, query or server status context.
/** A Buffer maintains a list of existing nicks and their status.
 */
class Buffer : public QObject {
  Q_OBJECT

  public:
    Buffer(BufferId);
    ~Buffer();

    enum Type { ServerBuffer, ChannelBuffer, QueryBuffer };

    enum Activity {
      NoActivity = 0x00,
      OtherActivity = 0x01,
      NewMessage = 0x02,
      Highlight = 0x40
    };
    Q_DECLARE_FLAGS(ActivityLevel, Activity)

    Type bufferType() const;
    bool isActive() const;

    QString networkName() const;
    QString bufferName() const;
    QString displayName() const;
    BufferId bufferId() const;
    QList<AbstractUiMsg *> contents() const;
    QVariantMap nickList() const;
    QString topic() const;
    QString ownNick() const;
    bool isStatusBuffer() const;

  signals:
    void userInput(const BufferId &, QString);
    void nickListChanged(QVariantMap nicks);
    void topicSet(QString topic);
    void ownNickSet(QString ownNick);
    void bufferUpdated(Buffer *);
    void bufferDestroyed(Buffer *);

    void msgAppended(AbstractUiMsg *);
    void msgPrepended(AbstractUiMsg *);
    void layoutQueueEmpty();

  public slots:
    void setActive(bool active = true);
    void appendMsg(const Message &);
    void prependMsg(const Message &);
    bool layoutMsg();
    void setTopic(QString);
    //void setNicks(QStringList);
    void addNick(QString nick, QVariantMap props);
    void renameNick(QString oldnick, QString newnick);
    void removeNick(QString nick);
    void updateNick(QString nick, QVariantMap props);
    void setOwnNick(QString nick);

    void processUserInput(QString);

  private:
    BufferId id;
    bool active;
    Type type;

    QVariantMap nicks;
    QString _topic;
    QString _ownNick;
    QString _networkName, _bufferName;
    BufferState *state;

    QList<Message> layoutQueue;
    QList<AbstractUiMsg *> layoutedMsgs;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(Buffer::ActivityLevel)

#endif
