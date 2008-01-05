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

#ifndef _CHATVIEW_H_
#define _CHATVIEW_H_

#include <QGraphicsView>

class AbstractUiMsg;
class Buffer;
class ChatLine;
class ChatScene;

class ChatView : public QGraphicsView {
  Q_OBJECT

  public:
    ChatView(Buffer *, QWidget *parent = 0);
    ~ChatView();

    ChatScene *scene() const;

  public slots:
/*
    void clear();

    void prependMsg(AbstractUiMsg *);
    void appendMsg(AbstractUiMsg *);

    void prependChatLine(ChatLine *);
    void appendChatLine(ChatLine *);
    void prependChatLines(QList<ChatLine *>);
    void appendChatLines(QList<ChatLine *>);

    void setContents(QList<ChatLine *>);
*/
  private:
    ChatScene *_scene;
};

#endif
