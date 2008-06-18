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

#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextLayout>

#include "chatitem.h"
#include "chatlinemodel.h"
#include "qtui.h"

ChatItem::ChatItem(const QPersistentModelIndex &index_, QGraphicsItem *parent) : QGraphicsItem(parent), _index(index_) {
  _fontMetrics = QtUi::style()->fontMetrics(data(ChatLineModel::FormatRole).value<UiStyle::FormatList>().at(0).second);
  _layout = 0;
  _lines = 0;
}

ChatItem::~ChatItem() {

}

QVariant ChatItem::data(int role) const {
  if(!_index.isValid()) {
    qWarning() << "ChatItem::data(): Model index is invalid!" << _index;
    return QVariant();
  }
  return _index.data(role);
}

int ChatItem::setWidth(int w) {
  w -= 10;
  if(w == _boundingRect.width()) return _boundingRect.height();
  int h = heightForWidth(w);
  _boundingRect.setWidth(w);
  _boundingRect.setHeight(h);
  if(haveLayout()) updateLayout();
  return h;
}

int ChatItem::heightForWidth(int width) {
  if(data(ChatLineModel::ColumnTypeRole).toUInt() != ChatLineModel::ContentsColumn)
    return fontMetrics()->lineSpacing(); // only contents can be multi-line

  ChatLineModel::WrapList wrapList = data(ChatLineModel::WrapListRole).value<ChatLineModel::WrapList>();
  _lines = 1;
  qreal w = 0;
  for(int i = 0; i < wrapList.count(); i++) {
    w += wrapList.at(i).width;
    if(w <= width) {
      w += wrapList.at(i).trailing;
      continue;
    }
    _lines++;
    w = wrapList.at(i).width;
    while(w >= width) {  // handle words longer than a line
      _lines++;
      // We do not want to compute an exact split position (that would require us to calculate glyph widths
      // and also apply formats in this step...)
      // Just using width should be a good estimate, but since a character can't be split in the middle, we
      // subtract averageCharWidth as well... sometimes this won't be enough, but meh.
      w -= width - 4*fontMetrics()->averageCharWidth();
    }
  }
  return _lines * fontMetrics()->lineSpacing();
}

void ChatItem::layout() {
  if(haveLayout()) return;
  _layout = new QTextLayout(data(MessageModel::DisplayRole).toString());

  QTextOption option;
  option.setWrapMode(QTextOption::WrapAnywhere);
  _layout->setTextOption(option);

  // Convert format information into a FormatRange
  QList<QTextLayout::FormatRange> formatRanges;
  UiStyle::FormatList formatList = data(MessageModel::FormatRole).value<UiStyle::FormatList>();
  QTextLayout::FormatRange range;
  int i = 0;
  for(i = 0; i < formatList.count(); i++) {
    range.format = QtUi::style()->mergedFormat(formatList.at(i).second);
    range.start = formatList.at(i).first;
    if(i > 0) formatRanges.last().length = range.start - formatRanges.last().start;
    formatRanges.append(range);
  }
  if(i > 0) formatRanges.last().length = _layout->text().length() - formatRanges.last().start;
  _layout->setAdditionalFormats(formatRanges);
  updateLayout();
}

void ChatItem::updateLayout() {
  if(!haveLayout()) layout();

  // Now layout
  ChatLineModel::WrapList wrapList = data(ChatLineModel::WrapListRole).value<ChatLineModel::WrapList>();
  if(!wrapList.count()) return; // empty chatitem
  int wordidx = 0;
  ChatLineModel::Word word = wrapList.at(0);

  qreal h = 0;
  _layout->beginLayout();
  forever {
    QTextLine line = _layout->createLine();
    if (!line.isValid())
      break;

    if(word.width >= width()) {
      line.setLineWidth(width());
      word.width -= line.naturalTextWidth();
      word.start = line.textStart() + line.textLength();
      //qDebug() << "setting width: " << width();
    } else {
      int w = 0;
      while((w += word.width) <= width() && wordidx < wrapList.count()) {
        w += word.trailing;
        if(++wordidx < wrapList.count()) word = wrapList.at(wordidx);
        else {
          // last word (and it fits), but if we expected an extra line, wrap anyway here
          // yeah, this is cheating, but much cheaper than computing widths ourself
          if(_layout->lineCount() < _lines) {
            wordidx--; qDebug() << "trigger!" << _lines << _layout->text();
            break;
          }
        }
      }
      int lastcol = wordidx < wrapList.count() ? word.start : _layout->text().length();
      line.setNumColumns(lastcol - line.textStart());// qDebug() << "setting cols:" << lastcol - line.textStart();
    }

    h += fontMetrics()->leading();
    line.setPosition(QPointF(0, h));
    h += line.height();
  }
  _layout->endLayout();
}

void ChatItem::clearLayout() {
  delete _layout;
  _layout = 0;
}

void ChatItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option); Q_UNUSED(widget);
  layout();
  _layout->draw(painter, QPointF(0,0), QVector<QTextLayout::FormatRange>(), boundingRect());
  painter->drawRect(boundingRect());
  int width = 0;
  QVariantList wrapList = data(ChatLineModel::WrapListRole).toList();
  for(int i = 2; i < wrapList.count(); i+=2) {
    QRect r(wrapList[i-1].toUInt(), 0, wrapList[i+1].toUInt() - wrapList[i-1].toUInt(), fontMetrics()->lineSpacing());
    painter->drawRect(r);
  }
}

/*
void ChatItem::layout() {
  if(!_layout.additionalFormats().count()) return; // no text set
  if(_width <= 0) return;
  prepareGeometryChange();
  QFontMetrics metrics(_layout.additionalFormats()[0].format.font());
  int leading = metrics.leading();
  int height = 0;
  _layout.setTextOption(textOption());
  _layout.beginLayout();
  while(1) {
    QTextLine line = _layout.createLine();
    if(!line.isValid()) break;
    line.setLineWidth(_width);
    if(textOption().wrapMode() != QTextOption::NoWrap && line.naturalTextWidth() > _width) {
      // word did not fit, we need to wrap it in the middle
      // this is a workaround for Qt failing to handle WrapAtWordBoundaryOrAnywhere correctly
      QTextOption::WrapMode mode = textOption().wrapMode();
      textOption().setWrapMode(QTextOption::WrapAnywhere);
      _layout.setTextOption(textOption());
      line.setLineWidth(_width);
      textOption().setWrapMode(mode);
      _layout.setTextOption(textOption());
    }
    height += leading;
    line.setPosition(QPoint(0, height));
    height += line.height();
  }
  _layout.endLayout();
  update();
}    QDateTime _timestamp;
    MsgId _msgId;


QRectF ChatItem::boundingRect() const {
  return _layout.boundingRect();
}

void ChatItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option); Q_UNUSED(widget);
  _layout.draw(painter, QPointF(0, 0));

}
*/

/*
void ChatItem::mouseMoveEvent ( QGraphicsSceneMouseEvent * event ) {
  qDebug() << (void*)this << "moving" << event->pos();
  if(event->pos().y() < 0) {
    QTextCursor cursor(document());
    //cursor.insertText("foo");
    //cursor.select(QTextCursor::Document);
    event->ignore();
  } else QGraphicsTextItem::mouseMoveEvent(event);
}
*/
