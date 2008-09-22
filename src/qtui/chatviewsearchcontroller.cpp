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

#include "chatviewsearchcontroller.h"

#include <QAbstractItemModel>
#include <QPainter>

#include "chatitem.h"
#include "chatlinemodel.h"
#include "chatscene.h"
#include "messagemodel.h"

ChatViewSearchController::ChatViewSearchController(QObject *parent)
  : QObject(parent),
    _scene(0),
    _currentHighlight(0),
    _caseSensitive(false),
    _searchSenders(false),
    _searchMsgs(true),
    _searchOnlyRegularMsgs(true)
{
}

void ChatViewSearchController::setSearchString(const QString &searchString) {
  QString oldSearchString = _searchString;
  _searchString = searchString;
  if(_scene) {
    if(!searchString.startsWith(oldSearchString) || oldSearchString.isEmpty()) {
      // we can't reuse our all findings... cler the scene and do it all over
      updateHighlights();
    } else {
      // reuse all findings
      updateHighlights(true);
    }
  }
}

 void ChatViewSearchController::setScene(ChatScene *scene) {
  Q_ASSERT(scene);
  if(scene == _scene)
    return;

  if(_scene) {
    disconnect(_scene, 0, this, 0);
    qDeleteAll(_highlightItems);
    _highlightItems.clear();
  }

  _scene = scene;
  if(!scene)
    return;

  connect(_scene, SIGNAL(destroyed()), this, SLOT(sceneDestroyed()));
  updateHighlights();
 }

void ChatViewSearchController::highlightNext() {
  if(_highlightItems.isEmpty())
    return;

  if(_currentHighlight < _highlightItems.count()) {
    _highlightItems.at(_currentHighlight)->setHighlighted(false);
  }

  _currentHighlight++;
  if(_currentHighlight >= _highlightItems.count())
    _currentHighlight = 0;
  _highlightItems.at(_currentHighlight)->setHighlighted(true);
  emit newCurrentHighlight(_highlightItems.at(_currentHighlight));
}

void ChatViewSearchController::highlightPrev() {
  if(_highlightItems.isEmpty())
    return;

  if(_currentHighlight < _highlightItems.count()) {
    _highlightItems.at(_currentHighlight)->setHighlighted(false);
  }

  _currentHighlight--;
  if(_currentHighlight < 0)
    _currentHighlight = _highlightItems.count() - 1;
  _highlightItems.at(_currentHighlight)->setHighlighted(true);
  emit newCurrentHighlight(_highlightItems.at(_currentHighlight));
}

void ChatViewSearchController::updateHighlights(bool reuse) {
  if(!_scene)
    return;

  QAbstractItemModel *model = _scene->model();
  Q_ASSERT(model);


  QList<ChatLine *> chatLines;
  if(reuse) {
    foreach(SearchHighlightItem *highlightItem, _highlightItems) {
      ChatLine *line = dynamic_cast<ChatLine *>(highlightItem->parentItem());
      if(!line || chatLines.contains(line))
	continue;
      chatLines << line;
    }
  }

  qDeleteAll(_highlightItems);
  _highlightItems.clear();
  Q_ASSERT(_highlightItems.isEmpty());

  if(searchString().isEmpty() || !(_searchSenders || _searchMsgs))
    return;

  if(reuse) {
    QModelIndex index;
    foreach(ChatLine *line, chatLines) {
      if(_searchOnlyRegularMsgs) {
	index = model->index(line->row(), 0);
	if(!checkType((Message::Type)index.data(MessageModel::TypeRole).toInt()))
	  continue;
      }
      highlightLine(line);
    }
  } else {
    // we have to crawl through the data
    QModelIndex index;
    QString plainText;
    int rowCount = model->rowCount();
    for(int row = 0; row < rowCount; row++) {
      ChatLine *line = _scene->chatLine(row);

      if(_searchOnlyRegularMsgs) {
	index = model->index(row, 0);
	if(!checkType((Message::Type)index.data(MessageModel::TypeRole).toInt()))
	  continue;
      }
      highlightLine(line);
    }
  }

  if(!_highlightItems.isEmpty()) {
    _highlightItems.last()->setHighlighted(true);
    _currentHighlight = _highlightItems.count() - 1;
    emit newCurrentHighlight(_highlightItems.last());
  }
}

void ChatViewSearchController::highlightLine(ChatLine *line) {
  QList<ChatItem *> checkItems;
  if(_searchSenders)
    checkItems << &(line->item(MessageModel::SenderColumn));

  if(_searchMsgs)
    checkItems << &(line->item(MessageModel::ContentsColumn));

  foreach(ChatItem *item, checkItems) {
    foreach(QRectF wordRect, item->findWords(searchString(), caseSensitive())) {
      _highlightItems << new SearchHighlightItem(wordRect.adjusted(item->x(), 0, item->x(), 0), line);
    }
  }
}

void ChatViewSearchController::sceneDestroyed() {
  // WARNING: don't call any methods on scene!
  _scene = 0;
  // the items will be automatically deleted when the scene is destroyed
  // so we just have to clear the list;
  _highlightItems.clear();
}

void ChatViewSearchController::setCaseSensitive(bool caseSensitive) {
  if(_caseSensitive == caseSensitive)
    return;

  _caseSensitive = caseSensitive;

  // we can reuse the original search results if the new search
  // parameters are a restriction of the original one
  updateHighlights(caseSensitive);
}

void ChatViewSearchController::setSearchSenders(bool searchSenders) {
  if(_searchSenders == searchSenders)
    return;

  _searchSenders = searchSenders;
  // we can reuse the original search results if the new search
  // parameters are a restriction of the original one
  updateHighlights(!searchSenders);
}

void ChatViewSearchController::setSearchMsgs(bool searchMsgs) {
  if(_searchMsgs == searchMsgs)
    return;

  _searchMsgs = searchMsgs;

  // we can reuse the original search results if the new search
  // parameters are a restriction of the original one
  updateHighlights(!searchMsgs);
}

void ChatViewSearchController::setSearchOnlyRegularMsgs(bool searchOnlyRegularMsgs) {
  if(_searchOnlyRegularMsgs == searchOnlyRegularMsgs)
    return;

  _searchOnlyRegularMsgs = searchOnlyRegularMsgs;

  // we can reuse the original search results if the new search
  // parameters are a restriction of the original one
  updateHighlights(searchOnlyRegularMsgs);
}


// ==================================================
//  SearchHighlightItem
// ==================================================
SearchHighlightItem::SearchHighlightItem(QRectF wordRect, QGraphicsItem *parent)
  : QObject(),
    QGraphicsItem(parent),
    _highlighted(false),
    _alpha(100),
    _timeLine(150)
{
  setPos(wordRect.x(), wordRect.y());
  qreal sizedelta = wordRect.height() * 0.1;
  _boundingRect = QRectF(-sizedelta, -sizedelta, wordRect.width() + 2 * sizedelta, wordRect.height() + 2 * sizedelta);

  connect(&_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateHighlight(qreal)));
}

void SearchHighlightItem::setHighlighted(bool highlighted) {
  _highlighted = highlighted;

  if(highlighted)
    _timeLine.setDirection(QTimeLine::Forward);
  else
    _timeLine.setDirection(QTimeLine::Backward);

  if(_timeLine.state() != QTimeLine::Running)
    _timeLine.start();

  update();
}

void SearchHighlightItem::updateHighlight(qreal value) {
  _alpha = 100 + 155 * value;
  update();
}

void SearchHighlightItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->setPen(QPen(QColor(0, 0, 0, _alpha), 1.5));
  painter->setBrush(QColor(254, 237, 45, _alpha));
  painter->setRenderHints(QPainter::Antialiasing);
  qreal radius = boundingRect().height() * 0.30;
  painter->drawRoundedRect(boundingRect(), radius, radius);
}
