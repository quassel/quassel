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
#include "messagefilter.h"

class AbstractUiMsg;
class ChatItem;
class ChatLine;
class WebPreviewItem;

class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene {
  Q_OBJECT

public:
  enum MyEventTypes {
    ClearWebPreviewEventType = QEvent::User
  };
  class ClearWebPreviewEvent;

  ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, QObject *parent);
  virtual ~ChatScene();

  inline QAbstractItemModel *model() const { return _model; }
  inline QString idString() const { return _idString; }

  int sectionByScenePos(int x);
  inline int sectionByScenePos(const QPoint &pos) { return sectionByScenePos(pos.x()); }
  inline bool isSingleBufferScene() const { return _singleBufferScene; }
  inline bool containsBuffer(const BufferId &id) const;
  inline ChatLine *chatLine(int row) { return (row < _lines.count()) ? _lines[row] : 0; }

  inline ColumnHandleItem *firstColumnHandle() const { return firstColHandle; }
  inline ColumnHandleItem *secondColumnHandle() const { return secondColHandle; }

  virtual bool eventFilter(QObject *watched, QEvent *event);

public slots:
  void updateForViewport(qreal width, qreal height);
  void setWidth(qreal, bool forceReposition = false);

  // these are used by the chatitems to notify the scene and manage selections
  void setSelectingItem(ChatItem *item);
  ChatItem *selectingItem() const { return _selectingItem; }
  void startGlobalSelection(ChatItem *item, const QPointF &itemPos);
  void putToClipboard(const QString &);

  void requestBacklog();

  void loadWebPreview(ChatItem *parentItem, const QString &url, const QRectF &urlRect);
  void clearWebPreview(ChatItem *parentItem = 0);

signals:
  void lastLineChanged(QGraphicsItem *item, qreal offset);

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
  virtual void customEvent(QEvent *event);

protected slots:
  void rowsInserted(const QModelIndex &, int, int);
  void rowsAboutToBeRemoved(const QModelIndex &, int, int);

private slots:
  void handlePositionChanged(qreal xpos);
  void showWebPreview();
  void clearWebPreviewEvent(ClearWebPreviewEvent *event);

private:
  void setHandleXLimits();
  void updateSelection(const QPointF &pos);
  QString selectionToString() const;

  QString _idString;
  QAbstractItemModel *_model;
  QList<ChatLine *> _lines;
  bool _singleBufferScene;

  // calls to QChatScene::sceneRect() are very expensive. As we manage the scenerect ourselves
  // we store the size in a member variable.
  QRectF _sceneRect;
  int _firstLineRow; // the first row to display (aka: not a daychange msg)
  void updateSceneRect();
  void updateSceneRect(qreal width);
  void updateSceneRect(const QRectF &rect);
  qreal _viewportHeight;

  ColumnHandleItem *firstColHandle, *secondColHandle;
  qreal firstColHandlePos, secondColHandlePos;

  ChatItem *_selectingItem;
  int _selectionStartCol, _selectionMinCol;
  int _selectionStart;
  int _selectionEnd;
  int _firstSelectionRow, _lastSelectionRow;
  bool _isSelecting;

  int _lastBacklogSize;

  struct WebPreview {
    ChatItem *parentItem;
    WebPreviewItem *previewItem;
    QString url;
    QRectF urlRect;
    QTimer delayTimer;
    WebPreview() : parentItem(0), previewItem(0) {}
  };
  WebPreview webPreview;
};

bool ChatScene::containsBuffer(const BufferId &id) const {
  MessageFilter *filter = qobject_cast<MessageFilter*>(model());
  if(filter)
    return filter->containsBuffer(id);
  else
    return false;
}

#endif
