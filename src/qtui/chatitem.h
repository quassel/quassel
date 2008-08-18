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

#ifndef CHATITEM_H_
#define CHATITEM_H_

#include <QGraphicsItem>
#include <QObject>

#include "chatlinemodel.h"
#include "chatscene.h"
#include "uistyle.h"
#include "qtui.h"

class QTextLayout;

class ChatItem : public QGraphicsItem {

protected:
  ChatItem(ChatLineModel::ColumnType column, QAbstractItemModel *, QGraphicsItem *parent);
  virtual ~ChatItem();

public:
  inline const QAbstractItemModel *model() const { return chatScene() ? chatScene()->model() : 0; }
  inline int row() const;
  virtual ChatLineModel::ColumnType column() const = 0;
  inline ChatScene *chatScene() const { return qobject_cast<ChatScene *>(scene()); }

  inline QFontMetricsF *fontMetrics() const { return _fontMetrics; }
  inline QRectF boundingRect() const { return _boundingRect; }
  inline qreal width() const { return _boundingRect.width(); }
  inline qreal height() const { return _boundingRect.height(); }

  virtual inline bool haveLayout() const { return layout() != 0; }
  virtual void clearLayout();
  virtual QTextLayout *createLayout(QTextOption::WrapMode, Qt::Alignment = Qt::AlignLeft);
  virtual void updateLayout();
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

  virtual QVariant data(int role) const;

  // returns height
  qreal setGeometry(qreal width, qreal height = -1);

  // selection stuff, to be called by the scene
  void clearSelection();
  void setFullSelection();
  void continueSelecting(const QPointF &pos);

  QList<QRectF> findWords(const QString &searchWord, Qt::CaseSensitivity caseSensitive);

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

  virtual inline QTextLayout *layout() const { return _layout; }
  virtual inline void setLayout(QTextLayout *l) { _layout = l; }
  qint16 posToCursor(const QPointF &pos);

  virtual qreal computeHeight();

  QRectF _boundingRect;

private:
  // internal selection stuff
  void setSelection(int start, int length);

  QFontMetricsF *_fontMetrics;

  enum SelectionMode { NoSelection, PartialSelection, FullSelection };
  SelectionMode _selectionMode;
  qint16 _selectionStart, _selectionEnd;

  QTextLayout *_layout;
};

/*************************************************************************************************/

//! A ChatItem for the timestamp column
class TimestampChatItem : public ChatItem {

public:
  TimestampChatItem(QAbstractItemModel *model, QGraphicsItem *parent) : ChatItem(column(), model, parent) {}
  inline ChatLineModel::ColumnType column() const { return ChatLineModel::TimestampColumn; }

};

/*************************************************************************************************/

//! A ChatItem for the sender column
class SenderChatItem : public ChatItem {

public:
  SenderChatItem(QAbstractItemModel *model, QGraphicsItem *parent) : ChatItem(column(), model, parent) {}
  inline ChatLineModel::ColumnType column() const { return ChatLineModel::SenderColumn; }

  virtual void updateLayout();
};

/*************************************************************************************************/

//! A ChatItem for the contents column
class ContentsChatItem : public ChatItem {

public:
  ContentsChatItem(QAbstractItemModel *model, QGraphicsItem *parent);
  virtual ~ContentsChatItem();

  inline ChatLineModel::ColumnType column() const { return ChatLineModel::ContentsColumn; }

  virtual void clearLayout();
  virtual void updateLayout();
  virtual inline bool haveLayout() const { return _layoutData != 0 && layout() != 0; }

protected:
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

  virtual inline QTextLayout *layout() const;
  virtual void setLayout(QTextLayout *l);

private:
  struct LayoutData;
  class WrapColumnFinder;

  qreal computeHeight();

  LayoutData *_layoutData;
};

struct ContentsChatItem::LayoutData {
  QTextLayout *layout;

  LayoutData() { layout = 0; }
  ~LayoutData() { delete layout; }
};

class ContentsChatItem::WrapColumnFinder {
public:
  WrapColumnFinder(ChatItem *parent);
  ~WrapColumnFinder();

  qint16 nextWrapColumn();

private:
  ChatItem *item;
  QTextLayout *layout;
  QTextLine line;
  ChatLineModel::WrapList wrapList;
  qint16 wordidx;
  qint16 lastwrapcol;
  qreal lastwrappos;
  qreal w;
};

/*************************************************************************************************/

// Avoid circular include deps
#include "chatline.h"
int ChatItem::row() const { return static_cast<ChatLine *>(parentItem())->row(); }
QTextLayout *ContentsChatItem::layout() const { return _layoutData->layout; }

#endif
