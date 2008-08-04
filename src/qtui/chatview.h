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

#ifndef CHATVIEW_H_
#define CHATVIEW_H_

#include <QGraphicsView>

#include "abstractbuffercontainer.h"

class AbstractUiMsg;
class Buffer;
class ChatLine;
class ChatScene;
class MessageFilter;

class ChatView : public QGraphicsView, public AbstractChatView {
  Q_OBJECT

  public:
    ChatView(MessageFilter *, QWidget *parent = 0);
    ChatView(Buffer *, QWidget *parent = 0);
    ~ChatView();

    ChatScene *scene() const;

  public slots:

    void clear();

  protected:
    virtual void resizeEvent(QResizeEvent *event);

  protected slots:
    virtual void sceneHeightChanged(qreal height);

  private:
    void init(MessageFilter *filter);

    ChatScene *_scene;
};

#endif
