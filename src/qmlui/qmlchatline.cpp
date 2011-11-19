/***************************************************************************
 *   Copyright (C) 2005-2011 by the Quassel Project                        *
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
#include <QPainter>

#include "qmlchatline.h"

void QmlChatLine::registerTypes() {
  qRegisterMetaType<RenderData>("QmlChatLine::RenderData");
  qRegisterMetaTypeStreamOperators<RenderData>("QmlChatLine::RenderData");
  qmlRegisterType<QmlChatLine>("eu.quassel.qml", 1, 0, "ChatLine");
}

QDataStream &operator<<(QDataStream &out, const QmlChatLine::RenderData &data) {
  for(int i = 0; i < (int)QmlChatLine::NumColumns; ++i) {
    const QmlChatLine::RenderData::Column &col = data[static_cast<QmlChatLine::ColumnType>(i)];
    out << col.text << col.formats;
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, QmlChatLine::RenderData &data) {
  for(int i = 0; i < (int)QmlChatLine::NumColumns; ++i) {
    QmlChatLine::RenderData::Column &col = data[static_cast<QmlChatLine::ColumnType>(i)];
    in >> col.text >> col.formats;
  }
  return in;
}

QmlChatLine::QmlChatLine(QDeclarativeItem *parent)
  : QDeclarativeItem(parent),
    _timestampWidth(0),
    _senderWidth(0),
    _contentsWidth(0),
    _layout(0)
{
  setFlag(ItemHasNoContents, false);
  setImplicitHeight(20);
  setImplicitWidth(1000);
  connect(this, SIGNAL(columnWidthChanged(ColumnType)), SLOT(onColumnWidthChanged(ColumnType)));
}

QmlChatLine::~QmlChatLine() {

}

void QmlChatLine::setTimestampWidth(qreal w) {
  if(w != _timestampWidth) {
    _timestampWidth = w;
    emit timestampWidthChanged(w);
    emit columnWidthChanged(TimestampColumn);
  }
}

void QmlChatLine::setSenderWidth(qreal w) {
  if(w != _senderWidth) {
    _senderWidth = w;
    emit senderWidthChanged(w);
    emit columnWidthChanged(SenderColumn);
  }
}

void QmlChatLine::setContentsWidth(qreal w) {
  if(w != _contentsWidth) {
    _contentsWidth = w;
    emit contentsWidthChanged(w);
    emit columnWidthChanged(ContentsColumn);
  }
}

void QmlChatLine::setColumnSpacing(qreal s) {
  if(s != _columnSpacing) {
    _columnSpacing = s;
    emit columnSpacingChanged(s);
  }
}

QPointF QmlChatLine::columnPos(ColumnType colType) const {
  switch(colType) {
  case TimestampColumn:
    return QPointF(0, 0);
  case SenderColumn:
    return QPointF(timestampWidth(), 0);
  case ContentsColumn:
    return QPointF(timestampWidth() + senderWidth(), 0);
  default:
    return QPointF();
  }
}

qreal QmlChatLine::columnWidth(ColumnType colType) const {
  switch(colType) {
  case TimestampColumn:
    return timestampWidth();
  case SenderColumn:
    return senderWidth();
  case ContentsColumn:
    return contentsWidth();
  default:
    return 0;
  }
}

QRectF QmlChatLine::columnBoundingRect(ColumnType colType) const {
  QRectF rect;
  switch(colType) {
  case TimestampColumn:
    return QRectF(columnPos(TimestampColumn), QSizeF(timestampWidth() - columnSpacing(), implicitHeight()));
  case SenderColumn:
    return QRectF(columnPos(SenderColumn), QSizeF(senderWidth() - columnSpacing(), implicitHeight()));
  case ContentsColumn:
    return QRectF(columnPos(ContentsColumn), QSizeF(contentsWidth(), implicitHeight()));
  default:
    return QRectF();
  }
}

void QmlChatLine::setRenderData(const RenderData &data) {
  _data = data;
  if(_layout) {
    delete _layout;
    _layout = 0;
  }

  //update();
}

QmlChatLine::ColumnLayout *QmlChatLine::layout() const {
  if(!_layout) {
    _layout = new ColumnLayout(this);
  }
  return _layout;
}

void QmlChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)
  //painter->drawText(0, 0, renderData()[TimestampColumn].text);
  //painter->drawText(timestampWidth() + columnSpacing(), 0, renderData()[SenderColumn].text);
  //painter->drawText(timestampWidth() + senderWidth() + 2*columnSpacing(), 0, renderData()[ContentsColumn].text);
  layout()->draw(painter);
}

void QmlChatLine::onColumnWidthChanged(ColumnType colType) {

  //qDebug() << "changed width" << _timestampWidth << _senderWidth << _contentsWidth;
  //setImplicitHeight(implicitHeight() + 5);

  if(colType == ContentsColumn) {
    layout()->prepare();
    setImplicitHeight(layout()->height());
  }

  update();
}

/**************************************************************************************/

QmlChatLine::ColumnLayout::ColumnLayout(const QmlChatLine *parent)
  : _parent(parent)
{

}

qreal QmlChatLine::ColumnLayout::height() const {
  return chatLine()->contentsWidth()/20;
}

void QmlChatLine::ColumnLayout::prepare() {

}

void QmlChatLine::ColumnLayout::draw(QPainter *p) {
  p->drawText(chatLine()->boundingRect(), chatLine()->renderData()[ContentsColumn].text);
  //p->drawText(chatLine()->timestampWidth() + chatLine()->columnSpacing(), 0, chatLine()->renderData()[SenderColumn].text);
  //p->drawText(chatLine()->timestampWidth() + chatLine()->senderWidth() + 2*chatLine()->columnSpacing(), 0, chatLine()->renderData()[ContentsColumn].text);

}
