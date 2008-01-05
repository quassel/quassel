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

#ifndef _CHATLINE_H_
#define _CHATLINE_H_

#include <QGraphicsItem>

#include "message.h"
#include "quasselui.h"
#include "uistyle.h"

class ChatItem;

class ChatLine : public QGraphicsItem, public AbstractUiMsg {

  public:
    ChatLine(Message);
    virtual ~ChatLine();
    virtual QString sender() const;
    virtual QString text() const;
    virtual MsgId msgId() const;
    virtual BufferInfo bufferInfo() const;
    virtual QDateTime timestamp() const;

    virtual QRectF boundingRect () const;
    virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    void layout();

    void setColumnWidths(int tsColWidth, int senderColWidth, int textColWidth);

    void myMousePressEvent ( QGraphicsSceneMouseEvent * event ) { qDebug() << "press"; mousePressEvent(event); }

  protected:
    bool sceneEvent ( QEvent * event );

  private:
    UiStyle::StyledText _styledTimestamp, _styledText, _styledSender;
    QDateTime _timestamp;
    MsgId _msgId;

    ChatItem *_tsItem, *_senderItem, *_textItem;
    int _tsColWidth, _senderColWidth, _textColWidth;
};

#endif
