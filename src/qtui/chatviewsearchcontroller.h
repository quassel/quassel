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

#ifndef CHATVIEWSEARCHCONTROLLER_H
#define CHATVIEWSEARCHCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>

#include "message.h"

class ChatLine;
class ChatScene;
class SearchHighlightItem;

class ChatViewSearchController : public QObject {
  Q_OBJECT

public:
  ChatViewSearchController(QObject *parent = 0);

  inline const QString &searchString() const { return _searchString; }

  void setScene(ChatScene *scene);

public slots:
  void setSearchString(const QString &searchString);
  void setCaseSensitive(bool caseSensitive);
  void setSearchSenders(bool searchSenders);
  void setSearchMsgs(bool searchMsgs);
  void setSearchOnlyRegularMsgs(bool searchOnlyRegularMsgs);

private slots:
  void sceneDestroyed();
  void updateHighlights(bool reuse = false);

private:
  QString _searchString;
  ChatScene *_scene;
  QList<SearchHighlightItem *> _highlightItems;

  bool _caseSensitive;
  bool _searchSenders;
  bool _searchMsgs;
  bool _searchOnlyRegularMsgs;

  inline Qt::CaseSensitivity caseSensitive() const { return _caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive; }

  inline bool checkType(Message::Type type) const { return type & (Message::Plain | Message::Notice | Message::Action); }
  void highlightLine(ChatLine *line);
};


// Highlight Items
#include <QGraphicsItem>

class SearchHighlightItem : public QGraphicsItem {
public:
  SearchHighlightItem(QRectF wordRect, QGraphicsItem *parent = 0);
  inline virtual QRectF boundingRect() const { return _boundingRect; }
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

private:
  QRectF _boundingRect;
};

#endif //CHATVIEWSEARCHCONTROLLER_H
