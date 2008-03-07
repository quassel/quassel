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
class ChatLineData;

/* Concept Ideas
* Probably makes sense to have ChatLineData be the AbstractUiMsg instead, if it turns out that creating ChatLineData
is the expensive part... In that case, we could have a QHash<MsgId, ChatLineData*> in the Client, and ChatLine just
gets a data pointer. This would allow us to share most data between AbstractUiMsgs, and ChatLines themselves could
be pretty cheap - that'd be a clean solution for having a monitor buffer, highlight buffer etcpp.

* ItemLayoutData

*/

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

//! This contains the data of a ChatLine, i.e. mainly the styled message contents.
/** By separating ChatLine and ChatLineData, ChatLine itself is very small and we can reuse the
 *  same contents in several ChatLine objects without duplicating data.
 */
class ChatLineData {

  public:
    ChatLineData(const Message &msg);

    inline UiStyle::StyledText styledSender() const { return _styledSender; }
    inline UiStyle::StyledText styledTimestamp() const { return _styledTimestamp; }
    inline UiStyle::StyledText styledText() const { return _styledText; }

    inline QString sender() const { return _styledSender.text; }
    inline QString text() const { return _styledText.text; }
    inline QDateTime timestamp() const { return _timestamp; }
    inline MsgId msgId() const { return _msgId; }

  private:
    UiStyle::StyledText _styledSender, _styledText, _styledTimestamp;
    QDateTime _timestamp;
    MsgId _msgId;

};


#endif
