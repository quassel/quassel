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

#include <QApplication>
#include <QClipboard>
#include <QGraphicsSceneMouseEvent>
#include <QPersistentModelIndex>

#include "chatitem.h"
#include "chatline.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "client.h"
#include "clientbacklogmanager.h"
#include "columnhandleitem.h"
#include "messagefilter.h"
#include "qtui.h"
#include "qtuistyle.h"
#include "chatviewsettings.h"
#include "webpreviewitem.h"

const qreal minContentsWidth = 200;

ChatScene::ChatScene(QAbstractItemModel *model, const QString &idString, qreal width, QObject *parent)
  : QGraphicsScene(0, 0, width, 0, parent),
    _idString(idString),
    _model(model),
    _singleBufferScene(false),
    _sceneRect(0, 0, width, 0),
    _viewportHeight(0),
    _selectingItem(0),
    _selectionStart(-1),
    _isSelecting(false),
    _lastBacklogSize(0)
{
  MessageFilter *filter = qobject_cast<MessageFilter*>(model);
  if(filter) {
    _singleBufferScene = filter->isSingleBufferFilter();
  }

  ChatViewSettings defaultSettings;
  int defaultFirstColHandlePos = defaultSettings.value("FirstColumnHandlePos", 80).toInt();
  int defaultSecondColHandlePos = defaultSettings.value("SecondColumnHandlePos", 200).toInt();

  ChatViewSettings viewSettings(this);
  firstColHandlePos = viewSettings.value("FirstColumnHandlePos", defaultFirstColHandlePos).toInt();
  secondColHandlePos = viewSettings.value("SecondColumnHandlePos", defaultSecondColHandlePos).toInt();

  firstColHandle = new ColumnHandleItem(QtUi::style()->firstColumnSeparator());
  addItem(firstColHandle);
  firstColHandle->setXPos(firstColHandlePos);
  connect(firstColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(handlePositionChanged(qreal)));
  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), firstColHandle, SLOT(sceneRectChanged(const QRectF &)));

  secondColHandle = new ColumnHandleItem(QtUi::style()->secondColumnSeparator());
  addItem(secondColHandle);
  secondColHandle->setXPos(secondColHandlePos);
  connect(secondColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(handlePositionChanged(qreal)));
  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), secondColHandle, SLOT(sceneRectChanged(const QRectF &)));

  setHandleXLimits();

  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
	  this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
	  this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));

  if(model->rowCount() > 0)
    rowsInserted(QModelIndex(), 0, model->rowCount() - 1);
}

ChatScene::~ChatScene() {
}

void ChatScene::rowsInserted(const QModelIndex &index, int start, int end) {
  Q_UNUSED(index);

  clearWebPreview();

  qreal h = 0;
  qreal y = _sceneRect.y();
  qreal width = _sceneRect.width();
  bool atTop = true;
  bool atBottom = false;
  bool moveTop = false;

  if(start > 0) {
    y = _lines.value(start - 1)->y() + _lines.value(start - 1)->height();
    atTop = false;
  }
  if(start == _lines.count())
    atBottom = true;

  qreal contentsWidth = width - secondColumnHandle()->sceneRight();
  qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
  qreal timestampWidth = firstColumnHandle()->sceneLeft();
  QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
  QPointF senderPos(firstColumnHandle()->sceneRight(), 0);


  for(int i = end; i >= start; i--) {
    ChatLine *line = new ChatLine(i, model(),
				  width,
				  timestampWidth, senderWidth, contentsWidth,
				  senderPos, contentsPos);
    if(atTop) {
      h -= line->height();
      line->setPos(0, y+h);
    } else {
      line->setPos(0, y+h);
      h += line->height();
    }
    _lines.insert(start, line);
    addItem(line);
  }

  // update existing items
  for(int i = end+1; i < _lines.count(); i++) {
    _lines[i]->setRow(i);
  }

  // update selection
  if(_selectionStart >= 0) {
    int offset = end - start + 1;
    if(_selectionStart >= start) _selectionStart += offset;
    if(_selectionEnd >= start) _selectionEnd += offset;
    if(_firstSelectionRow >= start) _firstSelectionRow += offset;
    if(_lastSelectionRow >= start) _lastSelectionRow += offset;
  }

  // neither pre- or append means we have to do dirty work: move items...
  if(!(atTop || atBottom)) {
    qreal offset = h;
    int moveStart = 0;
    int moveEnd = _lines.count() - 1;
    ChatLine *line = 0;
    if(end > _lines.count() - end) {
      // move top part
      moveTop = true;
      offset = -offset;
      moveEnd = end;
    } else {
      // move bottom part
      moveStart = start;
    }
    for(int i = moveStart; i <= moveEnd; i++) {
      line = _lines.at(i);
      line->setPos(0, line->pos().y() + offset);
    }
  }

  // update sceneRect
  if(atTop || moveTop) {
    updateSceneRect(_sceneRect.adjusted(0, h, 0, 0));
  } else {
    updateSceneRect(_sceneRect.adjusted(0, 0, 0, h));
    emit lastLineChanged(_lines.last());
  }

}

void ChatScene::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  Q_UNUSED(parent);

  clearWebPreview();

  qreal h = 0; // total height of removed items;

  bool atTop = (start == 0);
  bool atBottom = (end == _lines.count() - 1);
  bool moveTop = false;

  // remove items from scene
  QList<ChatLine *>::iterator lineIter = _lines.begin() + start;
  int lineCount = start;
  while(lineIter != _lines.end() && lineCount <= end) {
    h += (*lineIter)->height();
    delete *lineIter;
    lineIter = _lines.erase(lineIter);
    lineCount++;
  }

  // update rows of remaining chatlines
  for(int i = start; i < _lines.count(); i++) {
    _lines.at(i)->setRow(i);
  }

  // update selection
  if(_selectionStart >= 0) {
    int offset = end - start + 1;
    if(_selectionStart >= start)
      _selectionStart -= offset;
    if(_selectionEnd >= start)
      _selectionEnd -= offset;
    if(_firstSelectionRow >= start)
      _firstSelectionRow -= offset;
    if(_lastSelectionRow >= start)
      _lastSelectionRow -= offset;
  }

  // neither removing at bottom or top means we have to move items...
  if(!(atTop || atBottom)) {
    qreal offset = h;
    int moveStart = 0;
    int moveEnd = _lines.count() - 1;
    ChatLine *line = 0;
    if(start > _lines.count() - end) {
      // move top part
      moveTop = true;
      moveEnd = start - 1;
    } else {
      // move bottom part
      moveStart = start;
      offset = -offset;
    }
    for(int i = moveStart; i <= moveEnd; i++) {
      line = _lines.at(i);
      line->setPos(0, line->pos().y() + offset);
    }
  }

  // update sceneRect
  if(atTop || moveTop) {
    updateSceneRect(_sceneRect.adjusted(0, h, 0, 0));
  } else {
    updateSceneRect(_sceneRect.adjusted(0, 0, 0, -h));
  }
}

void ChatScene::updateForViewport(qreal width, qreal height) {
  _viewportHeight = height;
  setWidth(width);
}

// setWidth is used for 2 things:
//  a) updating the scene to fit the width of the corresponding view
//  b) to update the positions of the items if a columhandle has changed it's position
// forceReposition is true in the second case
// this method features some codeduplication for the sake of performance
void ChatScene::setWidth(qreal width, bool forceReposition) {
  if(width == _sceneRect.width() && !forceReposition)
    return;

//   clock_t startT = clock();

  qreal linePos = _sceneRect.y() + _sceneRect.height();
  qreal yBottom = linePos;
  QList<ChatLine *>::iterator lineIter = _lines.end();
  QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
  ChatLine *line = 0;
  qreal lineHeight = 0;
  qreal contentsWidth = width - secondColumnHandle()->sceneRight();

  if(forceReposition) {
    qreal timestampWidth = firstColumnHandle()->sceneLeft();
    qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
    QPointF senderPos(firstColumnHandle()->sceneRight(), 0);
    QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
    while(lineIter != lineIterBegin) {
      lineIter--;
      line = *lineIter;
      lineHeight = line->setColumns(timestampWidth, senderWidth, contentsWidth, senderPos, contentsPos);
      linePos -= lineHeight;
      line->setPos(0, linePos);
    }
  } else {
    while(lineIter != lineIterBegin) {
      lineIter--;
      line = *lineIter;
      lineHeight = line->setGeometryByWidth(width, contentsWidth);
      linePos -= lineHeight;
      line->setPos(0, linePos);
    }
  }

  updateSceneRect(QRectF(0, linePos, width, yBottom - linePos));
  setHandleXLimits();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}

void ChatScene::handlePositionChanged(qreal xpos) {
  bool first = (sender() == firstColHandle);
  qreal oldx;
  if(first) {
    oldx = firstColHandlePos;
    firstColHandlePos = xpos;
  } else {
    oldx = secondColHandlePos;
    secondColHandlePos = xpos;
  }

  ChatViewSettings viewSettings(this);
  viewSettings.setValue("FirstColumnHandlePos", firstColHandlePos);
  viewSettings.setValue("SecondColumnHandlePos", secondColHandlePos);

  ChatViewSettings defaultSettings;
  defaultSettings.setValue("FirstColumnHandlePos", firstColHandlePos);
  defaultSettings.setValue("SecondColumnHandlePos", secondColHandlePos);

  setWidth(width(), true);  // readjust all chatlines
  // we get ugly redraw errors if we don't update this explicitly... :(
  // width() should be the same for both handles, so just use firstColHandle regardless
  //update(qMin(oldx, xpos), 0, qMax(oldx, xpos) + firstColHandle->width(), height());
}

void ChatScene::setHandleXLimits() {
  firstColHandle->setXLimits(0, secondColHandle->sceneLeft());
  secondColHandle->setXLimits(firstColHandle->sceneRight(), width() - minContentsWidth);
}

void ChatScene::setSelectingItem(ChatItem *item) {
  if(_selectingItem) _selectingItem->clearSelection();
  _selectingItem = item;
}

void ChatScene::startGlobalSelection(ChatItem *item, const QPointF &itemPos) {
  _selectionStart = _selectionEnd = _lastSelectionRow = _firstSelectionRow = item->row();
  _selectionStartCol = _selectionMinCol = item->column();
  _isSelecting = true;
  _lines[_selectionStart]->setSelected(true, (ChatLineModel::ColumnType)_selectionMinCol);
  updateSelection(item->mapToScene(itemPos));
}

void ChatScene::updateSelection(const QPointF &pos) {
  // This is somewhat hacky... we look at the contents item that is at the cursor's y position (ignoring x), since
  // it has the full height. From this item, we can then determine the row index and hence the ChatLine.
  ChatItem *contentItem = static_cast<ChatItem *>(itemAt(QPointF(secondColHandle->sceneRight() + 1, pos.y())));
  if(!contentItem) return;

  int curRow = contentItem->row();
  int curColumn;
  if(pos.x() > secondColHandle->sceneRight()) curColumn = ChatLineModel::ContentsColumn;
  else if(pos.x() > firstColHandlePos) curColumn = ChatLineModel::SenderColumn;
  else curColumn = ChatLineModel::TimestampColumn;

  ChatLineModel::ColumnType minColumn = (ChatLineModel::ColumnType)qMin(curColumn, _selectionStartCol);
  if(minColumn != _selectionMinCol) {
    _selectionMinCol = minColumn;
    for(int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
      _lines[l]->setSelected(true, minColumn);
    }
  }
  int newstart = qMin(curRow, _firstSelectionRow);
  int newend = qMax(curRow, _firstSelectionRow);
  if(newstart < _selectionStart) {
    for(int l = newstart; l < _selectionStart; l++)
      _lines[l]->setSelected(true, minColumn);
  }
  if(newstart > _selectionStart) {
    for(int l = _selectionStart; l < newstart; l++)
      _lines[l]->setSelected(false);
  }
  if(newend > _selectionEnd) {
    for(int l = _selectionEnd+1; l <= newend; l++)
      _lines[l]->setSelected(true, minColumn);
  }
  if(newend < _selectionEnd) {
    for(int l = newend+1; l <= _selectionEnd; l++)
      _lines[l]->setSelected(false);
  }

  _selectionStart = newstart;
  _selectionEnd = newend;
  _lastSelectionRow = curRow;

  if(newstart == newend && minColumn == ChatLineModel::ContentsColumn) {
    if(!_selectingItem) {
      qWarning() << "WARNING: ChatScene::updateSelection() has a null _selectingItem, this should never happen! Please report.";
      return;
    }
    _lines[curRow]->setSelected(false);
    _isSelecting = false;
    _selectingItem->continueSelecting(_selectingItem->mapFromScene(pos));
  }
}

void ChatScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if(_isSelecting && event->buttons() == Qt::LeftButton) {
    updateSelection(event->scenePos());
    event->accept();
  } else {
    QGraphicsScene::mouseMoveEvent(event);
  }
}

void ChatScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() == Qt::LeftButton && _selectionStart >= 0) {
    for(int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
      _lines[l]->setSelected(false);
    }
    _selectionStart = -1;
    QGraphicsScene::mousePressEvent(event);  // so we can start a new local selection
  } else {
    QGraphicsScene::mousePressEvent(event);
  }
}

void ChatScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if(_isSelecting && !event->buttons() & Qt::LeftButton) {
    putToClipboard(selectionToString());
    _isSelecting = false;
    event->accept();
  } else {
    QGraphicsScene::mouseReleaseEvent(event);
  }
}

void ChatScene::putToClipboard(const QString &selection) {
  // TODO Configure clipboards
#   ifdef Q_WS_X11
  QApplication::clipboard()->setText(selection, QClipboard::Selection);
#   endif
//# else
  QApplication::clipboard()->setText(selection);
//# endif
}

//!\brief Convert current selection to human-readable string.
QString ChatScene::selectionToString() const {
  //TODO Make selection format configurable!
  if(!_isSelecting) return QString();
  int start = qMin(_selectionStart, _selectionEnd);
  int end = qMax(_selectionStart, _selectionEnd);
  if(start < 0 || end >= _lines.count()) {
    qDebug() << "Invalid selection range:" << start << end;
    return QString();
  }
  QString result;
  for(int l = start; l <= end; l++) {
    if(_selectionMinCol == ChatLineModel::TimestampColumn)
      result += _lines[l]->item(ChatLineModel::TimestampColumn).data(MessageModel::DisplayRole).toString() + " ";
    if(_selectionMinCol <= ChatLineModel::SenderColumn)
      result += _lines[l]->item(ChatLineModel::SenderColumn).data(MessageModel::DisplayRole).toString() + " ";
    result += _lines[l]->item(ChatLineModel::ContentsColumn).data(MessageModel::DisplayRole).toString() + "\n";
  }
  return result;
}

void ChatScene::requestBacklog() {
  static const int REQUEST_COUNT = 500;
  int backlogSize = model()->rowCount();
  if(isSingleBufferScene() && backlogSize != 0 && _lastBacklogSize + REQUEST_COUNT <= backlogSize) {
    QModelIndex msgIdx = model()->index(0, 0);
    MsgId msgId = model()->data(msgIdx, ChatLineModel::MsgIdRole).value<MsgId>();
    BufferId bufferId = model()->data(msgIdx, ChatLineModel::BufferIdRole).value<BufferId>();
    _lastBacklogSize = backlogSize;
    Client::backlogManager()->requestBacklog(bufferId, REQUEST_COUNT, msgId.toInt());
  }
}

int ChatScene::sectionByScenePos(int x) {
  if(x < firstColHandle->x())
    return ChatLineModel::TimestampColumn;
  if(x < secondColHandle->x())
    return ChatLineModel::SenderColumn;

  return ChatLineModel::ContentsColumn;
}

void ChatScene::updateSceneRect(const QRectF &rect) {
  _sceneRect = rect;
  setSceneRect(rect);
}


void ChatScene::loadWebPreview(ChatItem *parentItem, const QString &url, const QRectF &urlRect) {
#ifndef HAVE_WEBKIT
  Q_UNUSED(parentItem)
  Q_UNUSED(url)
  Q_UNUSED(urlRect)
#else
  if(webPreview.parentItem != parentItem)
    webPreview.parentItem = parentItem;

  if(webPreview.url != url) {
    webPreview.url = url;
    // load a new web view and delete the old one (if exists)
    if(webPreview.previewItem) {
      removeItem(webPreview.previewItem);
      delete webPreview.previewItem;
    }
    webPreview.previewItem = new WebPreviewItem(url);
    addItem(webPreview.previewItem);
  }
  if(webPreview.urlRect != urlRect) {
    webPreview.urlRect = urlRect;
    qreal previewY = urlRect.bottom();
    qreal previewX = urlRect.x();
    if(previewY + webPreview.previewItem->boundingRect().height() > sceneRect().bottom())
      previewY = urlRect.y() - webPreview.previewItem->boundingRect().height();

    if(previewX + webPreview.previewItem->boundingRect().width() > sceneRect().width())
      previewX = sceneRect().right() - webPreview.previewItem->boundingRect().width();

    webPreview.previewItem->setPos(previewX, previewY);
  }
#endif
}

void ChatScene::clearWebPreview(ChatItem *parentItem) {
#ifndef HAVE_WEBKIT
  Q_UNUSED(parentItem)
#else
  if(parentItem == 0 || webPreview.parentItem == parentItem) {
    if(webPreview.previewItem) {
      removeItem(webPreview.previewItem);
      delete webPreview.previewItem;
      webPreview.previewItem = 0;
    }
    webPreview.parentItem = 0;
    webPreview.url = QString();
    webPreview.urlRect = QRectF();
  }
#endif
}
