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

#ifndef _CHATSCENE_H_
#define _CHATSCENE_H_

#include <QAbstractItemModel>
#include <QGraphicsScene>
#include <QSet>

#include "types.h"

class AbstractUiMsg;
class Buffer;
class BufferId;
class ChatItem;
class ChatLine;
class ColumnHandleItem;

class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene {
  Q_OBJECT

  public:
    ChatScene(QAbstractItemModel *model, const QString &idString, QObject *parent);
    virtual ~ChatScene();

    inline QAbstractItemModel *model() const { return _model; }
    inline QString idString() const { return _idString; }

    inline bool isFetchingBacklog() const;
    inline bool isBacklogFetchingEnabled() const;
    inline BufferId bufferForBacklogFetching() const;
    int sectionByScenePos(int x);
    inline int sectionByScenePos(const QPoint &pos) { return sectionByScenePos(pos.x()); }

  public slots:
    void setWidth(qreal);

    // these are used by the chatitems to notify the scene and manage selections
    void setSelectingItem(ChatItem *item);
    ChatItem *selectingItem() const { return _selectingItem; }
    void startGlobalSelection(ChatItem *item, const QPointF &itemPos);

    void setIsFetchingBacklog(bool);
    inline void setBufferForBacklogFetching(BufferId buffer);

  signals:
    void heightChanged(qreal height);

  protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

  protected slots:
    void rowsInserted(const QModelIndex &, int, int);
    void modelReset();

  private slots:
    void rectChanged(const QRectF &);
    void handlePositionChanged(qreal xpos);

  private:
    void updateSelection(const QPointF &pos);
    QString selectionToString() const;
    void requestBacklogIfNeeded();

    QString _idString;
    qreal _width, _height;
    QAbstractItemModel *_model;
    QList<ChatLine *> _lines;

    ColumnHandleItem *firstColHandle, *secondColHandle;
    qreal firstColHandlePos, secondColHandlePos;

    ChatItem *_selectingItem, *_lastItem;
    QSet<ChatLine *> _selectedItems;
    int _selectionStartCol, _selectionMinCol;
    int _selectionStart;
    int _selectionEnd;
    bool _isSelecting;

    bool _fetchingBacklog;
    BufferId _backlogFetchingBuffer;
    MsgId _lastBacklogOffset;
    int _lastBacklogSize;
};

bool ChatScene::isFetchingBacklog() const {
  return _fetchingBacklog;
}

bool ChatScene::isBacklogFetchingEnabled() const {
  return _backlogFetchingBuffer.isValid();
}

BufferId ChatScene::bufferForBacklogFetching() const {
  return _backlogFetchingBuffer;
}

void ChatScene::setBufferForBacklogFetching(BufferId buf) {
  _backlogFetchingBuffer = buf;
}

#endif
