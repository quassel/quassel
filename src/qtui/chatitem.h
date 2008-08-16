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

  public:
    ChatItem(int col, QAbstractItemModel *, QGraphicsItem *parent);
    virtual ~ChatItem();

    inline const QAbstractItemModel *model() const { return chatScene() ? chatScene()->model() : 0; }
    int row() const;
    inline int column() const { return _col; }
    inline ChatScene *chatScene() const { return qobject_cast<ChatScene *>(scene()); }

    inline QFontMetricsF *fontMetrics() const { return _fontMetrics; }
    inline virtual QRectF boundingRect() const { return _boundingRect; }
    inline qreal width() const { return _boundingRect.width(); }
    inline qreal height() const { return _boundingRect.height(); }

    inline bool haveLayout() const { return _layoutData != 0 && layout() != 0; }
    void clearLayoutData();
    void updateLayout();
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
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

  private:
    struct LayoutData;
    class WrapColumnFinder;

    inline QTextLayout *layout() const;
    void setLayout(QTextLayout *);
    qint16 posToCursor(const QPointF &pos);
    qreal computeHeight();
    QTextLayout *createLayout(QTextOption::WrapMode, Qt::Alignment = Qt::AlignLeft);

    // internal selection stuff
    void setSelection(int start, int length);

    QRectF _boundingRect;
    QFontMetricsF *_fontMetrics;
    int _col;
    quint8 _lines;

    enum SelectionMode { NoSelection, PartialSelection, FullSelection };
    SelectionMode _selectionMode;
    qint16 _selectionStart, _selectionEnd;

    LayoutData *_layoutData;
};

struct ChatItem::LayoutData {
  QTextLayout *layout;

  LayoutData() { layout = 0; }
  ~LayoutData() { delete layout; }
};

class ChatItem::WrapColumnFinder {
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

#include "chatline.h"
inline int ChatItem::row() const { return static_cast<ChatLine *>(parentItem())->row(); }
inline QTextLayout *ChatItem::layout() const { return _layoutData->layout; }

#endif
