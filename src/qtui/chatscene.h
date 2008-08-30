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

#ifndef CHATSCENE_H_
#define CHATSCENE_H_

#include <QAbstractItemModel>
#include <QGraphicsScene>
#include <QSet>

#include "columnhandleitem.h"


class AbstractUiMsg;
class ChatItem;
class ChatLine;

class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene {
  Q_OBJECT

public:
  ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, QObject *parent);
  virtual ~ChatScene();

  inline QAbstractItemModel *model() const { return _model; }
  inline QString idString() const { return _idString; }

  int sectionByScenePos(int x);
  inline int sectionByScenePos(const QPoint &pos) { return sectionByScenePos(pos.x()); }
  inline bool isSingleBufferScene() const { return _singleBufferScene; }
  inline ChatLine *chatLine(int row) { return (row < _lines.count()) ? _lines[row] : 0; }

  inline QRectF firstColumnHandleRect() const { return firstColHandle->boundingRect().translated(firstColHandle->x(), 0); }
  inline QRectF secondColumnHandleRect() const { return secondColHandle->boundingRect().translated(secondColHandle->x(), 0); }

public slots:
  void setWidth(qreal, bool forceReposition = false);

  // these are used by the chatitems to notify the scene and manage selections
  void setSelectingItem(ChatItem *item);
  ChatItem *selectingItem() const { return _selectingItem; }
  void startGlobalSelection(ChatItem *item, const QPointF &itemPos);
  void putToClipboard(const QString &);

  void requestBacklog();

signals:
  void sceneHeightChanged(qreal dh);

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);

protected slots:
  void rowsInserted(const QModelIndex &, int, int);
  void rowsAboutToBeRemoved(const QModelIndex &, int, int);

private slots:
  void handlePositionChanged(qreal xpos);

private:
  void setHandleXLimits();
  void updateSelection(const QPointF &pos);
  QString selectionToString() const;

  QString _idString;
  QAbstractItemModel *_model;
  QList<ChatLine *> _lines;
  bool _singleBufferScene;

  ColumnHandleItem *firstColHandle, *secondColHandle;
  qreal firstColHandlePos, secondColHandlePos;

  ChatItem *_selectingItem;
  int _selectionStartCol, _selectionMinCol;
  int _selectionStart;
  int _selectionEnd;
  int _firstSelectionRow, _lastSelectionRow;
  bool _isSelecting;

  int _lastBacklogSize;
};

#endif
