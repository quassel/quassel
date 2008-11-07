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
#include <QWebView>

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
    _firstLineRow(-1),
    _viewportHeight(0),
    _cutoffMode(CutoffRight),
    _selectingItem(0),
    _selectionStart(-1),
    _isSelecting(false)
{
  MessageFilter *filter = qobject_cast<MessageFilter*>(model);
  if(filter) {
    _singleBufferScene = filter->isSingleBufferFilter();
  }

  ChatViewSettings defaultSettings;
  int defaultFirstColHandlePos = defaultSettings.value("FirstColumnHandlePos", 80).toInt();
  int defaultSecondColHandlePos = defaultSettings.value("SecondColumnHandlePos", 200).toInt();

  ChatViewSettings viewSettings(this);
  _firstColHandlePos = viewSettings.value("FirstColumnHandlePos", defaultFirstColHandlePos).toInt();
  _secondColHandlePos = viewSettings.value("SecondColumnHandlePos", defaultSecondColHandlePos).toInt();

  _firstColHandle = new ColumnHandleItem(QtUi::style()->firstColumnSeparator());
  addItem(_firstColHandle);
  _firstColHandle->setXPos(_firstColHandlePos);
  connect(_firstColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(firstHandlePositionChanged(qreal)));
  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), _firstColHandle, SLOT(sceneRectChanged(const QRectF &)));

  _secondColHandle = new ColumnHandleItem(QtUi::style()->secondColumnSeparator());
  addItem(_secondColHandle);
  _secondColHandle->setXPos(_secondColHandlePos);
  connect(_secondColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(secondHandlePositionChanged(qreal)));

  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), _secondColHandle, SLOT(sceneRectChanged(const QRectF &)));

  setHandleXLimits();

  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int, int)));

  if(model->rowCount() > 0)
    rowsInserted(QModelIndex(), 0, model->rowCount() - 1);

#ifdef HAVE_WEBKIT
  webPreview.delayTimer.setSingleShot(true);
  connect(&webPreview.delayTimer, SIGNAL(timeout()), this, SLOT(showWebPreviewEvent()));
  //webPreview.deleteTimer.setInterval(600000);
  webPreview.deleteTimer.setInterval(10000);
  connect(&webPreview.deleteTimer, SIGNAL(timeout()), this, SLOT(deleteWebPreviewEvent()));
#endif

  setItemIndexMethod(QGraphicsScene::NoIndex);
}

ChatScene::~ChatScene() {
}

void ChatScene::rowsInserted(const QModelIndex &index, int start, int end) {
  Q_UNUSED(index);


//   QModelIndex sidx = model()->index(start, 2);
//   QModelIndex eidx = model()->index(end, 2);
//   qDebug() << "rowsInserted:";
//   if(start > 0) {
//     QModelIndex ssidx = model()->index(start - 1, 2);
//     qDebug() << "Start--:" << start - 1 << ssidx.data(MessageModel::MsgIdRole).value<MsgId>()
// 	     << ssidx.data(Qt::DisplayRole).toString();
//   }
//   qDebug() << "Start:" << start << sidx.data(MessageModel::MsgIdRole).value<MsgId>()
// 	   << sidx.data(Qt::DisplayRole).toString();
//   qDebug() << "End:" << end << eidx.data(MessageModel::MsgIdRole).value<MsgId>()
// 	   << eidx.data(Qt::DisplayRole).toString();
//   if(end + 1 < model()->rowCount()) {
//     QModelIndex eeidx = model()->index(end + 1, 2);
//     qDebug() << "End++:" << end + 1 << eeidx.data(MessageModel::MsgIdRole).value<MsgId>()
// 	     << eeidx.data(Qt::DisplayRole).toString();
//   }

  qreal h = 0;
  qreal y = 0;
  qreal width = _sceneRect.width();
  bool atBottom = (start == _lines.count());
  bool atTop = !atBottom && (start == 0);
  bool moveTop = false;

  if(start < _lines.count()) {
    y = _lines.value(start)->y();
  } else if(atBottom && !_lines.isEmpty()) {
    y = _lines.last()->y() + _lines.last()->height();
  }

  qreal contentsWidth = width - secondColumnHandle()->sceneRight();
  qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
  qreal timestampWidth = firstColumnHandle()->sceneLeft();
  QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
  QPointF senderPos(firstColumnHandle()->sceneRight(), 0);

  if(atTop) {
    for(int i = end; i >= start; i--) {
      ChatLine *line = new ChatLine(i, model(),
                                    width,
                                    timestampWidth, senderWidth, contentsWidth,
                                    senderPos, contentsPos);
      h += line->height();
      line->setPos(0, y-h);
      _lines.insert(start, line);
      addItem(line);
    }
  } else {
    for(int i = start; i <= end; i++) {
      ChatLine *line = new ChatLine(i, model(),
                                    width,
                                    timestampWidth, senderWidth, contentsWidth,
                                    senderPos, contentsPos);
      line->setPos(0, y+h);
      h += line->height();
      _lines.insert(i, line);
      addItem(line);
    }
  }

  // update existing items
  for(int i = end+1; i < _lines.count(); i++) {
    _lines[i]->setRow(i);
  }

  // update selection
  if(_selectionStart >= 0) {
    int offset = end - start + 1;
    int oldStart = _selectionStart;
    if(_selectionStart >= start)
      _selectionStart += offset;
    if(_selectionEnd >= start) {
      _selectionEnd += offset;
      if(_selectionStart == oldStart)
        for(int i = start; i < start + offset; i++)
          _lines[i]->setSelected(true);
    }
    if(_firstSelectionRow >= start)
      _firstSelectionRow += offset;
  }

  // neither pre- or append means we have to do dirty work: move items...
  int moveStart = 0;
  int moveEnd = _lines.count() - 1;
  qreal offset = h;
  if(!(atTop || atBottom)) {
    // move top means: moving 0 to end (aka: end + 1)
    // move top means: moving end + 1 to _lines.count() - 1 (aka: _lines.count() - (end + 1)
    if(end + 1 < _lines.count() - end - 1) {
      // move top part
      moveTop = true;
      offset = -offset;
      moveEnd = end;
    } else {
      // move bottom part
      moveStart = end + 1;
    }
    ChatLine *line = 0;
    for(int i = moveStart; i <= moveEnd; i++) {
      line = _lines.at(i);
      line->setPos(0, line->pos().y() + offset);
    }
  }

  // check if all went right
  Q_ASSERT(start == 0 || _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() == _lines.at(start)->pos().y());
//   if(start != 0) {
//     if(_lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() != _lines.at(start)->pos().y()) {
//       qDebug() << "lines:" << _lines.count() << "start:" << start << "end:" << end;
//       qDebug() << "line[start - 1]:" << _lines.at(start - 1)->pos().y() << "+" << _lines.at(start - 1)->height() << "=" << _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height();
//       qDebug() << "line[start]" << _lines.at(start)->pos().y();
//       qDebug() << "needed moving:" << !(atTop || atBottom) << moveTop << moveStart << moveEnd << offset;
//       Q_ASSERT(false)
//     }
//   }
  Q_ASSERT(end + 1 == _lines.count() || _lines.at(end)->pos().y() + _lines.at(end)->height() == _lines.at(end + 1)->pos().y());
//   if(end + 1 < _lines.count()) {
//     if(_lines.at(end)->pos().y() + _lines.at(end)->height() != _lines.at(end + 1)->pos().y()) {
//       qDebug() << "lines:" << _lines.count() << "start:" << start << "end:" << end;
//       qDebug() << "line[end]:" << _lines.at(end)->pos().y() << "+" << _lines.at(end)->height() << "=" << _lines.at(end)->pos().y() + _lines.at(end)->height();
//       qDebug() << "line[end+1]" << _lines.at(end + 1)->pos().y();
//       qDebug() << "needed moving:" << !(atTop || atBottom) << moveTop << moveStart << moveEnd << offset;
//       Q_ASSERT(false);
//     }
//   }

  if(!atBottom) {
    if(start < _firstLineRow) {
      int prevFirstLineRow = _firstLineRow + (end - start + 1);
      for(int i = end + 1; i < prevFirstLineRow; i++) {
        _lines.at(i)->show();
      }
    }
    // force new search for first proper line
    _firstLineRow = -1;
  }
  updateSceneRect();
  if(atBottom || (!atTop && !moveTop)) {
    emit lastLineChanged(_lines.last(), h);
  }
}

void ChatScene::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  Q_UNUSED(parent);

  qreal h = 0; // total height of removed items;

  bool atTop = (start == 0);
  bool atBottom = (end == _lines.count() - 1);
  bool moveTop = false;

  // clear selection
  if(_selectingItem) {
    int row = _selectingItem->row();
    if(row >= start && row <= end)
      setSelectingItem(0);
  }

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
      _selectionStart = qMax(_selectionStart -= offset, start);
    if(_selectionEnd >= start)
      _selectionEnd -= offset;
    if(_firstSelectionRow >= start)
      _firstSelectionRow -= offset;

    if(_selectionEnd < _selectionStart) {
      _isSelecting = false;
      _selectionStart = -1;
    }
  }

  // neither removing at bottom or top means we have to move items...
  if(!(atTop || atBottom)) {
    qreal offset = h;
    int moveStart = 0;
    int moveEnd = _lines.count() - 1;
    if(start < _lines.count() - start) {
      // move top part
      moveTop = true;
      moveEnd = start - 1;
    } else {
      // move bottom part
      moveStart = start;
      offset = -offset;
    }
    ChatLine *line = 0;
    for(int i = moveStart; i <= moveEnd; i++) {
      line = _lines.at(i);
      line->setPos(0, line->pos().y() + offset);
    }
  }

  Q_ASSERT(start == 0 || start >= _lines.count() || _lines.at(start - 1)->pos().y() + _lines.at(start - 1)->height() == _lines.at(start)->pos().y());

  // update sceneRect
  // when searching for the first non-date-line we have to take into account that our
  // model still contains the just removed lines so we cannot simply call updateSceneRect()
  int numRows = model()->rowCount();
  QModelIndex firstLineIdx;
  _firstLineRow = -1;
  bool needOffset = false;
  do {
    _firstLineRow++;
    if(_firstLineRow >= start && _firstLineRow <= end) {
      _firstLineRow = end + 1;
      needOffset = true;
    }
    firstLineIdx = model()->index(_firstLineRow, 0);
  } while((Message::Type)(model()->data(firstLineIdx, MessageModel::TypeRole).toInt()) == Message::DayChange && _firstLineRow < numRows);

  if(needOffset)
    _firstLineRow -= end - start + 1;
  updateSceneRect();
}

void ChatScene::updateForViewport(qreal width, qreal height) {
  _viewportHeight = height;
  setWidth(width);
}

void ChatScene::setWidth(qreal width) {
  if(width == _sceneRect.width())
    return;

  // clock_t startT = clock();

  // disabling the index while doing this complex updates is about
  // 2 to 10 times faster!
  //setItemIndexMethod(QGraphicsScene::NoIndex);

  QList<ChatLine *>::iterator lineIter = _lines.end();
  QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
  qreal linePos = _sceneRect.y() + _sceneRect.height();
  qreal contentsWidth = width - secondColumnHandle()->sceneRight();
  while(lineIter != lineIterBegin) {
    lineIter--;
    (*lineIter)->setGeometryByWidth(width, contentsWidth, linePos);
  }
  //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  updateSceneRect(width);
  setHandleXLimits();
  emit layoutChanged();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}

void ChatScene::firstHandlePositionChanged(qreal xpos) {
  if(_firstColHandlePos == xpos)
    return;

  _firstColHandlePos = xpos;
  ChatViewSettings viewSettings(this);
  viewSettings.setValue("FirstColumnHandlePos", _firstColHandlePos);
  ChatViewSettings defaultSettings;
  defaultSettings.setValue("FirstColumnHandlePos", _firstColHandlePos);

  // clock_t startT = clock();

  // disabling the index while doing this complex updates is about
  // 2 to 10 times faster!
  //setItemIndexMethod(QGraphicsScene::NoIndex);

  QList<ChatLine *>::iterator lineIter = _lines.end();
  QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
  qreal timestampWidth = firstColumnHandle()->sceneLeft();
  qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
  QPointF senderPos(firstColumnHandle()->sceneRight(), 0);

  while(lineIter != lineIterBegin) {
    lineIter--;
    (*lineIter)->setFirstColumn(timestampWidth, senderWidth, senderPos);
  }
  //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  setHandleXLimits();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}

void ChatScene::secondHandlePositionChanged(qreal xpos) {
  if(_secondColHandlePos == xpos)
    return;

  _secondColHandlePos = xpos;
  ChatViewSettings viewSettings(this);
  viewSettings.setValue("SecondColumnHandlePos", _secondColHandlePos);
  ChatViewSettings defaultSettings;
  defaultSettings.setValue("SecondColumnHandlePos", _secondColHandlePos);

  // clock_t startT = clock();

  // disabling the index while doing this complex updates is about
  // 2 to 10 times faster!
  //setItemIndexMethod(QGraphicsScene::NoIndex);

  QList<ChatLine *>::iterator lineIter = _lines.end();
  QList<ChatLine *>::iterator lineIterBegin = _lines.begin();
  qreal linePos = _sceneRect.y() + _sceneRect.height();
  qreal senderWidth = secondColumnHandle()->sceneLeft() - firstColumnHandle()->sceneRight();
  qreal contentsWidth = _sceneRect.width() - secondColumnHandle()->sceneRight();
  QPointF contentsPos(secondColumnHandle()->sceneRight(), 0);
  while(lineIter != lineIterBegin) {
    lineIter--;
    (*lineIter)->setSecondColumn(senderWidth, contentsWidth, contentsPos, linePos);
  }
  //setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  updateSceneRect();
  setHandleXLimits();
  emit layoutChanged();

//   clock_t endT = clock();
//   qDebug() << "resized" << _lines.count() << "in" << (float)(endT - startT) / CLOCKS_PER_SEC << "sec";
}

void ChatScene::setHandleXLimits() {
  _firstColHandle->setXLimits(0, _secondColHandle->sceneLeft());
  _secondColHandle->setXLimits(_firstColHandle->sceneRight(), width() - minContentsWidth);
}

void ChatScene::setSelectingItem(ChatItem *item) {
  if(_selectingItem) _selectingItem->clearSelection();
  _selectingItem = item;
}

void ChatScene::startGlobalSelection(ChatItem *item, const QPointF &itemPos) {
  _selectionStart = _selectionEnd = _firstSelectionRow = item->row();
  _selectionStartCol = _selectionMinCol = item->column();
  _isSelecting = true;
  _lines[_selectionStart]->setSelected(true, (ChatLineModel::ColumnType)_selectionMinCol);
  updateSelection(item->mapToScene(itemPos));
}

void ChatScene::updateSelection(const QPointF &pos) {
  // This is somewhat hacky... we look at the contents item that is at the cursor's y position (ignoring x), since
  // it has the full height. From this item, we can then determine the row index and hence the ChatLine.
  ChatItem *contentItem = static_cast<ChatItem *>(itemAt(QPointF(_secondColHandle->sceneRight() + 1, pos.y())));
  if(!contentItem) return;

  int curRow = contentItem->row();
  int curColumn;
  if(pos.x() > _secondColHandle->sceneRight()) curColumn = ChatLineModel::ContentsColumn;
  else if(pos.x() > _firstColHandlePos) curColumn = ChatLineModel::SenderColumn;
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

  if(newstart == newend && minColumn == ChatLineModel::ContentsColumn) {
    if(!_selectingItem) {
      // _selectingItem has been removed already
      return;
    }
    _lines[curRow]->setSelected(false);
    _isSelecting = false;
    _selectingItem->continueSelecting(_selectingItem->mapFromScene(pos));
  }
}

bool ChatScene::isScrollingAllowed() const {
  if(_isSelecting)
    return false;

  // TODO: Handle clicks and single-item selections too

  return true;
}

void ChatScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if(_isSelecting && event->buttons() == Qt::LeftButton) {
    updateSelection(event->scenePos());
    emit mouseMoveWhileSelecting(event->scenePos());
    event->accept();
  } else {
    QGraphicsScene::mouseMoveEvent(event);
  }
}

void ChatScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if(_selectionStart >= 0 && event->buttons() == Qt::LeftButton) {
    for(int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
      _lines[l]->setSelected(false);
    }
    _isSelecting = false;
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
  MessageFilter *filter = qobject_cast<MessageFilter*>(model());
  if(filter)
    return filter->requestBacklog();
  return;
}

int ChatScene::sectionByScenePos(int x) {
  if(x < _firstColHandle->x())
    return ChatLineModel::TimestampColumn;
  if(x < _secondColHandle->x())
    return ChatLineModel::SenderColumn;

  return ChatLineModel::ContentsColumn;
}

void ChatScene::updateSceneRect(qreal width) {
  if(_lines.isEmpty()) {
    updateSceneRect(QRectF(0, 0, width, 0));
    return;
  }

  // we hide day change messages at the top by making the scene rect smaller
  // and by calling QGraphicsItem::hide() on all leading day change messages
  // the first one is needed to ensure proper scrollbar ranges
  // the second for cases where the viewport is larger then the set scenerect
  //  (in this case the items are shown anyways)
  if(_firstLineRow == -1) {
    int numRows = model()->rowCount();
    _firstLineRow = 0;
    QModelIndex firstLineIdx;
    while(_firstLineRow < numRows) {
      firstLineIdx = model()->index(_firstLineRow, 0);
      if((Message::Type)(model()->data(firstLineIdx, MessageModel::TypeRole).toInt()) != Message::DayChange)
        break;
      _lines.at(_firstLineRow)->hide();
      _firstLineRow++;
    }
  }

  // the following call should be safe. If it crashes something went wrong during insert/remove
  if(_firstLineRow < _lines.count()) {
    ChatLine *firstLine = _lines.at(_firstLineRow);
    ChatLine *lastLine = _lines.last();
    updateSceneRect(QRectF(0, firstLine->pos().y(), width, lastLine->pos().y() + lastLine->height() - firstLine->pos().y()));
  } else {
    // empty scene rect
    updateSceneRect(QRectF(0, 0, width, 0));
  }
}

void ChatScene::updateSceneRect(const QRectF &rect) {
  _sceneRect = rect;
  setSceneRect(rect);
  update();
}

bool ChatScene::event(QEvent *e) {
  if(e->type() == QEvent::ApplicationPaletteChange) {
    _firstColHandle->setColor(QApplication::palette().windowText().color());
    _secondColHandle->setColor(QApplication::palette().windowText().color());
  }
  return QGraphicsScene::event(e);
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
    if(webPreview.previewItem && webPreview.previewItem->scene()) {
      removeItem(webPreview.previewItem);
      delete webPreview.previewItem;
    }
    webPreview.previewItem = new WebPreviewItem(url);
    webPreview.delayTimer.start(2000);
    webPreview.deleteTimer.stop();
  } else if(webPreview.previewItem && !webPreview.previewItem->scene()) {
      // we just have to readd the item to the scene
      webPreview.delayTimer.start(2000);
      webPreview.deleteTimer.stop();
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

void ChatScene::showWebPreviewEvent() {
#ifdef HAVE_WEBKIT
  if(webPreview.previewItem)
    addItem(webPreview.previewItem);
#endif
}

void ChatScene::clearWebPreview(ChatItem *parentItem) {
#ifndef HAVE_WEBKIT
  Q_UNUSED(parentItem)
#else
  if(parentItem == 0 || webPreview.parentItem == parentItem) {
    if(webPreview.previewItem && webPreview.previewItem->scene()) {
      removeItem(webPreview.previewItem);
      webPreview.deleteTimer.start();
    }
    webPreview.delayTimer.stop();
  }
#endif
}

void ChatScene::deleteWebPreviewEvent() {
#ifdef HAVE_WEBKIT
  if(webPreview.previewItem) {
    delete webPreview.previewItem;
    webPreview.previewItem = 0;
  }
  webPreview.parentItem = 0;
  webPreview.url = QString();
  webPreview.urlRect = QRectF();
#endif
}
