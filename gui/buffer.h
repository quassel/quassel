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

#include <QtCore>
#include <QtGui>

#include "chatwidget.h"
#include "global.h"
#include "message.h"

class ChatWidget;
class ChatLine;
class ChatWidgetContents;
class BufferWidget;
struct BufferState;

//!\brief Encapsulates the contents of a single channel, query or server status context.
/** A Buffer maintains a list of existing nicks and their status. New messages can be appended using
 * displayMsg(). A buffer displays its contents by way of a BufferWidget, which can be shown
 * (and created on demand) by calling showWidget().
 */
class Buffer : public QObject {
  Q_OBJECT

  public:
    //Buffer(QString network, QString buffer);
    Buffer(BufferId);
    ~Buffer();
    static void init();

    enum Type { ServerBuffer, ChannelBuffer, QueryBuffer };
    Type bufferType() { return type; }
    bool isActive() { return active; }

    QString networkName() { return _networkName; }
    QString bufferName() { return _bufferName; }
    BufferId bufferId() { return id; }
    QList<ChatLine *> contents() { return lines; }
    VarMap nickList() { return nicks; }
    QString topic() { return _topic; }
    QString ownNick() { return _ownNick; }

  signals:
    void userInput(BufferId, QString);
    //void msgDisplayed(Message);
    void chatLineAppended(ChatLine *);
    void chatLinePrepended(ChatLine *);
    void nickListChanged(VarMap nicks);
    void topicSet(QString topic);
    void ownNickSet(QString ownNick);
    void bufferUpdated(Buffer *);
    void bufferDestroyed(Buffer *);

  public slots:
    void setActive(bool active = true);
    //void displayMsg(Message);
    //void prependMessages(QList<Message>); // for backlog
    void appendChatLine(ChatLine *);
    void prependChatLine(ChatLine *);
    //void prependChatLines(QList<ChatLine *>);
    //void recvStatusMsg(QString msg);
    void setTopic(QString);
    //void setNicks(QStringList);
    void addNick(QString nick, VarMap props);
    void renameNick(QString oldnick, QString newnick);
    void removeNick(QString nick);
    void updateNick(QString nick, VarMap props);
    void setOwnNick(QString nick);

    void processUserInput(QString);

  private:
    BufferId id;
    bool active;
    Type type;

    VarMap nicks;
    QString _topic;
    QString _ownNick;
    QString _networkName, _bufferName;
    BufferState *state;

    //QList<Message> _contents;
    QList<ChatLine *> lines;

};

#endif
