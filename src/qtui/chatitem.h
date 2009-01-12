/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include <QAction>
#include <QGraphicsItem>
#include <QObject>

#include "chatlinemodel.h"
#include "chatscene.h"
#include "uistyle.h"
#include "qtui.h"

class QTextLayout;
struct ChatItemPrivate;

class ChatItem : public QGraphicsItem {
protected:
  ChatItem(const qreal &width, const qreal &height, const QPointF &pos, QGraphicsItem *parent);
  virtual ~ChatItem();

public:
  inline const QAbstractItemModel *model() const;
  inline int row() const;
  virtual ChatLineModel::ColumnType column() const = 0;
  inline ChatScene *chatScene() const { return qobject_cast<ChatScene *>(scene()); }

  inline QRectF boundingRect() const { return _boundingRect; }
  inline qreal width() const { return _boundingRect.width(); }
  inline qreal height() const { return _boundingRect.height(); }

  QTextLayout *createLayout(QTextOption::WrapMode, Qt::Alignment = Qt::AlignLeft) const;
  virtual void doLayout();
  void clearLayout();

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
  enum { Type = ChatScene::ChatItemType };
  virtual inline int type() const { return Type; }

  QVariant data(int role) const;

  // selection stuff, to be called by the scene
  QString selection() const;
  void clearSelection();
  void setFullSelection();
  void continueSelecting(const QPointF &pos);
  bool hasSelection() const;
  bool isPosOverSelection(const QPointF &pos) const;

  QList<QRectF> findWords(const QString &searchWord, Qt::CaseSensitivity caseSensitive);

  virtual void addActionsToMenu(QMenu *menu, const QPointF &itemPos);
  virtual void handleClick(const QPointF &pos, ChatScene::ClickMode);

protected:
  enum SelectionMode {
    NoSelection,
    PartialSelection,
    FullSelection
  };

  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

  inline QTextLayout *layout() const;

  virtual QTextLayout::FormatRange selectionFormat() const;
  virtual inline QVector<QTextLayout::FormatRange> additionalFormats() const { return QVector<QTextLayout::FormatRange>(); }

  inline qint16 selectionStart() const { return _selectionStart; }
  inline void setSelectionStart(qint16 start) { _selectionStart = start; }
  inline qint16 selectionEnd() const { return _selectionEnd; }
  inline void setSelectionEnd(qint16 end) { _selectionEnd = end; }
  inline SelectionMode selectionMode() const { return _selectionMode; }
  inline void setSelectionMode(SelectionMode mode) { _selectionMode = mode; }
  void setSelection(SelectionMode mode, qint16 selectionStart, qint16 selectionEnd);

  qint16 posToCursor(const QPointF &pos) const;

  inline bool hasPrivateData() const { return (bool)_data; }
  ChatItemPrivate *privateData() const;
  virtual inline ChatItemPrivate *newPrivateData();

  // WARNING: setGeometry and setHeight should not be used without either:
  //  a) calling prepareGeometryChange() immediately before setColumns()
  //  b) calling Chatline::setPos() immediately afterwards
  inline void setGeometry(qreal width, qreal height) {
    _boundingRect.setWidth(width);
    _boundingRect.setHeight(height);
  }
  inline void setHeight(const qreal &height) {
    _boundingRect.setHeight(height);
  }
  inline void setWidth(const qreal &width) {
    _boundingRect.setWidth(width);
  }

private:
  // internal selection stuff
  void setSelection(int start, int length);

  ChatItemPrivate *_data;
  QRectF _boundingRect;

  SelectionMode _selectionMode;
  qint16 _selectionStart, _selectionEnd;

  friend class ChatLine;
};

struct ChatItemPrivate {
  QTextLayout *layout;
  ChatItemPrivate(QTextLayout *l) : layout(l) {}
  virtual ~ChatItemPrivate() {
    delete layout;
  }
};

// inlines of ChatItem
QTextLayout *ChatItem::layout() const { return privateData()->layout; }
ChatItemPrivate *ChatItem::newPrivateData() { return new ChatItemPrivate(createLayout(QTextOption::WrapAnywhere)); }

// ************************************************************
// TimestampChatItem
// ************************************************************

//! A ChatItem for the timestamp column
class TimestampChatItem : public ChatItem {
public:
  TimestampChatItem(const qreal &width, const qreal &height, QGraphicsItem *parent) : ChatItem(width, height, QPointF(0, 0), parent) {}
  enum { Type = ChatScene::TimestampChatItemType };
  virtual inline int type() const { return Type; }
  virtual inline ChatLineModel::ColumnType column() const { return ChatLineModel::TimestampColumn; }
};

// ************************************************************
// SenderChatItem
// ************************************************************
//! A ChatItem for the sender column
class SenderChatItem : public ChatItem {
public:
  SenderChatItem(const qreal &width, const qreal &height, const QPointF &pos, QGraphicsItem *parent) : ChatItem(width, height, pos, parent) {}
  virtual inline ChatLineModel::ColumnType column() const { return ChatLineModel::SenderColumn; }

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
  enum { Type = ChatScene::SenderChatItemType };
  virtual inline int type() const { return Type; }
  virtual inline ChatItemPrivate *newPrivateData() { return new ChatItemPrivate(createLayout(QTextOption::ManualWrap, Qt::AlignRight)); }
};

// ************************************************************
// ContentsChatItem
// ************************************************************
struct ContentsChatItemPrivate;

//! A ChatItem for the contents column
class ContentsChatItem : public ChatItem {
  Q_DECLARE_TR_FUNCTIONS(ContentsChatItem);

public:
  ContentsChatItem(const qreal &width, const QPointF &pos, QGraphicsItem *parent);

  enum { Type = ChatScene::ContentsChatItemType };
  virtual inline int type() const { return Type; }

  inline ChatLineModel::ColumnType column() const { return ChatLineModel::ContentsColumn; }
  inline QFontMetricsF *fontMetrics() const { return _fontMetrics; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
  virtual void handleClick(const QPointF &pos, ChatScene::ClickMode clickMode);

  virtual void addActionsToMenu(QMenu *menu, const QPointF &itemPos);
  virtual void copyLinkToClipboard();

  virtual QVector<QTextLayout::FormatRange> additionalFormats() const;

  virtual void doLayout();
  virtual inline ChatItemPrivate *newPrivateData();

private:
  struct Clickable;
  class ActionProxy;
  class WrapColumnFinder;

  inline ContentsChatItemPrivate *privateData() const;

  QList<Clickable> findClickables() const;
  Clickable clickableAt(const QPointF &pos) const;

  void endHoverMode();
  void showWebPreview(const Clickable &click);
  void clearWebPreview();

  qreal setGeometryByWidth(qreal w);
  friend class ChatLine;
  friend struct ContentsChatItemPrivate;

  QFontMetricsF *_fontMetrics;

  // we need a receiver for Action signals
  static ActionProxy _actionProxy;
};

struct ContentsChatItem::Clickable {
  // Don't change these enums without also changing the regexps in analyze()!
  enum Type {
    Invalid = -1,
    Url = 0,
    Channel = 1,
    Nick = 2
  };

  Type type;
  quint16 start;
  quint16 length;

  inline Clickable() : type(Invalid) {};
  inline Clickable(Type type_, quint16 start_, quint16 length_) : type(type_), start(start_), length(length_) {};
  inline bool isValid() const { return type != Invalid; }
};

struct ContentsChatItemPrivate : ChatItemPrivate {
  ContentsChatItem *contentsItem;
  QList<ContentsChatItem::Clickable> clickables;
  ContentsChatItem::Clickable currentClickable;
  ContentsChatItem::Clickable activeClickable;

  ContentsChatItemPrivate(QTextLayout *l, const QList<ContentsChatItem::Clickable> &c, ContentsChatItem *parent)
  : ChatItemPrivate(l), contentsItem(parent), clickables(c) {}
};

//inlines regarding ContentsChatItemPrivate
ChatItemPrivate *ContentsChatItem::newPrivateData() {
  return new ContentsChatItemPrivate(createLayout(QTextOption::WrapAnywhere), findClickables(), this);
}
ContentsChatItemPrivate *ContentsChatItem::privateData() const { return (ContentsChatItemPrivate *)ChatItem::privateData(); }

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
  qint16 lineCount;
  qreal choppedTrailing;
};

//! Acts as a proxy for Action signals targetted at a ContentsChatItem
/** Since a ChatItem is not a QObject, hence cannot receive signals, we use a static ActionProxy
 *  as a receiver instead. This avoids having to handle ChatItem actions (e.g. context menu entries)
 *  outside the ChatItem.
 */
class ContentsChatItem::ActionProxy : public QObject {
  Q_OBJECT

public slots:
  inline void copyLinkToClipboard() { item()->copyLinkToClipboard(); }

private:
  /// Returns the ContentsChatItem that should receive the action event.
  /** For efficiency reasons, values are not checked for validity. You gotta make sure that you set the data() member
   *  in the Action correctly.
   *  @return The ChatItem from which the sending Action originated
   */
  inline ContentsChatItem *item() const {
    return static_cast<ContentsChatItem *>(qobject_cast<QAction *>(sender())->data().value<void *>());
  }
};

/*************************************************************************************************/

// Avoid circular include deps
#include "chatline.h"
const QAbstractItemModel *ChatItem::model() const { return static_cast<ChatLine *>(parentItem())->model(); }
int ChatItem::row() const { return static_cast<ChatLine *>(parentItem())->row(); }

#endif
